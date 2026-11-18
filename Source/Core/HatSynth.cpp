#include "HatSynth.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <ranges>

namespace openhatter::core {

namespace {

[[nodiscard]] constexpr float pi() noexcept {
    return std::numbers::pi_v<float>;
}

// High-speed 32-bit xorshift PRNG for pristine noise without repetitive periodicity
[[nodiscard]] inline float nextRandomFloat(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<float>(state) * (2.0f / 4294967296.0f)) - 1.0f;
}

[[nodiscard]] BiquadCoeffs computeHighpassCoeffs(float freqHz, float q, double sampleRate) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.49f;
    const float clampedFreq = std::clamp(freqHz, 10.0f, nyquist);
    const float clampedQ = std::clamp(q, 0.1f, 10.0f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * clampedQ);
    const float a0 = 1.0f + alpha;
    return {
        .b0 = ((1.0f + cosW0) * 0.5f) / a0,
        .b1 = (-(1.0f + cosW0)) / a0,
        .b2 = ((1.0f + cosW0) * 0.5f) / a0,
        .a1 = (-2.0f * cosW0) / a0,
        .a2 = (1.0f - alpha) / a0
    };
}

[[nodiscard]] BiquadCoeffs computeLowpassCoeffs(float freqHz, float q, double sampleRate) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.49f;
    const float clampedFreq = std::clamp(freqHz, 20.0f, nyquist);
    const float clampedQ = std::clamp(q, 0.1f, 10.0f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * clampedQ);
    const float a0 = 1.0f + alpha;
    return {
        .b0 = ((1.0f - cosW0) * 0.5f) / a0,
        .b1 = (1.0f - cosW0) / a0,
        .b2 = ((1.0f - cosW0) * 0.5f) / a0,
        .a1 = (-2.0f * cosW0) / a0,
        .a2 = (1.0f - alpha) / a0
    };
}

[[nodiscard]] BiquadCoeffs computeBandpassCoeffs(float freqHz, float q, double sampleRate) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.45f;
    const float clampedFreq = std::clamp(freqHz, 40.0f, nyquist);
    const float clampedQ = std::clamp(q, 0.5f, 14.0f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * clampedQ);
    const float a0 = 1.0f + alpha;
    return {
        .b0 = alpha / a0,
        .b1 = 0.0f,
        .b2 = -alpha / a0,
        .a1 = (-2.0f * cosW0) / a0,
        .a2 = (1.0f - alpha) / a0
    };
}

[[nodiscard]] inline float processBiquad(float in, BiquadState& s, const BiquadCoeffs& c) noexcept {
    const float out = c.b0 * in + c.b1 * s.x1 + c.b2 * s.x2 - c.a1 * s.y1 - c.a2 * s.y2;
    s.x2 = s.x1;
    s.x1 = in;
    s.y2 = s.y1;
    s.y1 = out;
    return out;
}

[[nodiscard]] inline float advancePhase(float& phase, float hz, double sampleRate) noexcept {
    phase += hz / static_cast<float>(std::max(1000.0, sampleRate));
    phase -= std::floor(phase);
    return phase;
}

// Odd saturation only. An always-positive even term parked a DC offset on every driven hat.
[[nodiscard]] inline float softSaturate(float x, float drive) noexcept {
    if (drive <= 0.001f) return x;
    const float gain = 1.0f + 2.2f * std::clamp(drive, 0.0f, 1.0f);
    return std::tanh(x * gain) / std::tanh(gain);
}

[[nodiscard]] inline float dcBlock(float x, float& x1, float& y1) noexcept {
    const float y = x - x1 + 0.995f * y1;
    x1 = x;
    y1 = y;
    return y;
}

} // namespace

// ============================================================================
// GatedStereoReverb
// ============================================================================

void GatedStereoReverb::prepare(double sampleRate) {
    inputDiffusersL_[0].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0031)), 0.65f);
    inputDiffusersL_[1].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0079)), 0.60f);
    inputDiffusersR_[0].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0027)), 0.65f);
    inputDiffusersR_[1].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0083)), 0.60f);

    // 4 prime delay lines for dense, flutter-free diffusion
    constexpr std::array<double, 4> kBaseSec = { 0.0193, 0.0241, 0.0307, 0.0379 };
    for (size_t i = 0; i < 4; ++i) {
        const size_t len = std::max<size_t>(16, static_cast<size_t>(sampleRate * kBaseSec[i]));
        delayLines_[i].assign(len + static_cast<size_t>(sampleRate * 0.01), 0.0f);
        delayLengths_[i] = len;
        writeIndices_[i] = 0;
        dampStates_[i] = 0.0f;
    }

    reset();
}

void GatedStereoReverb::reset() noexcept {
    for (auto& diff : inputDiffusersL_) diff.reset();
    for (auto& diff : inputDiffusersR_) diff.reset();
    for (auto& line : delayLines_) std::fill(line.begin(), line.end(), 0.0f);
    writeIndices_.fill(0);
    dampStates_.fill(0.0f);
    hpfStateL_ = 0.0f;
    hpfStateR_ = 0.0f;
    sampleCounter_ = 0;
    gateEnvelope_ = 1.0f;
}

StereoFrame GatedStereoReverb::process(
    float inLeft,
    float inRight,
    float size,
    float decayMs,
    float damp,
    float gateMs,
    double sampleRate) noexcept {
    if (delayLines_[0].empty()) {
        return { inLeft, inRight };
    }

    sampleCounter_++;

    // Choke gate calculation: sharply cuts off reverb tail
    if (gateMs > 0.0f) {
        const float gateSamples = (gateMs * 0.001f) * static_cast<float>(sampleRate);
        const float releaseSamples = (15.0f * 0.001f) * static_cast<float>(sampleRate);
        if (static_cast<float>(sampleCounter_) > gateSamples) {
            const float samplesOver = static_cast<float>(sampleCounter_) - gateSamples;
            gateEnvelope_ = std::max(0.0f, 1.0f - (samplesOver / releaseSamples));
        } else {
            gateEnvelope_ = 1.0f;
        }
    } else {
        gateEnvelope_ = 1.0f;
    }

    if (gateEnvelope_ <= 0.0f) {
        return { 0.0f, 0.0f };
    }

    // Input highpass filter (220 Hz) lets warm bronze shell weight breathe without sub mud
    const float hpfAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 220.0 / sampleRate));
    hpfStateL_ += hpfAlpha * (inLeft - hpfStateL_);
    hpfStateR_ += hpfAlpha * (inRight - hpfStateR_);
    const float hpL = inLeft - hpfStateL_;
    const float hpR = inRight - hpfStateR_;

    // Smear transients through cascaded allpass diffusers
    const float diffInL = inputDiffusersL_[1].process(inputDiffusersL_[0].process(hpL));
    const float diffInR = inputDiffusersR_[1].process(inputDiffusersR_[0].process(hpR));

    // The room control changes the FDN loop lengths; feedback then follows the
    // selected decay time for each loop instead of using one fixed delay estimate.
    std::array<size_t, 4> activeLengths{};
    const float roomScale = 0.72f + 0.45f * std::clamp(size, 0.1f, 1.0f);
    for (size_t i = 0; i < activeLengths.size(); ++i) {
        activeLengths[i] = std::clamp(static_cast<size_t>(static_cast<float>(delayLengths_[i]) * roomScale),
                                      size_t{ 16 }, delayLines_[i].size());
        writeIndices_[i] %= activeLengths[i];
    }
    const float dampFactor = std::clamp(damp, 0.05f, 0.85f);

    // Read current delay outputs
    std::array<float, 4> delayOut{};
    for (size_t i = 0; i < 4; ++i) {
        delayOut[i] = delayLines_[i][writeIndices_[i]];
    }

    // Hadamard 4x4 unitary matrix mixing for maximal diffusion without coloration
    const float m0 = 0.5f * (delayOut[0] + delayOut[1] + delayOut[2] + delayOut[3]);
    const float m1 = 0.5f * (delayOut[0] - delayOut[1] + delayOut[2] - delayOut[3]);
    const float m2 = 0.5f * (delayOut[0] + delayOut[1] - delayOut[2] - delayOut[3]);
    const float m3 = 0.5f * (delayOut[0] - delayOut[1] - delayOut[2] + delayOut[3]);

    const std::array<float, 4> mixed = { m0, m1, m2, m3 };

    // Apply damping and write back into delay lines with stereo input injection
    for (size_t i = 0; i < 4; ++i) {
        dampStates_[i] = dampStates_[i] * dampFactor + mixed[i] * (1.0f - dampFactor);
        const float inSig = (i % 2 == 0) ? diffInL : diffInR;
        const float loopMs = static_cast<float>(activeLengths[i]) * 1000.0f / static_cast<float>(sampleRate);
        const float feedback = std::clamp(std::pow(0.001f, loopMs / std::max(50.0f, decayMs)), 0.12f, 0.96f);
        delayLines_[i][writeIndices_[i]] = inSig * 0.7f + dampStates_[i] * feedback;
        writeIndices_[i] = (writeIndices_[i] + 1) % activeLengths[i];
    }

    const float wetL = (delayOut[0] + delayOut[2]) * 0.7f * gateEnvelope_;
    const float wetR = (delayOut[1] + delayOut[3]) * 0.7f * gateEnvelope_;

    return { wetL, wetR };
}

// ============================================================================
// HatSynth
// ============================================================================

namespace {

[[nodiscard]] float airTailMs(const HatSynthSettings& settings) noexcept {
    const float scale = std::clamp(settings.decayScale, 0.25f, 2.0f);
    return settings.noiseAttackMs + settings.noiseHoldMs + settings.noiseDecayMs * scale;
}

[[nodiscard]] float bodyTailMs(const HatSynthSettings& settings) noexcept {
    if (settings.bodyLevel <= 0.001f) return 0.0f;
    const float scale = std::clamp(settings.decayScale, 0.25f, 2.0f);
    return settings.noiseAttackMs + settings.bodyDecayMs * scale;
}

[[nodiscard]] float dryTailDurationMs(const HatSynthSettings& settings) noexcept {
    return std::max(airTailMs(settings), bodyTailMs(settings));
}

[[nodiscard]] float tailDecayEnvelope(float age, float attackMs, float holdMs, float decayMs,
                                      float scale, float curve, double sampleRate) noexcept {
    const float sr = static_cast<float>(sampleRate);
    const float attackSamples = std::max(0.0f, attackMs) * 0.001f * sr;
    if (age < attackSamples && attackSamples > 0.0f) {
        return 0.5f * (1.0f - std::cos(pi() * age / attackSamples));
    }
    const float holdSamples = std::max(0.0f, holdMs) * 0.001f * sr;
    if (age < attackSamples + holdSamples) {
        return 1.0f;
    }
    const float decaySamples = std::max(1.0f, decayMs * scale * 0.001f * sr);
    const float decayAge = age - attackSamples - holdSamples;
    const float progress = std::clamp(decayAge / decaySamples, 0.0f, 1.0f);
    const float linearEnv = 1.0f - progress;
    const float endFade = progress > 0.92f ? std::max(0.0f, (1.0f - progress) / 0.08f) : 1.0f;
    // 0.20 of the decay length puts most of the energy up front and lets the tail
    // fall through -40 dB before the note is cut, instead of staying loud and stopping.
    const float expEnv = std::exp(-decayAge / std::max(1.0f, decaySamples * 0.20f)) * endFade;
    const float clampedCurve = std::clamp(curve, -1.0f, 1.0f);
    if (clampedCurve < 0.0f) {
        return (1.0f + clampedCurve) * linearEnv - clampedCurve * expEnv;
    }
    const float sustainedEnv = std::sqrt(std::max(0.0f, linearEnv));
    return (1.0f - clampedCurve) * linearEnv + clampedCurve * sustainedEnv;
}

[[nodiscard]] float bandLimitGain(float hz, float sampleRate) noexcept {
    return std::clamp((0.45f - hz / sampleRate) * 18.0f, 0.0f, 1.0f);
}

// Sum of sines, falling as 1/k^rolloff, each faded out before Nyquist.
[[nodiscard]] float bandLimitedPartials(float phase, float hz, float sampleRate, int maxHarmonic,
                                       int step, float rolloff) noexcept {
    float sum = 0.0f;
    float weight = 0.0f;
    for (int k = 1; k <= maxHarmonic; k += step) {
        const float freq = hz * static_cast<float>(k);
        const float gain = bandLimitGain(freq, sampleRate);
        if (gain <= 0.0f) break;
        const float amp = gain / std::pow(static_cast<float>(k), rolloff);
        sum += amp * std::sin(2.0f * pi() * phase * static_cast<float>(k));
        weight += amp;
    }
    return weight > 1.0e-6f ? sum / weight : 0.0f;
}

} // namespace

void HatSynth::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    const double safeRate = std::max(1000.0, sampleRate);
    stickHpAlpha_ = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 2200.0 / safeRate));
    const double stickLpHz = std::min(10500.0, safeRate * 0.42);
    stickLpAlpha_ = static_cast<float>(1.0 - std::exp(-2.0 * pi() * stickLpHz / safeRate));
    reverb_.prepare(sampleRate);
    const std::array<double, 2> diffuserDelaySec{{ 0.0017, 0.0023 }};
    const size_t combCap = std::max<size_t>(64, static_cast<size_t>(sampleRate * 0.025));
    std::ranges::for_each(std::views::iota(size_t{0}, channels_.size()), [&](size_t chIdx) {
        channels_[chIdx].diffuser.prepare(
            std::max<size_t>(4, static_cast<size_t>(sampleRate * diffuserDelaySec[chIdx])), 0.5f);
        channels_[chIdx].combDelay.assign(combCap, 0.0f);
    });
    reset();
}

void HatSynth::reset() noexcept {
    isPlaying_ = false;
    totalSamplesPlayed_ = 0;
    totalDurationSamples_ = 0;
    dryDurationSamples_ = 0;
    chokeStartSample_ = -1;
    chokeFadeSamples_ = 0;
    midiPitchRatio_ = 1.0f;
    fullAdsrActive_ = false;
    activeMidiNote_ = -1;
    activeMidiChannel_ = 0;
    adsrReleaseStartSample_ = -1;
    adsrReleaseSamples_ = 0;
    adsrReleaseStartLevel_ = 0.0f;

    lastNoiseHPF_ = -1.0f;
    lastNoiseLPF_ = -1.0f;
    lastBodyTone_ = -1.0f;
    lastModelType_ = -1;
    lastModelColor_ = -10.0f;
    lastModelDensity_ = -10.0f;
    chestPhase_ = 0.0f;

    std::ranges::for_each(channels_, [](ChannelState& ch) {
        ch.hpf1.reset();
        ch.hpf2.reset();
        ch.lpf1.reset();
        ch.lpf2.reset();
        std::ranges::for_each(ch.bodyModes, [](BiquadState& m) { m.reset(); });
        ch.tiltLp = 0.0f;
        ch.bodySubHp = 0.0f;
        ch.bodyWarmLp = 0.0f;
        ch.pinkB0 = ch.pinkB1 = ch.pinkB2 = 0.0f;
        ch.lofiHeld = 0.0f;
        ch.lofiCounter = 0;
        ch.metalPhases.fill(0.0f);
        ch.partialPhases.fill(0.0f);
        ch.sizzlePhases.fill(0.0f);
        std::ranges::fill(ch.combDelay, 0.0f);
        ch.combWrite = 0;
        ch.combFilter = 0.0f;
        ch.diffuser.reset();
        ch.modelBp.reset();
        ch.modelHp.reset();
        ch.shade = 1.0f;
        ch.shadeLp = 0.0f;
        ch.stickHp = 0.0f;
        ch.stickLp = 0.0f;
        ch.dcX = 0.0f;
        ch.dcY = 0.0f;
    });

    reverb_.reset();
}

void HatSynth::trigger(const HatSynthSettings& settings, float velocity) noexcept {
    start(settings, velocity);
}

void HatSynth::triggerMidi(const HatSynthSettings& settings, float velocity, int midiNoteNumber, int midiChannel) noexcept {
    const int note = std::clamp(midiNoteNumber, 0, 127);
    start(settings, velocity);
    midiPitchRatio_ = midiNoteToHz(note) / midiNoteToHz(60);
    activeMidiNote_ = note;
    activeMidiChannel_ = std::clamp(midiChannel, 1, 16);
    fullAdsrActive_ = settings.fullAdsrEnabled;
    if (fullAdsrActive_) {
        totalDurationSamples_ = std::numeric_limits<int>::max();
        adsrReleaseStartLevel_ = fullAdsrEnvelopeAt(0, settings);
    }
}

float HatSynth::fullAdsrEnvelopeAt(int sample, const HatSynthSettings& settings) const noexcept {
    if (adsrReleaseStartSample_ >= 0) {
        if (adsrReleaseSamples_ <= 0) return 0.0f;
        const float releaseProgress = static_cast<float>(sample - adsrReleaseStartSample_)
            / static_cast<float>(adsrReleaseSamples_);
        return adsrReleaseStartLevel_ * std::clamp(1.0f - releaseProgress, 0.0f, 1.0f);
    }

    const float attackSamples = std::clamp(settings.ampAttackMs, 0.05f, 20.0f)
        * 0.001f * static_cast<float>(sampleRate_);
    const float decaySamples = std::clamp(settings.ampDecayMs, 1.0f, 600.0f)
        * 0.001f * static_cast<float>(sampleRate_);
    const float sustain = std::clamp(settings.ampSustain, 0.0f, 1.0f);
    if (static_cast<float>(sample) < attackSamples)
        return static_cast<float>(sample) / std::max(1.0f, attackSamples);
    if (static_cast<float>(sample) < attackSamples + decaySamples) {
        const float progress = (static_cast<float>(sample) - attackSamples) / std::max(1.0f, decaySamples);
        return 1.0f + (sustain - 1.0f) * progress;
    }
    return sustain;
}

void HatSynth::noteOff(int midiNoteNumber, int midiChannel, const HatSynthSettings& settings) noexcept {
    if (!fullAdsrActive_ || adsrReleaseStartSample_ >= 0
        || activeMidiNote_ != midiNoteNumber || activeMidiChannel_ != midiChannel)
        return;

    const int releaseStartSample = totalSamplesPlayed_;
    adsrReleaseStartLevel_ = fullAdsrEnvelopeAt(releaseStartSample, settings);
    adsrReleaseStartSample_ = releaseStartSample;
    const float releaseMs = std::clamp(settings.ampReleaseMs, 0.0f, 20000.0f);
    adsrReleaseSamples_ = releaseMs <= 0.0f
        ? 0
        : std::max(1, static_cast<int>(releaseMs * 0.001f * static_cast<float>(sampleRate_)));
    const float dryTailMs = dryTailDurationMs(settings);
    const float effectTailMs = std::max({ settings.reverbDecayMs, settings.reverbGateMs, dryTailMs });
    const int tailSamples = static_cast<int>((effectTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_));
    totalDurationSamples_ = std::max(totalSamplesPlayed_ + 1,
        adsrReleaseStartSample_ + adsrReleaseSamples_ + tailSamples);
}

void HatSynth::start(const HatSynthSettings& settings, float velocity) noexcept {
    reset();
    // Scatter oscillator phase so the partials don't start as one stacked click.
    std::ranges::for_each(channels_, [&](ChannelState& ch) {
        const auto scramble = [&](auto& phases) {
            for (float& phase : phases)
                phase = 0.5f * (nextRandomFloat(prngState_) + 1.0f);
        };
        scramble(ch.metalPhases);
        scramble(ch.partialPhases);
        scramble(ch.sizzlePhases);
        ch.shade = 1.0f;
    });
    isPlaying_ = true;
    chokeStartSample_ = -1;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);

    preChickCount_ = std::clamp(settings.preChickCount, 1, 5);
    const float spreadSamples = std::max(0.0f, settings.preChickSpreadMs) * 0.001f * static_cast<float>(sampleRate_);
    std::ranges::for_each(std::views::iota(0, preChickCount_), [&](int i) {
        const float jitterTime = nextRandomFloat(prngState_) * 0.5f * settings.preChickJitter * spreadSamples;
        burstStartSamples_[i] = std::max(0, static_cast<int>(static_cast<float>(i) * spreadSamples + jitterTime));
        const float progression = preChickCount_ > 1
            ? 0.65f + 0.35f * static_cast<float>(i) / static_cast<float>(preChickCount_ - 1)
            : 1.0f;
        const float jitterAmplitude = 1.0f + nextRandomFloat(prngState_) * 0.15f * settings.preChickJitter;
        burstAmplitudes_[i] = std::clamp(settings.preChickLevel * progression * jitterAmplitude, 0.05f, 1.0f);
        const float panDirection = (i % 2 == 0) ? -1.0f : 1.0f;
        const float panFraction = preChickCount_ > 1 ? static_cast<float>(i + 1) / static_cast<float>(preChickCount_) : 0.4f;
        burstPans_[i] = std::clamp(panDirection * settings.preChickFlamSpread * panFraction, -1.0f, 1.0f);
    });

    mainHitSample_ = preChickCount_ > 1
        ? static_cast<int>(static_cast<float>(preChickCount_ - 1) * spreadSamples)
        : 0;

    const float preRollMs = static_cast<float>(mainHitSample_) * 1000.0f / static_cast<float>(sampleRate_);
    const float dryTailMs = dryTailDurationMs(settings);
    const float effectTailMs = std::max({settings.reverbDecayMs, settings.reverbGateMs, dryTailMs});
    dryDurationSamples_ = std::max(1, static_cast<int>((preRollMs + dryTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
    totalDurationSamples_ = std::max(dryDurationSamples_,
        static_cast<int>((preRollMs + effectTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
}

void HatSynth::updateFilterCaches(const HatSynthSettings& settings) noexcept {
    if (std::abs(settings.noiseHPF - lastNoiseHPF_) > 1.0f) {
        lastNoiseHPF_ = settings.noiseHPF;
        nh1Coeffs_ = computeHighpassCoeffs(lastNoiseHPF_, 0.54f, sampleRate_);
        nh2Coeffs_ = computeHighpassCoeffs(lastNoiseHPF_, 1.30f, sampleRate_);
    }
    if (std::abs(settings.noiseLPF - lastNoiseLPF_) > 1.0f) {
        lastNoiseLPF_ = settings.noiseLPF;
        nl1Coeffs_ = computeLowpassCoeffs(lastNoiseLPF_, 0.54f, sampleRate_);
        nl2Coeffs_ = computeLowpassCoeffs(lastNoiseLPF_, 1.30f, sampleRate_);
    }
    const float clampedTone = std::clamp(settings.bodyTone, 0.0f, 1.0f);
    if (std::abs(clampedTone - lastBodyTone_) > 0.002f) {
        lastBodyTone_ = clampedTone;
        // A hat shell lives around 0.9–5 kHz. The old 330 Hz base was a tom.
        constexpr std::array<float, 4> kRatios{{ 1.00f, 1.48f, 2.11f, 2.89f }};
        constexpr std::array<float, 4> kModeQs{{ 2.15f, 2.55f, 3.05f, 3.40f }};
        const float baseHz = 880.0f * std::exp2(clampedTone * 1.05f);
        std::ranges::for_each(std::views::iota(size_t{0}, size_t{2}), [&](size_t chIdx) {
            const float stereoDetune = chIdx == 0 ? 0.994f : 1.006f;
            std::ranges::for_each(std::views::iota(size_t{0}, size_t{4}), [&](size_t modeIdx) {
                bodyModeCoeffs_[chIdx][modeIdx] = computeBandpassCoeffs(
                    baseHz * kRatios[modeIdx] * stereoDetune, kModeQs[modeIdx], sampleRate_);
            });
        });
    }

    const int modelType = static_cast<int>(settings.noiseType);
    const float modelColor = std::clamp(settings.hissColor, -1.0f, 1.0f);
    const float modelDensity = std::clamp(settings.hissDensity, 0.0f, 1.0f);
    if (modelType != lastModelType_ || std::abs(modelColor - lastModelColor_) > 0.01f
        || std::abs(modelDensity - lastModelDensity_) > 0.01f) {
        lastModelType_ = modelType;
        lastModelColor_ = modelColor;
        lastModelDensity_ = modelDensity;
        float bpHz = 6000.0f;
        float bpQ = 0.8f;
        float hpHz = 2500.0f;
        float hpQ = 0.7f;
        switch (settings.noiseType) {
            case NoiseType::CrispWhite:
                bpHz = 7200.0f * std::exp2(modelColor * 0.40f);
                bpQ = 1.25f + 1.9f * modelDensity;
                hpHz = std::clamp(bpHz * 0.38f, 1400.0f, 5200.0f);
                hpQ = 0.62f;
                break;
            case NoiseType::PinkSilk:
                bpHz = 4600.0f * std::exp2(modelColor * 0.32f);
                bpQ = 0.75f + 0.55f * modelDensity;
                hpHz = 1800.0f * std::exp2(std::max(0.0f, modelColor) * 0.40f);
                hpQ = 0.55f;
                break;
            case NoiseType::VelvetShimmer:
                hpHz = 3000.0f * std::exp2(modelColor * 0.25f);
                hpQ = 0.60f;
                bpHz = 8500.0f;
                bpQ = 0.70f;
                break;
            case NoiseType::LoFiCrunch:
                hpHz = 2800.0f * std::exp2(modelColor * 0.35f);
                hpQ = 0.70f;
                bpHz = 5200.0f;
                bpQ = 0.80f;
                break;
            case NoiseType::AnalogMetal:
                hpHz = 3400.0f * std::exp2(modelColor * 0.28f);
                hpQ = 0.75f;
                bpHz = 7800.0f * std::exp2(modelColor * 0.22f);
                bpQ = 1.15f;
                break;
            case NoiseType::BronzeCymbal:
                hpHz = 3400.0f * std::exp2(modelColor * 0.35f);
                hpQ = 0.60f;
                bpHz = 6200.0f;
                bpQ = 0.70f;
                break;
            case NoiseType::SupersawSizzle:
                hpHz = 5400.0f * std::exp2(modelColor * 0.28f);
                hpQ = 0.65f;
                bpHz = 8600.0f;
                bpQ = 0.80f;
                break;
            case NoiseType::CombWash:
                hpHz = 2000.0f * std::exp2(modelColor * 0.30f);
                hpQ = 0.70f;
                bpHz = 6400.0f;
                bpQ = 0.60f;
                break;
        }
        modelBpCoeffs_ = computeBandpassCoeffs(bpHz, bpQ, sampleRate_);
        modelHpCoeffs_ = computeHighpassCoeffs(hpHz, hpQ, sampleRate_);
    }
}

float HatSynth::generateHissChannel(ChannelState& ch, int channelIdx, const HatSynthSettings& settings) noexcept {
    const float white = nextRandomFloat(prngState_);
    ch.pinkB0 = 0.99765f * ch.pinkB0 + white * 0.0990460f;
    ch.pinkB1 = 0.96300f * ch.pinkB1 + white * 0.2965164f;
    ch.pinkB2 = 0.57000f * ch.pinkB2 + white * 1.0526913f;
    const float pink = (ch.pinkB0 + ch.pinkB1 + ch.pinkB2 + white * 0.1848f) * 0.35f;

    const float sr = static_cast<float>(sampleRate_);
    const float color = std::clamp(settings.hissColor, -1.0f, 1.0f);
    const float density = std::clamp(settings.hissDensity, 0.0f, 1.0f);
    const float chDetune = channelIdx == 0 ? 0.997f : 1.003f;
    if (!fullAdsrActive_) {
        const float tau = 0.15f + 0.10f * (1.0f - density);
        ch.shade *= std::exp(-1.0f / (sr * tau));
    }

    // Inharmonic stack. A closing lowpass after this function darkens the hit.
    const auto modalStack = [&](float baseHz, std::span<const float> ratios, float tilt, float level) {
        std::array<float, 24> amps{};
        const size_t count = std::min(ratios.size(), amps.size());
        float sumSq = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            const float pos = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.0f;
            const float amp = std::exp(-tilt * pos);
            amps[i] = amp;
            sumSq += amp * amp;
        }
        const float norm = sumSq > 1.0e-8f ? level / std::sqrt(sumSq) : 0.0f;
        float sum = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            const float hz = baseHz * ratios[i];
            const float gain = bandLimitGain(hz, sr);
            if (gain <= 0.0f || amps[i] <= 0.0f) continue;
            const float phase = advancePhase(ch.partialPhases[i], hz, sampleRate_);
            const size_t modIdx = (i + 5) % count;
            const float fm = (0.04f + 0.16f * density) * std::sin(2.0f * pi() * ch.partialPhases[modIdx]);
            sum += gain * amps[i] * std::sin(2.0f * pi() * phase + fm);
        }
        return sum * norm;
    };

    switch (settings.noiseType) {
        case NoiseType::CrispWhite: {
            // 909-style: noise through a resonant peak plus a softer highpassed bed.
            const float bp = processBiquad(white, ch.modelBp, modelBpCoeffs_);
            const float hp = processBiquad(white, ch.modelHp, modelHpCoeffs_);
            return bp * 1.05f + hp * 0.42f;
        }
        case NoiseType::PinkSilk: {
            const float bed = 0.72f * pink + 0.28f * white;
            const float bp = processBiquad(bed, ch.modelBp, modelBpCoeffs_);
            const float hp = processBiquad(pink, ch.modelHp, modelHpCoeffs_);
            return bp * 0.72f + hp * 0.28f + pink * 0.06f;
        }
        case NoiseType::VelvetShimmer: {
            constexpr std::array<float, 12> kRatios{{
                1.00f, 1.17f, 1.36f, 1.58f, 1.83f, 2.12f, 2.46f, 2.85f, 3.31f, 3.84f, 4.45f, 5.16f
            }};
            const float base = 2800.0f * std::exp2(color * 0.34f) * chDetune;
            float sum = 0.0f;
            float sumSq = 0.0f;
            std::array<float, 12> amps{};
            for (size_t i = 0; i < kRatios.size(); ++i) {
                const float pos = static_cast<float>(i) / 11.0f;
                amps[i] = std::exp(-0.42f * pos);
                sumSq += amps[i] * amps[i];
            }
            const float norm = 0.34f / std::sqrt(sumSq);
            for (size_t i = 0; i < kRatios.size(); ++i) {
                const float hz = base * kRatios[i];
                const float pairHz = hz * (1.0045f + 0.0075f * density);
                const float a = bandLimitGain(hz, sr) * std::sin(2.0f * pi() * advancePhase(ch.partialPhases[i], hz, sampleRate_));
                const float b = bandLimitGain(pairHz, sr) * std::sin(2.0f * pi() * advancePhase(ch.partialPhases[i + 12], pairHz, sampleRate_));
                sum += (a + 0.72f * b) * amps[i];
            }
            const float air = processBiquad(white, ch.modelHp, modelHpCoeffs_);
            return sum * norm + air * 0.10f;
        }
        case NoiseType::LoFiCrunch: {
            const float bright = processBiquad(white, ch.modelHp, modelHpCoeffs_);
            const int holdSteps = 2 + static_cast<int>(density * 3.0f);
            const float levels = std::max(8.0f, 26.0f - density * 14.0f);
            if (ch.lofiCounter <= 0) {
                ch.lofiHeld = std::round(bright * levels) / levels;
                ch.lofiCounter = holdSteps;
            }
            --ch.lofiCounter;
            const float crushed = ch.lofiHeld + (0.18f + 0.12f * color) * (bright - ch.lofiHeld);
            return std::tanh(crushed * (1.6f + 0.9f * density)) * 0.58f;
        }
        case NoiseType::AnalogMetal: {
            // Six band-limited squares at the 808 metal ratios, ring-modulated in pairs,
            // then highpassed. Sine ring-mod of these lows never reached the hat band.
            constexpr std::array<float, 6> kBaseHz{{ 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f }};
            const float pitchScale = std::exp2(color * 0.42f) * chDetune;
            std::array<float, 6> sq{};
            for (size_t idx = 0; idx < sq.size(); ++idx) {
                const float hz = kBaseHz[idx] * pitchScale;
                const float phase = advancePhase(ch.metalPhases[idx], hz, sampleRate_);
                sq[idx] = bandLimitedPartials(phase, hz, sr, 11, 2, 1.0f);
            }
            const float metal = (sq[0] * sq[1] + sq[2] * sq[3] + sq[4] * sq[5]) * (0.85f + 0.25f * density);
            const float shaped = std::tanh(metal * (1.15f + 0.55f * density));
            const float hp = processBiquad(shaped, ch.modelHp, modelHpCoeffs_);
            const float grit = processBiquad(white, ch.modelBp, modelBpCoeffs_);
            // Bright noise following the metal keeps the 808 noisy instead of a low buzz.
            const float follow = 0.28f + 0.72f * std::min(1.0f, std::abs(shaped) * 1.6f);
            return hp * 2.35f + grit * 0.72f * follow;
        }
        case NoiseType::BronzeCymbal: {
            constexpr std::array<float, 14> kRatios{{
                1.00f, 1.46f, 1.97f, 2.58f, 3.29f, 4.12f, 5.08f, 6.19f,
                7.47f, 8.93f, 10.58f, 12.44f, 14.52f, 16.85f
            }};
            const float base = 780.0f * std::exp2(color * 0.40f) * chDetune;
            const float wash = processBiquad(0.75f * white + 0.25f * pink, ch.modelHp, modelHpCoeffs_);
            return modalStack(base, kRatios, 0.45f, 0.40f) + wash * (0.12f + 0.08f * density);
        }
        case NoiseType::SupersawSizzle: {
            // A few band-limited triangles in the sizzle band, plus bright noise.
            // Naive saws at 8 kHz were mostly alias.
            constexpr std::array<float, 5> kHz{{ 1880.0f, 2640.0f, 3520.0f, 4680.0f, 6120.0f }};
            const float pitchScale = std::exp2(color * 0.32f);
            const float spread = channelIdx == 0 ? -1.0f : 1.0f;
            float sum = 0.0f;
            for (size_t idx = 0; idx < kHz.size(); ++idx) {
                const float detune = 1.0f + spread * (0.003f + 0.012f * density) * static_cast<float>(idx + 1) / 5.0f;
                const float hz = kHz[idx] * pitchScale * detune;
                const float phase = advancePhase(ch.sizzlePhases[idx], hz, sampleRate_);
                const float tri = bandLimitedPartials(phase, hz, sr, 5, 2, 2.0f);
                sum += bandLimitGain(hz, sr) * tri * (1.0f - 0.12f * static_cast<float>(idx));
            }
            const float air = processBiquad(white, ch.modelHp, modelHpCoeffs_);
            return sum * 0.20f + air * 0.62f;
        }
        case NoiseType::CombWash: {
            const float input = 0.70f * white + 0.30f * pink;
            if (ch.combDelay.empty()) return input;
            const float baseMs = (channelIdx == 0 ? 4.15f : 5.55f) * (1.12f - 0.38f * color);
            const auto tap = [&](float delayMs) {
                const size_t delaySamples = std::clamp<size_t>(
                    static_cast<size_t>(std::max(0.2f, delayMs) * 0.001f * sr), size_t{1}, ch.combDelay.size() - 1);
                const size_t readIdx = (ch.combWrite + ch.combDelay.size() - delaySamples) % ch.combDelay.size();
                return ch.combDelay[readIdx];
            };
            const float delayed = 0.64f * tap(baseMs) + 0.36f * tap(baseMs * 0.67f);
            const float feedback = 0.34f + 0.36f * density;
            ch.combDelay[ch.combWrite] = std::tanh(input + delayed * feedback);
            ch.combWrite = (ch.combWrite + 1) % ch.combDelay.size();
            const float mixed = 0.16f * input + 1.05f * delayed;
            return processBiquad(mixed, ch.modelHp, modelHpCoeffs_) * 0.95f;
        }
    }
    return white;
}

float HatSynth::processBodyChannel(ChannelState& ch, int channelIdx, float excitation, float thump) noexcept {
    constexpr std::array<float, 4> kModeWeights{{ 0.38f, 0.28f, 0.20f, 0.14f }};
    float modes = 0.0f;
    std::ranges::for_each(std::views::iota(size_t{0}, size_t{4}), [&](size_t modeIdx) {
        modes += processBiquad(excitation, ch.bodyModes[modeIdx], bodyModeCoeffs_[channelIdx][modeIdx])
            * kModeWeights[modeIdx];
    });
    const float subAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 340.0 / sampleRate_));
    ch.bodySubHp += subAlpha * (excitation - ch.bodySubHp);
    const float chest = excitation - ch.bodySubHp;
    const float warmAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 3400.0 / sampleRate_));
    ch.bodyWarmLp += warmAlpha * (chest - ch.bodyWarmLp);
    return modes * 1.85f + ch.bodyWarmLp * 0.42f + thump * 0.22f;
}

float HatSynth::generateNoiseSample(NoiseType type) noexcept {
    HatSynthSettings singleSettings;
    singleSettings.noiseType = type;
    return generateHissChannel(channels_[0], 0, singleSettings);
}

float HatSynth::generateChickSample(ChickType type, float freqHz, int ageSamples, float level) noexcept {
    if (ageSamples < 0) return 0.0f;
    const float t = static_cast<float>(ageSamples) / static_cast<float>(sampleRate_);
    const float sampleRate = static_cast<float>(sampleRate_);
    // Fade each partial across the top tenth of the band before it can fold back.
    const auto bandGain = [sampleRate](float hz) {
        return std::clamp((0.5f - hz / sampleRate) * 20.0f, 0.0f, 1.0f);
    };
    const auto partial = [&](float ratio) {
        const float hz = freqHz * ratio;
        const float gain = bandGain(hz);
        return gain > 0.0f ? gain * std::sin(2.0f * pi() * hz * t) : 0.0f;
    };
    switch (type) {
        case ChickType::CrispStick: {
            if (t > 0.008f) return 0.0f;
            const float env = std::exp(-t / 0.0014f);
            // The broadband spike has to be gone before 0.2 ms; the tone is the stick.
            const float click = std::max(0.0f, 1.0f - t * 7200.0f);
            return (click * 0.42f
                + (partial(1.0f) * 0.52f + partial(1.51f) * 0.30f + partial(2.29f) * 0.16f + partial(3.17f) * 0.08f) * env)
                * level;
        }
        case ChickType::PedalFoot: {
            if (t > 0.020f) return 0.0f;
            const float env = std::exp(-t / 0.0036f);
            return (partial(0.47f) * 0.62f + partial(0.81f) * 0.42f + partial(1.23f) * 0.26f + partial(1.84f) * 0.12f)
                * env * level * 1.15f;
        }
        case ChickType::LaserTick: {
            if (t > 0.010f) return 0.0f;
            constexpr float sweepTime = 0.0015f;
            const float env = std::exp(-t / sweepTime);
            const float gain = bandGain(freqHz * (1.0f + 2.8f * env));
            if (gain <= 0.0f) return 0.0f;
            // Integrate the exponential frequency sweep to keep its pitch above the landing note.
            const float cycles = freqHz * (t + 2.8f * sweepTime * (1.0f - env));
            return std::sin(2.0f * pi() * cycles) * gain * env * level * 1.25f;
        }
    }
    return 0.0f;
}

float HatSynth::getEnvelopeProgress() const noexcept {
    if (!isPlaying_ || totalDurationSamples_ <= 0) return 1.0f;
    return std::clamp(static_cast<float>(totalSamplesPlayed_) / static_cast<float>(totalDurationSamples_), 0.0f, 1.0f);
}

StereoFrame HatSynth::processSample(const HatSynthSettings& settings) noexcept {
    return processSampleLayers(settings).output;
}

void HatSynth::choke() noexcept {
    if (!isPlaying_ || chokeStartSample_ >= 0)
        return;
    chokeStartSample_ = totalSamplesPlayed_;
    chokeFadeSamples_ = std::max(1, static_cast<int>(sampleRate_ * 0.005));
}

namespace {

[[nodiscard]] float oneShotAmplitudeEnvelope(int currentSample, int dryDurationSamples, double sampleRate,
                                            const HatSynthSettings& settings) noexcept {
    const float dryDuration = static_cast<float>(dryDurationSamples);
    const float sr = static_cast<float>(sampleRate);
    const float attackSamples = std::clamp(settings.ampAttackMs, 0.05f, 20.0f) * 0.001f * sr;
    const float releaseSamples = std::clamp(settings.ampReleaseMs, 5.0f, 800.0f) * 0.001f * sr;
    const float safeAttack = std::min(attackSamples, dryDuration * 0.30f);
    const float safeRelease = std::min(releaseSamples, dryDuration * 0.35f);
    const float decaySamples = std::clamp(settings.ampDecayMs, 1.0f, 600.0f) * 0.001f * sr;
    const float safeDecay = std::min(decaySamples, std::max(0.0f, dryDuration - safeAttack - safeRelease));
    const float sustain = std::clamp(settings.ampSustain, 0.0f, 1.0f);
    const float releaseStart = std::max(safeAttack + safeDecay, dryDuration - safeRelease);
    const float pos = static_cast<float>(currentSample);

    if (pos < safeAttack) {
        return pos / std::max(1.0f, safeAttack);
    }
    if (pos < safeAttack + safeDecay) {
        const float phase = (pos - safeAttack) / std::max(1.0f, safeDecay);
        return 1.0f + (sustain - 1.0f) * phase;
    }
    if (pos < releaseStart) {
        return sustain;
    }
    return sustain * std::max(0.0f, dryDuration - pos) / std::max(1.0f, dryDuration - releaseStart);
}

} // namespace

HatSynthLayers HatSynth::processSampleLayers(const HatSynthSettings& settings) noexcept {
    if (!isPlaying_) return {};

    const int currentSample = totalSamplesPlayed_++;
    if (totalSamplesPlayed_ >= totalDurationSamples_) {
        isPlaying_ = false;
        return {};
    }

    updateFilterCaches(settings);

    std::array<float, 2> preChickGain{};
    std::array<float, 2> preChickTick{};
    const float decayScale = std::clamp(settings.decayScale, 0.25f, 2.0f);
    const float sr = static_cast<float>(sampleRate_);
    const float burstDecaySamples = std::max(1.0f, settings.preChickDecayMs * decayScale * 0.001f * sr);
    const float mainAge = static_cast<float>(currentSample - mainHitSample_);

    std::ranges::for_each(std::views::iota(0, preChickCount_), [&](int i) {
        const int age = currentSample - burstStartSamples_[i];
        if (age < 0 || static_cast<float>(age) >= burstDecaySamples * 4.0f) return;
        const float envelope = std::exp(-static_cast<float>(age) / burstDecaySamples) * burstAmplitudes_[i];
        const float pan = burstPans_[i];
        const float gainL = std::sqrt(std::clamp(0.5f * (1.0f - pan), 0.0f, 1.0f));
        const float gainR = std::sqrt(std::clamp(0.5f * (1.0f + pan), 0.0f, 1.0f));
        preChickGain[0] += envelope * gainL;
        preChickGain[1] += envelope * gainR;
        const float tick = generateChickSample(settings.chickType, settings.chickFreqHz * midiPitchRatio_,
                                               age, burstAmplitudes_[i] * 0.55f);
        preChickTick[0] += tick * gainL;
        preChickTick[1] += tick * gainR;
    });

    float noiseEnvelope = 0.0f;
    float bodyEnvelope = 0.0f;
    float chick = 0.0f;
    float chestThump = 0.0f;
    if (mainAge >= 0.0f) {
        noiseEnvelope = fullAdsrActive_
            ? 1.0f
            : tailDecayEnvelope(mainAge, settings.noiseAttackMs, settings.noiseHoldMs, settings.noiseDecayMs,
                                decayScale, settings.noiseCurve, sampleRate_);
        bodyEnvelope = fullAdsrActive_
            ? 1.0f
            : tailDecayEnvelope(mainAge, settings.noiseAttackMs, std::min(settings.noiseHoldMs, 10.0f),
                                settings.bodyDecayMs, decayScale, -0.35f, sampleRate_);
        chick = generateChickSample(settings.chickType, settings.chickFreqHz * midiPitchRatio_,
                                    static_cast<int>(mainAge), settings.chickLevel);
        const float thumpTime = mainAge / sr;
        if (thumpTime < 0.009f && settings.bodyLevel > 0.001f) {
            const float thumpEnv = std::exp(-thumpTime / 0.0024f);
            const float thumpHz = 740.0f + 640.0f * std::clamp(settings.bodyTone, 0.0f, 1.0f);
            chestThump = std::sin(2.0f * pi() * advancePhase(chestPhase_, thumpHz, sampleRate_)) * thumpEnv;
        }
    }

    const float attackEnvelope = mainAge >= 0.0f
        ? std::exp(-mainAge / std::max(1.0f, sr * 0.0018f))
        : 0.0f;
    const float attackLevel = std::clamp(settings.chickLevel, 0.0f, 1.0f) * 1.15f;
    const float hissColor = std::clamp(settings.hissColor, -1.0f, 1.0f);
    const float tiltAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 4200.0 / sampleRate_));
    const bool useDiffuser = settings.noiseType == NoiseType::PinkSilk || settings.noiseType == NoiseType::CombWash;
    const float bodyMix = std::clamp(settings.bodyLevel, 0.0f, 1.0f);

    std::array<float, 2> airPair{};
    std::array<float, 2> unscaledChickPair{};

    // Process independent left/right channels uniformly through the same synthesis helper chain.
    std::ranges::for_each(std::views::iota(size_t{0}, size_t{2}), [&](size_t chIdx) {
        auto& ch = channels_[chIdx];
        const float rawTone = generateHissChannel(ch, static_cast<int>(chIdx), settings);
        // The lowpass closes over the note, the way a real cymbal loses its top.
        const float shadeNow = std::clamp(ch.shade, 0.0f, 1.0f);
        const float cutoff = std::min(1700.0f + 16000.0f * shadeNow, static_cast<float>(sampleRate_) * 0.45f);
        const float darkAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * cutoff / sampleRate_));
        ch.shadeLp += darkAlpha * (rawTone - ch.shadeLp);
        const float rawHiss = ch.shadeLp;
        float stickNoise = nextRandomFloat(prngState_);
        ch.stickHp += stickHpAlpha_ * (stickNoise - ch.stickHp);
        stickNoise -= ch.stickHp;
        ch.stickLp += stickLpAlpha_ * (stickNoise - ch.stickLp);
        stickNoise = ch.stickLp;
        const float noiseSwell = rawHiss * preChickGain[chIdx] * 0.28f + rawHiss * noiseEnvelope * settings.noiseLevel;

        // The air & bronze body layer is unpitched, following filter & tone controls instead of MIDI pitch.
        float air = processBiquad(noiseSwell, ch.hpf1, nh1Coeffs_);
        air = processBiquad(air, ch.hpf2, nh2Coeffs_);
        air = processBiquad(air, ch.lpf1, nl1Coeffs_);
        air = processBiquad(air, ch.lpf2, nl2Coeffs_);

        ch.tiltLp += tiltAlpha * (air - ch.tiltLp);
        const float airHigh = air - ch.tiltLp;
        air = hissColor >= 0.0f
            ? air + airHigh * (0.45f * hissColor)
            : airHigh * (1.0f + 0.45f * hissColor) + ch.tiltLp * (1.0f - 0.35f * hissColor);

        if (useDiffuser) {
            air = ch.diffuser.process(air);
        }

        const float warmPink = (ch.pinkB0 + ch.pinkB1) * 0.35f;
        const float bodyExcite = (warmPink + rawHiss * 0.30f) * (preChickGain[chIdx] * 0.30f + bodyEnvelope * 0.80f)
            + attackEnvelope * 0.45f;
        const float bronzeBody = bodyMix > 0.001f
            ? processBodyChannel(ch, static_cast<int>(chIdx), bodyExcite, chestThump * (bodyEnvelope > 0.0f ? 1.0f : 0.0f)) * bodyMix
            : 0.0f;

        airPair[chIdx] = dcBlock(softSaturate(air + bronzeBody, settings.hissDrive), ch.dcX, ch.dcY);
        unscaledChickPair[chIdx] = chick + preChickTick[chIdx] + stickNoise * attackEnvelope * attackLevel;
    });

    if (settings.noiseStereoWidth != 1.0f) {
        const float mid = (airPair[0] + airPair[1]) * 0.5f;
        const float side = (airPair[0] - airPair[1]) * 0.5f * settings.noiseStereoWidth;
        airPair[0] = mid + side;
        airPair[1] = mid - side;
    }

    const float airVelocity = std::pow(velocity_, std::clamp(settings.velocityAir, 0.25f, 4.0f));
    const float chickVelocity = std::pow(velocity_, std::clamp(settings.velocityChick, 0.25f, 4.0f));
    const float reverbVelocity = std::pow(velocity_, std::clamp(settings.velocityReverb, 0.25f, 4.0f));
    const float unscaledAirL = airPair[0];
    const float unscaledAirR = airPair[1];
    const float airL = unscaledAirL * airVelocity;
    const float airR = unscaledAirR * airVelocity;

    const float amplitudeEnvelope = fullAdsrActive_
        ? fullAdsrEnvelopeAt(currentSample, settings)
        : oneShotAmplitudeEnvelope(currentSample, dryDurationSamples_, sampleRate_, settings);

    const float chickL = unscaledChickPair[0] * chickVelocity;
    const float chickR = unscaledChickPair[1] * chickVelocity;
    const float reverbSendL = (unscaledAirL + unscaledChickPair[0]) * reverbVelocity * amplitudeEnvelope;
    const float reverbSendR = (unscaledAirR + unscaledChickPair[1]) * reverbVelocity * amplitudeEnvelope;
    StereoFrame reverbOut{0.0f, 0.0f};
    if (settings.reverbMix > 0.001f) {
        reverbOut = reverb_.process(reverbSendL, reverbSendR, settings.reverbSize, settings.reverbDecayMs,
                                    settings.reverbDamp, settings.reverbGateMs, sampleRate_);
    }
    const float dryMix = 1.0f - settings.reverbMix * 0.40f;
    HatSynthLayers layers {
        .air = { airL * amplitudeEnvelope * dryMix, airR * amplitudeEnvelope * dryMix },
        .chick = { chickL * amplitudeEnvelope * dryMix, chickR * amplitudeEnvelope * dryMix },
        .reverb = { reverbOut.left * settings.reverbMix,
                    reverbOut.right * settings.reverbMix }
    };
    layers.output = {
        layers.air.left + layers.chick.left + layers.reverb.left,
        layers.air.right + layers.chick.right + layers.reverb.right
    };

    if (chokeStartSample_ >= 0) {
        const int chokeAge = currentSample - chokeStartSample_;
        const float chokeGain = std::clamp(1.0f - static_cast<float>(chokeAge) / static_cast<float>(chokeFadeSamples_),
                                           0.0f, 1.0f);
        const auto applyChokeGain = [chokeGain](StereoFrame& frame) {
            frame.left *= chokeGain;
            frame.right *= chokeGain;
        };
        applyChokeGain(layers.air);
        applyChokeGain(layers.chick);
        applyChokeGain(layers.reverb);
        applyChokeGain(layers.output);
        if (chokeAge >= chokeFadeSamples_) {
            isPlaying_ = false;
            reverb_.reset();
            chokeStartSample_ = -1;
            return {};
        }
    }
    return layers;
}

void HatSynth::renderBlock(
    std::vector<float>& leftOut,
    std::vector<float>& rightOut,
    const HatSynthSettings& settings,
    float velocity) noexcept {
    trigger(settings, velocity);
    leftOut.resize(totalDurationSamples_);
    rightOut.resize(totalDurationSamples_);
    std::ranges::for_each(std::views::iota(0, totalDurationSamples_), [&](int i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    });
}

void HatSynth::renderMidiBlock(
    std::vector<float>& leftOut,
    std::vector<float>& rightOut,
    const HatSynthSettings& settings,
    float velocity,
    int midiNoteNumber) noexcept {
    auto oneShotSettings = settings;
    oneShotSettings.fullAdsrEnabled = false;
    triggerMidi(oneShotSettings, velocity, midiNoteNumber);
    leftOut.resize(totalDurationSamples_);
    rightOut.resize(totalDurationSamples_);
    std::ranges::for_each(std::views::iota(0, totalDurationSamples_), [&](int i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    });
}

int HatSynth::renderMidiBlock(
    std::span<float> leftOut,
    std::span<float> rightOut,
    const HatSynthSettings& settings,
    float velocity,
    int midiNoteNumber) noexcept {
    auto oneShotSettings = settings;
    oneShotSettings.fullAdsrEnabled = false;
    triggerMidi(oneShotSettings, velocity, midiNoteNumber);
    const int renderCount = std::min({ totalDurationSamples_, static_cast<int>(leftOut.size()),
                                       static_cast<int>(rightOut.size()) });
    std::ranges::for_each(std::views::iota(0, renderCount), [&](int i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    });
    return renderCount;
}

} // namespace openhatter::core
