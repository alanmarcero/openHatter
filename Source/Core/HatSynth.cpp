#include "HatSynth.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace openhatter::core {

namespace {

[[nodiscard]] constexpr float pi() noexcept {
    return std::numbers::pi_v<float>;
}

// High-speed 32-bit xoroshiro / xorshift PRNG for pristine noise without repetitive periodicity
[[nodiscard]] inline float nextRandomFloat(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<float>(state) * (2.0f / 4294967296.0f)) - 1.0f;
}

// Biquad highpass filter coefficients (12 dB/octave)
void computeHighpassCoeffs(float freqHz, float q, double sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.49f;
    const float clampedFreq = std::clamp(freqHz, 10.0f, nyquist);
    const float clampedQ = std::clamp(q, 0.1f, 10.0f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * clampedQ);

    const float a0 = 1.0f + alpha;
    b0 = ((1.0f + cosW0) * 0.5f) / a0;
    b1 = (-(1.0f + cosW0)) / a0;
    b2 = ((1.0f + cosW0) * 0.5f) / a0;
    a1 = (-2.0f * cosW0) / a0;
    a2 = (1.0f - alpha) / a0;
}

// Biquad lowpass filter coefficients (12 dB/octave)
void computeLowpassCoeffs(float freqHz, float q, double sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.49f;
    const float clampedFreq = std::clamp(freqHz, 20.0f, nyquist);
    const float clampedQ = std::clamp(q, 0.1f, 10.0f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * clampedQ);

    const float a0 = 1.0f + alpha;
    b0 = ((1.0f - cosW0) * 0.5f) / a0;
    b1 = (1.0f - cosW0) / a0;
    b2 = ((1.0f - cosW0) * 0.5f) / a0;
    a1 = (-2.0f * cosW0) / a0;
    a2 = (1.0f - alpha) / a0;
}

[[nodiscard]] inline float processBiquad(float in, BiquadState& s, float b0, float b1, float b2, float a1, float a2) noexcept {
    const float out = b0 * in + b1 * s.x1 + b2 * s.x2 - a1 * s.y1 - a2 * s.y2;
    s.x2 = s.x1;
    s.x1 = in;
    s.y2 = s.y1;
    s.y1 = out;
    return out;
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

    // Input highpass filter (350 Hz) to keep the hi-hat shimmer crystalline and mud-free
    const float hpfAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 350.0 / sampleRate));
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

} // namespace

void HatSynth::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    reverb_.prepare(sampleRate);
    noiseDiffL_.prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0017)), 0.5f);
    noiseDiffR_.prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0023)), 0.5f);
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

    noiseHPF1_L_.reset();
    noiseHPF1_R_.reset();
    noiseHPF2_L_.reset();
    noiseHPF2_R_.reset();
    noiseLPF1_L_.reset();
    noiseLPF1_R_.reset();
    noiseLPF2_L_.reset();
    noiseLPF2_R_.reset();

    lastNoiseHPF_ = -1.0f;
    lastNoiseLPF_ = -1.0f;
    pinkB0_L_ = pinkB1_L_ = pinkB2_L_ = 0.0f;
    pinkB0_R_ = pinkB1_R_ = pinkB2_R_ = 0.0f;
    lofiHeld_L_ = lofiHeld_R_ = 0.0f;
    lofiCounter_L_ = lofiCounter_R_ = 0;

    noiseDiffL_.reset();
    noiseDiffR_.reset();
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
    const float dryTailMs = airTailMs(settings);
    const float effectTailMs = std::max({ settings.reverbDecayMs, settings.reverbGateMs, dryTailMs });
    const int tailSamples = static_cast<int>((effectTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_));
    totalDurationSamples_ = std::max(totalSamplesPlayed_ + 1,
        adsrReleaseStartSample_ + adsrReleaseSamples_ + tailSamples);
}

void HatSynth::start(const HatSynthSettings& settings, float velocity) noexcept {
    reset();
    isPlaying_ = true;
    chokeStartSample_ = -1;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);

    preChickCount_ = std::clamp(settings.preChickCount, 1, 5);
    const float spreadSamples = std::max(0.0f, settings.preChickSpreadMs) * 0.001f * static_cast<float>(sampleRate_);
    for (int i = 0; i < preChickCount_; ++i) {
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
    }

    mainHitSample_ = preChickCount_ > 1
        ? static_cast<int>(static_cast<float>(preChickCount_ - 1) * spreadSamples)
        : 0;

    const float preRollMs = static_cast<float>(mainHitSample_) * 1000.0f / static_cast<float>(sampleRate_);
    const float dryTailMs = airTailMs(settings);
    const float effectTailMs = std::max({settings.reverbDecayMs, settings.reverbGateMs, dryTailMs});
    dryDurationSamples_ = std::max(1, static_cast<int>((preRollMs + dryTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
    totalDurationSamples_ = std::max(dryDurationSamples_,
        static_cast<int>((preRollMs + effectTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
}

float HatSynth::generateNoiseSample(NoiseType type) noexcept {
    // Kept for single-channel callers; defaults to left channel state.
    const float white = nextRandomFloat(prngState_);
    if (type == NoiseType::PinkSilk) {
        pinkB0_L_ = 0.99765f * pinkB0_L_ + white * 0.0990460f;
        pinkB1_L_ = 0.96300f * pinkB1_L_ + white * 0.2965164f;
        pinkB2_L_ = 0.57000f * pinkB2_L_ + white * 1.0526913f;
        const float pink = (pinkB0_L_ + pinkB1_L_ + pinkB2_L_ + white * 0.1848f) * 0.35f;
        return 0.5f * white + 0.5f * pink;
    }
    if (type == NoiseType::VelvetShimmer) {
        return std::abs(white) > 0.82f ? white * 4.0f : white * 0.22f;
    }
    if (type == NoiseType::LoFiCrunch) {
        if (lofiCounter_L_ <= 0) {
            lofiHeld_L_ = std::round(white * 31.0f) / 31.0f;
            lofiCounter_L_ = 3;
        }
        --lofiCounter_L_;
        return lofiHeld_L_;
    }
    return white;
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
            const float env = std::exp(-t / 0.0012f);
            const float click = std::max(0.0f, 1.0f - t * 6000.0f);
            return (click * 0.6f + (partial(1.0f) * 0.6f + partial(1.62f) * 0.4f) * env) * level;
        }
        case ChickType::PedalFoot: {
            if (t > 0.018f) return 0.0f;
            const float env = std::exp(-t / 0.0028f);
            return (partial(0.6f) * 0.65f + partial(1.25f) * 0.35f) * env * level * 1.2f;
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

HatSynthLayers HatSynth::processSampleLayers(const HatSynthSettings& settings) noexcept {
    if (!isPlaying_) return {};

    const int currentSample = totalSamplesPlayed_++;
    if (totalSamplesPlayed_ >= totalDurationSamples_) {
        isPlaying_ = false;
        return {};
    }

    float preChickGainL = 0.0f;
    float preChickGainR = 0.0f;
    float preChickTickL = 0.0f;
    float preChickTickR = 0.0f;
    const float decayScale = std::clamp(settings.decayScale, 0.25f, 2.0f);
    const float burstDecaySamples = std::max(1.0f, settings.preChickDecayMs * decayScale * 0.001f * static_cast<float>(sampleRate_));
    const float mainAge = static_cast<float>(currentSample - mainHitSample_);

    for (int i = 0; i < preChickCount_; ++i) {
        const int age = currentSample - burstStartSamples_[i];
        if (age < 0 || static_cast<float>(age) >= burstDecaySamples * 4.0f) continue;
        const float envelope = std::exp(-static_cast<float>(age) / burstDecaySamples) * burstAmplitudes_[i];
        const float pan = burstPans_[i];
        const float gainL = std::sqrt(std::clamp(0.5f * (1.0f - pan), 0.0f, 1.0f));
        const float gainR = std::sqrt(std::clamp(0.5f * (1.0f + pan), 0.0f, 1.0f));
        preChickGainL += envelope * gainL;
        preChickGainR += envelope * gainR;
        const float tick = generateChickSample(settings.chickType, settings.chickFreqHz * midiPitchRatio_,
                                               age, burstAmplitudes_[i] * 0.55f);
        preChickTickL += tick * gainL;
        preChickTickR += tick * gainR;
    }

    float noiseEnvelope = 0.0f;
    float chick = 0.0f;
    if (mainAge >= 0.0f) {
        if (fullAdsrActive_) {
            noiseEnvelope = 1.0f;
        } else {
            const float attackSamples = std::max(0.0f, settings.noiseAttackMs) * 0.001f * static_cast<float>(sampleRate_);
            const float holdSamples = std::max(0.0f, settings.noiseHoldMs) * 0.001f * static_cast<float>(sampleRate_);
            const float decaySamples = std::max(1.0f, settings.noiseDecayMs * decayScale * 0.001f * static_cast<float>(sampleRate_));
            if (mainAge < attackSamples && attackSamples > 0.0f) {
                noiseEnvelope = 0.5f * (1.0f - std::cos(pi() * mainAge / attackSamples));
            } else if (mainAge < attackSamples + holdSamples) {
                noiseEnvelope = 1.0f;
            } else {
                const float decayAge = mainAge - attackSamples - holdSamples;
                const float linear = std::clamp(decayAge / decaySamples, 0.0f, 1.0f);
                const float exponential = std::exp(-decayAge / std::max(1.0f, decaySamples * 0.35f));
                const float curve = std::clamp(settings.noiseCurve, -1.0f, 1.0f);
                noiseEnvelope = curve < 0.0f
                    ? (1.0f + curve) * (1.0f - linear) - curve * exponential
                    : (1.0f - curve) * exponential + curve * (1.0f - linear);
            }
        }
        chick = generateChickSample(settings.chickType, settings.chickFreqHz * midiPitchRatio_,
                                     static_cast<int>(mainAge), settings.chickLevel);
    }

    const float rawNoiseL = generateNoiseSample(settings.noiseType);
    const float whiteR = nextRandomFloat(prngState_);
    float rawNoiseR = whiteR;
    if (settings.noiseType == NoiseType::PinkSilk) {
        pinkB0_R_ = 0.99765f * pinkB0_R_ + whiteR * 0.0990460f;
        pinkB1_R_ = 0.96300f * pinkB1_R_ + whiteR * 0.2965164f;
        pinkB2_R_ = 0.57000f * pinkB2_R_ + whiteR * 1.0526913f;
        const float pinkR = (pinkB0_R_ + pinkB1_R_ + pinkB2_R_ + whiteR * 0.1848f) * 0.35f;
        rawNoiseR = 0.5f * whiteR + 0.5f * pinkR;
    } else if (settings.noiseType == NoiseType::VelvetShimmer) {
        rawNoiseR = std::abs(whiteR) > 0.82f ? whiteR * 4.0f : whiteR * 0.22f;
    } else if (settings.noiseType == NoiseType::LoFiCrunch) {
        if (lofiCounter_R_ <= 0) {
            lofiHeld_R_ = std::round(whiteR * 31.0f) / 31.0f;
            lofiCounter_R_ = 3;
        }
        --lofiCounter_R_;
        rawNoiseR = lofiHeld_R_;
    }
    const float attackNoiseL = generateNoiseSample(NoiseType::CrispWhite);
    const float attackNoiseR = generateNoiseSample(NoiseType::CrispWhite);
    const float airVelocity = std::pow(velocity_, std::clamp(settings.velocityAir, 0.25f, 4.0f));
    const float chickVelocity = std::pow(velocity_, std::clamp(settings.velocityChick, 0.25f, 4.0f));
    const float reverbVelocity = std::pow(velocity_, std::clamp(settings.velocityReverb, 0.25f, 4.0f));
    const float attackEnvelope = mainAge >= 0.0f
        ? std::exp(-mainAge / std::max(1.0f, static_cast<float>(sampleRate_) * 0.0025f))
        : 0.0f;
    const float attackLevel = std::clamp(settings.chickLevel, 0.0f, 1.0f) * 1.8f;
    const float noiseSwellL = rawNoiseL * preChickGainL * 0.4f + rawNoiseL * noiseEnvelope * settings.noiseLevel;
    const float noiseSwellR = rawNoiseR * preChickGainR * 0.4f + rawNoiseR * noiseEnvelope * settings.noiseLevel;
    // The air layer is unpitched, so its filters follow the knobs and never the played note.
    if (std::abs(settings.noiseHPF - lastNoiseHPF_) > 1.0f) {
        lastNoiseHPF_ = settings.noiseHPF;
        computeHighpassCoeffs(lastNoiseHPF_, 0.54f, sampleRate_, b0_nh1_, b1_nh1_, b2_nh1_, a1_nh1_, a2_nh1_);
        computeHighpassCoeffs(lastNoiseHPF_, 1.30f, sampleRate_, b0_nh2_, b1_nh2_, b2_nh2_, a1_nh2_, a2_nh2_);
    }
    if (std::abs(settings.noiseLPF - lastNoiseLPF_) > 1.0f) {
        lastNoiseLPF_ = settings.noiseLPF;
        computeLowpassCoeffs(lastNoiseLPF_, 0.54f, sampleRate_, b0_nl1_, b1_nl1_, b2_nl1_, a1_nl1_, a2_nl1_);
        computeLowpassCoeffs(lastNoiseLPF_, 1.30f, sampleRate_, b0_nl2_, b1_nl2_, b2_nl2_, a1_nl2_, a2_nl2_);
    }

    float airL = processBiquad(noiseSwellL, noiseHPF1_L_, b0_nh1_, b1_nh1_, b2_nh1_, a1_nh1_, a2_nh1_);
    airL = processBiquad(airL, noiseHPF2_L_, b0_nh2_, b1_nh2_, b2_nh2_, a1_nh2_, a2_nh2_);
    airL = processBiquad(airL, noiseLPF1_L_, b0_nl1_, b1_nl1_, b2_nl1_, a1_nl1_, a2_nl1_);
    airL = processBiquad(airL, noiseLPF2_L_, b0_nl2_, b1_nl2_, b2_nl2_, a1_nl2_, a2_nl2_);
    float airR = processBiquad(noiseSwellR, noiseHPF1_R_, b0_nh1_, b1_nh1_, b2_nh1_, a1_nh1_, a2_nh1_);
    airR = processBiquad(airR, noiseHPF2_R_, b0_nh2_, b1_nh2_, b2_nh2_, a1_nh2_, a2_nh2_);
    airR = processBiquad(airR, noiseLPF1_R_, b0_nl1_, b1_nl1_, b2_nl1_, a1_nl1_, a2_nl1_);
    airR = processBiquad(airR, noiseLPF2_R_, b0_nl2_, b1_nl2_, b2_nl2_, a1_nl2_, a2_nl2_);

    if (settings.noiseType == NoiseType::PinkSilk || settings.noiseType == NoiseType::VelvetShimmer) {
        airL = noiseDiffL_.process(airL);
        airR = noiseDiffR_.process(airR);
    }
    if (settings.noiseStereoWidth != 1.0f) {
        const float mid = (airL + airR) * 0.5f;
        const float side = (airL - airR) * 0.5f * settings.noiseStereoWidth;
        airL = mid + side;
        airR = mid - side;
    }
    const float unscaledAirL = airL;
    const float unscaledAirR = airR;
    airL *= airVelocity;
    airR *= airVelocity;

    const float dryDuration = static_cast<float>(dryDurationSamples_);
    const float attackSamples = std::clamp(settings.ampAttackMs, 0.05f, 20.0f) * 0.001f * static_cast<float>(sampleRate_);
    const float releaseSamples = std::clamp(settings.ampReleaseMs, 5.0f, 800.0f) * 0.001f * static_cast<float>(sampleRate_);
    const float safeAttack = std::min(attackSamples, dryDuration * 0.30f);
    const float safeRelease = std::min(releaseSamples, dryDuration * 0.35f);
    const float decaySamples = std::clamp(settings.ampDecayMs, 1.0f, 600.0f) * 0.001f * static_cast<float>(sampleRate_);
    const float safeDecay = std::min(decaySamples, std::max(0.0f, dryDuration - safeAttack - safeRelease));
    const float sustain = std::clamp(settings.ampSustain, 0.0f, 1.0f);
    const float releaseStart = std::max(safeAttack + safeDecay, dryDuration - safeRelease);
    float amplitudeEnvelope = 0.0f;
    if (fullAdsrActive_) {
        amplitudeEnvelope = fullAdsrEnvelopeAt(currentSample, settings);
    } else if (static_cast<float>(currentSample) < safeAttack) {
        amplitudeEnvelope = static_cast<float>(currentSample) / std::max(1.0f, safeAttack);
    } else if (static_cast<float>(currentSample) < safeAttack + safeDecay) {
        const float phase = (static_cast<float>(currentSample) - safeAttack) / std::max(1.0f, safeDecay);
        amplitudeEnvelope = 1.0f + (sustain - 1.0f) * phase;
    } else if (static_cast<float>(currentSample) < releaseStart) {
        amplitudeEnvelope = sustain;
    } else {
        amplitudeEnvelope = sustain * std::max(0.0f, dryDuration - static_cast<float>(currentSample))
            / std::max(1.0f, dryDuration - releaseStart);
    }

    const float unscaledChickL = chick + preChickTickL + attackNoiseL * attackEnvelope * attackLevel;
    const float unscaledChickR = chick + preChickTickR + attackNoiseR * attackEnvelope * attackLevel;
    const float chickL = unscaledChickL * chickVelocity;
    const float chickR = unscaledChickR * chickVelocity;
    const float dryL = (airL + chickL) * amplitudeEnvelope;
    const float dryR = (airR + chickR) * amplitudeEnvelope;
    const float reverbSendL = (unscaledAirL + unscaledChickL) * reverbVelocity * amplitudeEnvelope;
    const float reverbSendR = (unscaledAirR + unscaledChickR) * reverbVelocity * amplitudeEnvelope;
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
    for (int i = 0; i < totalDurationSamples_; ++i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    }
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
    for (int i = 0; i < totalDurationSamples_; ++i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    }
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
    for (int i = 0; i < renderCount; ++i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    }
    return renderCount;
}

} // namespace openhatter::core
