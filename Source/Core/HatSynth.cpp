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

// Generate decorrelated bipolar white noise via xorshift64* without short-cycle periodicity.
[[nodiscard]] inline float nextRandom64(uint64_t& state) noexcept {
    const uint64_t fallback = state == 0ULL ? 0x9E3779B97F4A7C15ULL : 0ULL;
    state ^= fallback;
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    const uint64_t word = state * 0x2545F4914F6CDD1DULL;
    return static_cast<float>(static_cast<double>(word >> 40) * (2.0 / 16777216.0) - 1.0);
}

struct SvfOutputs {
    float lowpass = 0.0f;
    float bandpass = 0.0f;
    float highpass = 0.0f;
    float resonator = 0.0f;
};

// Pre-warp cutoff with tan(pi * fc / fs) so high-frequency hat bands stay accurate near Nyquist.
[[nodiscard]] SvfCoeffs computeSvfCoeffs(float freqHz, float quality, double sampleRate) noexcept {
    const float safeRate = static_cast<float>(std::max(1000.0, sampleRate));
    const float nyquistLimit = safeRate * 0.485f;
    const float clampedFreq = std::clamp(freqHz, 12.0f, nyquistLimit);
    const float clampedQuality = std::clamp(quality, 0.25f, 240.0f);
    const float warp = std::tan(pi() * clampedFreq / safeRate);
    const float damping = 1.0f / clampedQuality;
    const float a1 = 1.0f / (1.0f + warp * (warp + damping));
    const float a2 = warp * a1;
    const float a3 = warp * a2;
    const float bandFade = std::clamp((0.475f - freqHz / safeRate) * 16.0f, 0.0f, 1.0f);
    const float unitImpulseScale = bandFade * (1.0f + warp * warp) / std::max(1.0e-5f, 2.0f * warp);
    return {
        .warp = warp,
        .damping = damping,
        .a1 = a1,
        .a2 = a2,
        .a3 = a3,
        .modalScale = unitImpulseScale
    };
}

[[nodiscard]] inline SvfOutputs stepSvf(float input, SvfState& state, const SvfCoeffs& coeffs) noexcept {
    const float v3 = input - state.ic2eq;
    const float v1 = coeffs.a1 * state.ic1eq + coeffs.a2 * v3;
    const float v2 = state.ic2eq + coeffs.a2 * state.ic1eq + coeffs.a3 * v3;
    state.ic1eq = 2.0f * v1 - state.ic1eq;
    state.ic2eq = 2.0f * v2 - state.ic2eq;
    const float highpass = input - coeffs.damping * v1 - v2;
    return {
        .lowpass = v2,
        .bandpass = coeffs.damping * v1,
        .highpass = highpass,
        .resonator = v1 * coeffs.modalScale
    };
}

[[nodiscard]] inline float advancePhase(float& phase, float hz, double sampleRate) noexcept {
    phase += hz / static_cast<float>(std::max(1000.0, sampleRate));
    phase -= std::floor(phase);
    return phase;
}

// Apply a 2-sample polynomial residual across step discontinuities to suppress oscillator aliasing.
[[nodiscard]] inline float polyBlep(float phasePos, float phaseStep) noexcept {
    if (phaseStep <= 1.0e-7f) return 0.0f;
    if (phasePos < phaseStep) {
        const float norm = phasePos / phaseStep;
        return norm + norm - norm * norm - 1.0f;
    }
    if (phasePos > 1.0f - phaseStep) {
        const float norm = (phasePos - 1.0f) / phaseStep;
        return norm * norm + norm + norm + 1.0f;
    }
    return 0.0f;
}

// Read fractional delay taps with 4-point cubic Lagrange interpolation to avoid pitch quantization.
[[nodiscard]] inline float readLagrange3(
    std::span<const float> buffer, size_t writeIdx, float delaySamples) noexcept {
    const size_t capacity = buffer.size();
    if (capacity < 6) return 0.0f;
    const float maxDelay = static_cast<float>(capacity - 4);
    const float clamped = std::clamp(delaySamples, 2.0f, maxDelay);
    const size_t baseDelay = static_cast<size_t>(clamped);
    const float frac = clamped - static_cast<float>(baseDelay);

    const float ym1 = buffer[(writeIdx + capacity - (baseDelay - 1)) % capacity];
    const float y0 = buffer[(writeIdx + capacity - baseDelay) % capacity];
    const float y1 = buffer[(writeIdx + capacity - (baseDelay + 1)) % capacity];
    const float y2 = buffer[(writeIdx + capacity - (baseDelay + 2)) % capacity];

    const float c0 = y0;
    const float c1 = 0.5f * (y1 - ym1) - (y0 - 2.0f * y1 + y2 - (ym1 - 2.0f * y0 + y1)) / 6.0f;
    const float c2 = 0.5f * (ym1 - 2.0f * y0 + y1);
    const float c3 = (y2 - ym1 + 3.0f * (y0 - y1)) / 6.0f;
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

[[nodiscard]] inline float dcBlock(float x, float& x1, float& y1) noexcept {
    const float y = x - x1 + 0.995f * y1;
    x1 = x;
    y1 = y;
    return y;
}

constexpr std::array<float, 14> kBronzeRatios{{
    1.00f, 1.46f, 1.97f, 2.58f, 3.29f, 4.12f, 5.08f,
    6.19f, 7.47f, 8.93f, 10.58f, 12.44f, 14.52f, 16.85f
}};

// L and R partials sit 4..9 cents apart, split evenly around the nominal ratio.
// A wider split beats the mono sum to a null inside a ride's first 160 ms.
constexpr std::array<std::array<float, 6>, 2> kMetalDetuneCents{{
    {{ -3.75f, +2.6f, -4.3f, +2.2f, -3.05f, +4.05f }},
    {{ +3.75f, -2.6f, +4.3f, -2.2f, +3.05f, -4.05f }}
}};

constexpr std::array<std::array<float, 14>, 2> kBronzeDetuneCents{{
    {{ -3.1f, +2.4f, -4.2f, +2.75f, -3.55f, +4.4f, -2.25f, +3.45f, -4.05f, +2.55f, -3.35f, +4.25f, -2.65f, +3.8f }},
    {{ +3.1f, -2.4f, +4.2f, -2.75f, +3.55f, -4.4f, +2.25f, -3.45f, +4.05f, -2.55f, +3.35f, -4.25f, +2.65f, -3.8f }}
}};

constexpr std::array<std::array<float, 4>, 2> kShellDetuneCents{{
    {{ -3.4f, +2.7f, -3.95f, +3.05f }},
    {{ +3.4f, -2.7f, +3.95f, -3.05f }}
}};

} // namespace

// ============================================================================
// GatedStereoReverb
// ============================================================================

void GatedStereoReverb::prepare(double sampleRate) {
    inputDiffusersL_[0].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0031)), 0.65f);
    inputDiffusersL_[1].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0079)), 0.60f);
    inputDiffusersR_[0].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0027)), 0.65f);
    inputDiffusersR_[1].prepare(std::max<size_t>(4, static_cast<size_t>(sampleRate * 0.0083)), 0.60f);

    // Prime-ratio base lengths prevent coincident echoes across the 4 FDN loops.
    constexpr std::array<double, 4> kBaseSec = { 0.0193, 0.0241, 0.0307, 0.0379 };
    constexpr std::array<size_t, 4> kPrimeOffsets = { 1, 3, 7, 9 };
    const size_t ringCapacity = std::max<size_t>(64, static_cast<size_t>(sampleRate * 0.065) + 16);
    std::ranges::for_each(std::views::iota(size_t{0}, size_t{4}), [&](size_t lineIdx) {
        const size_t rawLen = std::max<size_t>(17, static_cast<size_t>(sampleRate * kBaseSec[lineIdx]));
        const size_t oddLen = (rawLen | size_t{1}) + kPrimeOffsets[lineIdx] - 1;
        baseDelaySamples_[lineIdx] = static_cast<float>(oddLen);
        delayLines_[lineIdx].assign(ringCapacity, 0.0f);
    });

    reset();
}

void GatedStereoReverb::reset() noexcept {
    std::ranges::for_each(inputDiffusersL_, [](AllpassDiffuser& diff) { diff.reset(); });
    std::ranges::for_each(inputDiffusersR_, [](AllpassDiffuser& diff) { diff.reset(); });
    std::ranges::for_each(delayLines_, [](std::vector<float>& line) { std::ranges::fill(line, 0.0f); });
    writeIndices_.fill(0);
    hfStates_.fill(0.0f);
    lfStates_.fill(0.0f);
    lfoPhases_ = { 0.0f, 0.25f, 0.50f, 0.75f };
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
    if (delayLines_[0].empty()) return { inLeft, inRight };

    ++sampleCounter_;
    const float safeRate = static_cast<float>(std::max(1000.0, sampleRate));

    // Apply a smoothstep S-curve over the 15 ms choke window so gated tails close without clicks.
    const float gateSamples = gateMs * 0.001f * safeRate;
    const float releaseSamples = std::max(1.0f, 15.0f * 0.001f * safeRate);
    const float samplesOver = static_cast<float>(sampleCounter_) - gateSamples;
    const float rawProgress = std::clamp(samplesOver / releaseSamples, 0.0f, 1.0f);
    const float smoothGate = 1.0f - rawProgress * rawProgress * (3.0f - 2.0f * rawProgress);
    const bool gateActive = gateMs > 0.0f && samplesOver > 0.0f;
    gateEnvelope_ = gateActive ? smoothGate : 1.0f;
    if (gateEnvelope_ <= 0.0f) return { 0.0f, 0.0f };

    const float hpfAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 220.0 / safeRate));
    hpfStateL_ += hpfAlpha * (inLeft - hpfStateL_);
    hpfStateR_ += hpfAlpha * (inRight - hpfStateR_);
    const float hpL = inLeft - hpfStateL_;
    const float hpR = inRight - hpfStateR_;

    const float diffInL = inputDiffusersL_[1].process(inputDiffusersL_[0].process(hpL));
    const float diffInR = inputDiffusersR_[1].process(inputDiffusersR_[0].process(hpR));

    const float clampedSize = std::clamp(size, 0.1f, 1.0f);
    const float roomScale = 0.72f + 0.45f * clampedSize;
    const float modDepthSamples = (0.16f + 0.22f * clampedSize) * 0.001f * safeRate;
    const float clampedDamp = std::clamp(damp, 0.0f, 1.0f);
    const float hfCutHz = 2400.0f + 9600.0f * (1.0f - clampedDamp);
    const float hfAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * hfCutHz / safeRate));
    const float hfDrop = 0.14f + 0.72f * clampedDamp;
    const float lfAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 185.0 / safeRate));
    constexpr std::array<float, 4> kLfoRatesHz{{ 0.41f, 0.53f, 0.67f, 0.79f }};

    std::array<float, 4> rawTaps{};
    std::array<float, 4> absorbed{};
    std::ranges::for_each(std::views::iota(size_t{0}, size_t{4}), [&](size_t lineIdx) {
        const float lfoPhase = advancePhase(lfoPhases_[lineIdx], kLfoRatesHz[lineIdx], safeRate);
        const float nominalSamples = baseDelaySamples_[lineIdx] * roomScale;
        const float readDelay = nominalSamples + modDepthSamples * std::sin(2.0f * pi() * lfoPhase);
        const float tap = readLagrange3(delayLines_[lineIdx], writeIndices_[lineIdx], readDelay);
        rawTaps[lineIdx] = tap;

        // Place Jot per-line HF and LF shelving absorption inside each loop before Hadamard mixing.
        hfStates_[lineIdx] += hfAlpha * (tap - hfStates_[lineIdx]);
        const float hfShelved = tap * (1.0f - hfDrop) + hfStates_[lineIdx] * hfDrop;
        lfStates_[lineIdx] += lfAlpha * (hfShelved - lfStates_[lineIdx]);
        const float bothShelved = hfShelved - lfStates_[lineIdx] * 0.20f;

        const float loopMs = nominalSamples * 1000.0f / safeRate;
        const float loopGain = std::clamp(std::pow(0.001f, loopMs / std::max(50.0f, decayMs)), 0.12f, 0.96f);
        absorbed[lineIdx] = bothShelved * loopGain;
    });

    const float mix0 = 0.5f * (absorbed[0] + absorbed[1] + absorbed[2] + absorbed[3]);
    const float mix1 = 0.5f * (absorbed[0] - absorbed[1] + absorbed[2] - absorbed[3]);
    const float mix2 = 0.5f * (absorbed[0] + absorbed[1] - absorbed[2] - absorbed[3]);
    const float mix3 = 0.5f * (absorbed[0] - absorbed[1] - absorbed[2] + absorbed[3]);
    const std::array<float, 4> mixed{{ mix0, mix1, mix2, mix3 }};

    std::ranges::for_each(std::views::iota(size_t{0}, size_t{4}), [&](size_t lineIdx) {
        const float inputFeed = (lineIdx % 2 == 0) ? diffInL : diffInR;
        const size_t capacity = delayLines_[lineIdx].size();
        delayLines_[lineIdx][writeIndices_[lineIdx]] = inputFeed * 0.70f + mixed[lineIdx];
        writeIndices_[lineIdx] = (writeIndices_[lineIdx] + 1) % capacity;
    });

    const float wetL = (rawTaps[0] + rawTaps[2]) * 0.70f * gateEnvelope_;
    const float wetR = (rawTaps[1] + rawTaps[3]) * 0.70f * gateEnvelope_;
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
    sizzleSplitCoeffs_ = computeSvfCoeffs(3000.0f, 0.5f, sampleRate);
    sizzlePostCoeffs_ = computeSvfCoeffs(4500.0f, 0.7071f, sampleRate);

    // Coprime delays with orthogonal feedback signs decorrelate L and R shimmer without altering tone.
    constexpr std::array<std::array<double, 2>, 2> kDiffuserSec{{
        {{ 0.00061, 0.00137 }},
        {{ 0.00083, 0.00113 }}
    }};
    constexpr std::array<std::array<float, 2>, 2> kDiffuserGains{{
        {{ +0.30f, -0.26f }},
        {{ -0.30f, +0.26f }}
    }};
    const size_t combCap = std::max<size_t>(64, static_cast<size_t>(safeRate * 0.028) + 8);
    std::ranges::for_each(std::views::iota(size_t{0}, channels_.size()), [&](size_t chIdx) {
        std::ranges::for_each(std::views::iota(size_t{0}, size_t{2}), [&](size_t stageIdx) {
            const size_t delaySamples = std::max<size_t>(
                3, static_cast<size_t>(safeRate * kDiffuserSec[chIdx][stageIdx]));
            channels_[chIdx].diffusers[stageIdx].prepare(delaySamples, kDiffuserGains[chIdx][stageIdx]);
        });
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
    lastBodyDecayMs_ = -1.0f;
    lastDecayScale_ = -1.0f;
    lastTrackedPitch_ = -1.0f;
    lastModelType_ = -1;
    lastModelColor_ = -10.0f;
    lastModelDensity_ = -10.0f;
    chestPhase_ = 0.0f;

    std::ranges::for_each(channels_, [](ChannelState& ch) {
        ch.hpf1.reset();
        ch.hpf2.reset();
        ch.lpf1.reset();
        ch.lpf2.reset();
        std::ranges::for_each(ch.bodyModes, [](SvfState& mode) { mode.reset(); });
        std::ranges::for_each(ch.bronzeModes, [](SvfState& mode) { mode.reset(); });
        ch.metalOversampler.reset();
        ch.sizzleAdaa.reset();
        ch.combAdaa.reset();
        ch.sizzleSplit.reset();
        ch.sizzlePostHp.reset();
        ch.tiltLp = 0.0f;
        ch.bodySubHp = 0.0f;
        ch.bodyWarmLp = 0.0f;
        ch.pinkB0 = ch.pinkB1 = ch.pinkB2 = 0.0f;
        ch.lofiHeld = 0.0f;
        ch.lofiCounter = 0;
        ch.metalPhases.fill(0.0f);
        ch.sizzlePhases.fill(0.0f);
        std::ranges::fill(ch.combDelay, 0.0f);
        ch.combWrite = 0;
        ch.combFilter = 0.0f;
        std::ranges::for_each(ch.diffusers, [](AllpassDiffuser& diff) { diff.reset(); });
        ch.modelBp.reset();
        ch.modelHp.reset();
        ch.shade = 1.0f;
        ch.shadeLp = 0.0f;
        ch.ringEnv = 1.0f;
        ch.stickHp = 0.0f;
        ch.stickLp = 0.0f;
        ch.dcX = 0.0f;
        ch.dcY = 0.0f;
    });

    reverb_.reset();
}

float HatSynth::trackedPitchRatio(const HatSynthSettings& settings) const noexcept {
    const float track = std::clamp(settings.keyTracking, 0.0f, 1.0f);
    if (track <= 0.0001f) return 1.0f;
    return std::clamp(std::pow(midiPitchRatio_, track), 0.25f, 4.0f);
}

void HatSynth::trigger(const HatSynthSettings& settings, float velocity) noexcept {
    start(settings, velocity, 1.0f);
}

void HatSynth::triggerMidi(const HatSynthSettings& settings, float velocity, int midiNoteNumber, int midiChannel) noexcept {
    const int note = std::clamp(midiNoteNumber, 0, 127);
    const float noteRatio = midiNoteToHz(note) / midiNoteToHz(60);
    start(settings, velocity, noteRatio);
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

void HatSynth::start(const HatSynthSettings& settings, float velocity, float pitchRatio) noexcept {
    reset();
    midiPitchRatio_ = pitchRatio;
    // Scatter oscillator phase so the partials don't start as one stacked click.
    // Each channel draws from its own PRNG, so L and R never start phase-locked.
    std::ranges::for_each(channels_, [](ChannelState& ch) {
        const auto scramble = [&](auto& phases) {
            std::ranges::for_each(phases, [&](float& phase) { phase = 0.5f * (nextRandom64(ch.prng) + 1.0f); });
        };
        scramble(ch.metalPhases);
        scramble(ch.sizzlePhases);
        ch.shade = 1.0f;
    });
    isPlaying_ = true;
    chokeStartSample_ = -1;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);

    preChickCount_ = std::clamp(settings.preChickCount, 1, 5);
    const float spreadSamples = std::max(0.0f, settings.preChickSpreadMs) * 0.001f * static_cast<float>(sampleRate_);
    std::ranges::for_each(std::views::iota(0, preChickCount_), [&](int i) {
        const float jitterTime = nextRandom64(burstPrng_) * 0.5f * settings.preChickJitter * spreadSamples;
        burstStartSamples_[i] = std::max(0, static_cast<int>(static_cast<float>(i) * spreadSamples + jitterTime));
        const float progression = preChickCount_ > 1
            ? 0.65f + 0.35f * static_cast<float>(i) / static_cast<float>(preChickCount_ - 1)
            : 1.0f;
        const float jitterAmplitude = 1.0f + nextRandom64(burstPrng_) * 0.15f * settings.preChickJitter;
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
    if (std::abs(settings.noiseHPF - lastNoiseHPF_) > 1.0e-3f) {
        lastNoiseHPF_ = settings.noiseHPF;
        nh1Coeffs_ = computeSvfCoeffs(lastNoiseHPF_, 0.5412f, sampleRate_);
        nh2Coeffs_ = computeSvfCoeffs(lastNoiseHPF_, 1.3066f, sampleRate_);
    }
    if (std::abs(settings.noiseLPF - lastNoiseLPF_) > 1.0e-3f) {
        lastNoiseLPF_ = settings.noiseLPF;
        nl1Coeffs_ = computeSvfCoeffs(lastNoiseLPF_, 0.5412f, sampleRate_);
        nl2Coeffs_ = computeSvfCoeffs(lastNoiseLPF_, 1.3066f, sampleRate_);
    }

    const float clampedTone = std::clamp(settings.bodyTone, 0.0f, 1.0f);
    const float clampedScale = std::clamp(settings.decayScale, 0.25f, 2.0f);
    const float trackedPitch = trackedPitchRatio(settings);
    const bool pitchChanged = std::abs(trackedPitch - lastTrackedPitch_) > 1.0e-4f;

    if (std::abs(clampedTone - lastBodyTone_) > 1.0e-3f
        || std::abs(settings.bodyDecayMs - lastBodyDecayMs_) > 0.05f
        || std::abs(clampedScale - lastDecayScale_) > 1.0e-3f
        || pitchChanged) {
        lastBodyTone_ = clampedTone;
        lastBodyDecayMs_ = settings.bodyDecayMs;
        lastDecayScale_ = clampedScale;

        // A hat shell lives around 0.9–5 kHz. The old 330 Hz base was a tom.
        // High-Q modes store the strike energy. Higher modes die sooner.
        constexpr std::array<float, kShellModeCount> kShellRatios{{ 1.00f, 1.48f, 2.11f, 2.89f }};
        const float shellBaseHz = 880.0f * std::exp2(clampedTone * 1.05f) * trackedPitch;
        const float shellBaseTau = std::clamp(settings.bodyDecayMs * clampedScale * 0.00011f, 0.008f, 0.036f);
        std::ranges::for_each(std::views::iota(size_t{0}, size_t{2}), [&](size_t chIdx) {
            std::ranges::for_each(std::views::iota(size_t{0}, kShellModeCount), [&](size_t modeIdx) {
                const float detune = std::exp2(kShellDetuneCents[chIdx][modeIdx] / 1200.0f);
                const float modeHz = shellBaseHz * kShellRatios[modeIdx] * detune;
                const float modeTau = shellBaseTau * std::pow(kShellRatios[modeIdx], -0.55f);
                const float modeQuality = std::clamp(pi() * modeHz * modeTau, 18.0f, 185.0f);
                bodyModeCoeffs_[chIdx][modeIdx] = computeSvfCoeffs(modeHz, modeQuality, sampleRate_);
            });
        });
    }

    const int modelType = static_cast<int>(settings.noiseType);
    const float modelColor = std::clamp(settings.hissColor, -1.0f, 1.0f);
    const float modelDensity = std::clamp(settings.hissDensity, 0.0f, 1.0f);
    if (modelType != lastModelType_
        || std::abs(modelColor - lastModelColor_) > 1.0e-3f
        || std::abs(modelDensity - lastModelDensity_) > 1.0e-3f
        || pitchChanged) {
        lastModelType_ = modelType;
        lastModelColor_ = modelColor;
        lastModelDensity_ = modelDensity;
        lastTrackedPitch_ = trackedPitch;

        float bpHz = 6000.0f;
        float bpQ = 0.8f;
        float hpHz = 2500.0f;
        float hpQ = 0.7f;
        switch (settings.noiseType) {
            case NoiseType::CrispWhite:
                bpHz = 7200.0f * std::exp2(modelColor * 0.40f);
                // Broad hat band. A high Q here is a whistle, not a 909.
                bpQ = 0.72f + 0.35f * modelDensity;
                hpHz = std::clamp(bpHz * 0.42f, 1600.0f, 5200.0f);
                hpQ = 0.55f;
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
            case NoiseType::AnalogMetal: {
                const float filterPitch = std::pow(trackedPitch, 0.65f);
                hpHz = 3400.0f * std::exp2(modelColor * 0.28f) * filterPitch;
                hpQ = 0.75f;
                bpHz = 7800.0f * std::exp2(modelColor * 0.22f) * filterPitch;
                bpQ = 1.15f;
                break;
            }
            case NoiseType::BronzeCymbal: {
                hpHz = 3400.0f * std::exp2(modelColor * 0.35f) * std::pow(trackedPitch, 0.65f);
                hpQ = 0.60f;
                bpHz = 6200.0f;
                bpQ = 0.70f;

                // Damp upper bronze partials faster via tau_k = tau_0 * r_k^(-0.95).
                const float ring = std::clamp((modelDensity - 0.58f) / 0.30f, 0.0f, 1.0f);
                const float baseTau = 0.024f + 0.38f * ring;
                const float bronzeBaseHz = 780.0f * std::exp2(modelColor * 0.40f) * trackedPitch;
                std::ranges::for_each(std::views::iota(size_t{0}, size_t{2}), [&](size_t chIdx) {
                    std::ranges::for_each(std::views::iota(size_t{0}, kBronzeModeCount), [&](size_t modeIdx) {
                        const float detune = std::exp2(kBronzeDetuneCents[chIdx][modeIdx] / 1200.0f);
                        const float modeHz = bronzeBaseHz * kBronzeRatios[modeIdx] * detune;
                        const float modeTau = std::max(
                            0.0035f, baseTau * std::pow(kBronzeRatios[modeIdx], -0.95f));
                        const float modeQuality = std::clamp(pi() * modeHz * modeTau, 8.0f, 240.0f);
                        bronzeModeCoeffs_[chIdx][modeIdx] = computeSvfCoeffs(modeHz, modeQuality, sampleRate_);
                    });
                });
                break;
            }
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
        modelBpCoeffs_ = computeSvfCoeffs(bpHz, bpQ, sampleRate_);
        modelHpCoeffs_ = computeSvfCoeffs(hpHz, hpQ, sampleRate_);
    }
}

float HatSynth::generateHissChannel(
    ChannelState& ch, int channelIdx, float strikeExcite, const HatSynthSettings& settings) noexcept {
    const float white = nextRandom64(ch.prng);
    ch.pinkB0 = 0.99765f * ch.pinkB0 + white * 0.0990460f;
    ch.pinkB1 = 0.96300f * ch.pinkB1 + white * 0.2965164f;
    ch.pinkB2 = 0.57000f * ch.pinkB2 + white * 1.0526913f;
    const float pink = (ch.pinkB0 + ch.pinkB1 + ch.pinkB2 + white * 0.1848f) * 0.35f;

    const float sr = static_cast<float>(sampleRate_);
    const float color = std::clamp(settings.hissColor, -1.0f, 1.0f);
    const float density = std::clamp(settings.hissDensity, 0.0f, 1.0f);
    const float trackedPitch = trackedPitchRatio(settings);
    if (!fullAdsrActive_) {
        const float tau = 0.15f + 0.10f * (1.0f - density);
        ch.shade *= std::exp(-1.0f / (sr * tau));
    }

    switch (settings.noiseType) {
        case NoiseType::CrispWhite: {
            // 909-style: noise through a resonant peak plus a softer highpassed bed.
            const float bp = stepSvf(white, ch.modelBp, modelBpCoeffs_).bandpass;
            const float hp = stepSvf(white, ch.modelHp, modelHpCoeffs_).highpass;
            return bp * 1.05f + hp * 0.42f;
        }
        case NoiseType::PinkSilk: {
            const float bed = 0.72f * pink + 0.28f * white;
            const float bp = stepSvf(bed, ch.modelBp, modelBpCoeffs_).bandpass;
            const float hp = stepSvf(pink, ch.modelHp, modelHpCoeffs_).highpass;
            return bp * 0.72f + hp * 0.28f + pink * 0.06f;
        }
        case NoiseType::VelvetShimmer: {
            // Soft noisy shimmer. Sine partials here read as a bell, not a hat.
            const float air = stepSvf(white, ch.modelHp, modelHpCoeffs_).highpass;
            const float bp = stepSvf(0.55f * white + 0.45f * pink, ch.modelBp, modelBpCoeffs_).bandpass;
            const float rateHz = 3.2f + 4.5f * density;
            const float lfo = 0.78f + 0.22f * std::sin(2.0f * pi() * advancePhase(ch.sizzlePhases[0], rateHz, sampleRate_));
            return (bp * 0.95f + air * 0.42f) * lfo;
        }
        case NoiseType::LoFiCrunch: {
            const float bright = stepSvf(white, ch.modelHp, modelHpCoeffs_).highpass;
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
            // Six PolyBLEP squares at the 808 metal ratios, ring-modulated in pairs, then highpassed.
            // The matrix runs at 4x so the ring-mod sum tones do not fold back into the hat band.
            constexpr std::array<float, 6> kBaseHz{{ 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f }};
            const float pitchScale = std::exp2(color * 0.42f) * trackedPitch;
            const double oversampledRate = std::max(4000.0, sampleRate_ * 4.0);
            const float oversampledRateF = static_cast<float>(oversampledRate);

            std::array<float, 4> oversampledBlock{};
            std::ranges::for_each(std::views::iota(size_t{0}, size_t{4}), [&](size_t subIdx) {
                std::array<float, 6> squares{};
                std::ranges::for_each(std::views::iota(size_t{0}, squares.size()), [&](size_t oscIdx) {
                    const float detune = std::exp2(kMetalDetuneCents[channelIdx][oscIdx] / 1200.0f);
                    const float oscHz = std::min(kBaseHz[oscIdx] * pitchScale * detune, oversampledRateF * 0.46f);
                    const float phaseStep = oscHz / oversampledRateF;
                    const float phase = advancePhase(ch.metalPhases[oscIdx], oscHz, oversampledRate);
                    const float halfPhase = phase + 0.5f - std::floor(phase + 0.5f);
                    const float baseWave = phase < 0.5f ? 1.0f : -1.0f;
                    const float blepSquare = baseWave + polyBlep(phase, phaseStep) - polyBlep(halfPhase, phaseStep);
                    squares[oscIdx] = 0.62f * blepSquare;
                });
                const float ringMod = (squares[0] * squares[1] + squares[2] * squares[3] + squares[4] * squares[5])
                    * (0.85f + 0.25f * density);
                oversampledBlock[subIdx] = std::tanh(ringMod * (1.15f + 0.55f * density));
            });

            const float shaped = ch.metalOversampler.downsample(oversampledBlock);
            const float hp = stepSvf(shaped, ch.modelHp, modelHpCoeffs_).highpass;
            const float grit = stepSvf(white, ch.modelBp, modelBpCoeffs_).bandpass;
            // Bright noise following the metal keeps the 808 noisy instead of a low buzz.
            // The squares only color the attack. A sustaining ring-mod is a pitched metal tone.
            ch.ringEnv *= std::exp(-1.0f / (sr * 0.028f));
            const float follow = 0.55f + 0.45f * std::min(1.0f, std::abs(shaped) * 1.4f);
            return hp * 1.7f * ch.ringEnv + grit * 0.95f * follow;
        }
        case NoiseType::BronzeCymbal: {
            // Density above about 0.6 keeps the partials ringing (ride / bell).
            // Below that they die in a few dozen milliseconds and the tail is noise.
            const float ring = std::clamp((density - 0.58f) / 0.30f, 0.0f, 1.0f);
            const float sustainFeed = fullAdsrActive_ ? (0.008f * ring * white) : 0.0f;
            const float modalInput = strikeExcite + sustainFeed;
            float modalSum = 0.0f;
            std::ranges::for_each(std::views::iota(size_t{0}, kBronzeModeCount), [&](size_t modeIdx) {
                const float polarity = (modeIdx % 2 == 0) ? 1.0f : -1.0f;
                const float modeOut = stepSvf(
                    modalInput, ch.bronzeModes[modeIdx], bronzeModeCoeffs_[channelIdx][modeIdx]).resonator;
                modalSum += polarity * modeOut;
            });
            // Equal strike weights. The per-mode damping already darkens the tail.
            const float bells = modalSum * (0.55f / std::sqrt(static_cast<float>(kBronzeModeCount)));
            const float wash = stepSvf(0.80f * white + 0.20f * pink, ch.modelHp, modelHpCoeffs_).highpass;
            return bells * (0.24f + 3.20f * ring) + wash * (0.95f - 0.60f * ring);
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
            const float air = stepSvf(white, ch.modelHp, modelHpCoeffs_).highpass;
            ch.ringEnv *= std::exp(-1.0f / (sr * 0.020f));
            return sum * 0.22f * ch.ringEnv + air * 0.78f;
        }
        case NoiseType::CombWash: {
            const float input = 0.70f * white + 0.30f * pink;
            if (ch.combDelay.empty()) return input;
            const float baseMs = (channelIdx == 0 ? 4.15f : 5.55f) * (1.12f - 0.38f * color);
            const float primaryDelay = std::max(0.25f, baseMs) * 0.001f * sr;
            const float secondaryDelay = std::max(0.25f, baseMs * 0.67f) * 0.001f * sr;
            const float delayed = 0.64f * readLagrange3(ch.combDelay, ch.combWrite, primaryDelay)
                + 0.36f * readLagrange3(ch.combDelay, ch.combWrite, secondaryDelay);

            // Damp the highs inside the loop. Higher feedback then stays a noisy smear, not narrow ringing peaks.
            const float dampHz = std::clamp(3800.0f + 4400.0f * (1.0f - 0.55f * density) + 1600.0f * color,
                                            1400.0f, sr * 0.42f);
            const float dampAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * dampHz / sr));
            ch.combFilter += dampAlpha * (delayed - ch.combFilter);
            const float feedback = 0.34f + 0.48f * density;
            ch.combDelay[ch.combWrite] = ch.combAdaa.process(input + ch.combFilter * feedback);
            ch.combWrite = (ch.combWrite + 1) % ch.combDelay.size();
            const float mixed = 0.16f * input + 1.02f * delayed;
            return stepSvf(mixed, ch.modelHp, modelHpCoeffs_).highpass * 1.10f;
        }
    }
    return white;
}

float HatSynth::processBodyChannel(
    ChannelState& ch, int channelIdx, float shellImpulse, float transientBed, float thump) noexcept {
    constexpr std::array<float, kShellModeCount> kModeWeights{{ +0.44f, -0.32f, +0.23f, -0.16f }};
    float modes = 0.0f;
    std::ranges::for_each(std::views::iota(size_t{0}, kShellModeCount), [&](size_t modeIdx) {
        const float modeOut = stepSvf(
            shellImpulse, ch.bodyModes[modeIdx], bodyModeCoeffs_[channelIdx][modeIdx]).resonator;
        modes += modeOut * kModeWeights[modeIdx];
    });
    const float subAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 340.0 / sampleRate_));
    ch.bodySubHp += subAlpha * (transientBed - ch.bodySubHp);
    const float chest = transientBed - ch.bodySubHp;
    const float warmAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 3400.0 / sampleRate_));
    ch.bodyWarmLp += warmAlpha * (chest - ch.bodyWarmLp);
    return modes * 1.45f + ch.bodyWarmLp * 0.42f + thump * 0.22f;
}

float HatSynth::generateNoiseSample(NoiseType type) noexcept {
    HatSynthSettings singleSettings;
    singleSettings.noiseType = type;
    return generateHissChannel(channels_[0], 0, 0.0f, singleSettings);
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
    const float releaseSamples = std::clamp(settings.ampReleaseMs, 5.0f, kOneShotMaxReleaseMs) * 0.001f * sr;
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
    std::array<float, 2> bounceImpulses{};
    const float decayScale = std::clamp(settings.decayScale, 0.25f, 2.0f);
    const float sr = static_cast<float>(sampleRate_);
    const float burstDecaySamples = std::max(1.0f, settings.preChickDecayMs * decayScale * 0.001f * sr);
    const float mainAge = static_cast<float>(currentSample - mainHitSample_);
    const float trackedPitch = trackedPitchRatio(settings);

    std::ranges::for_each(std::views::iota(0, preChickCount_), [&](int i) {
        const int age = currentSample - burstStartSamples_[i];
        if (age < 0 || static_cast<float>(age) >= burstDecaySamples * 4.0f) return;
        const float envelope = std::exp(-static_cast<float>(age) / burstDecaySamples) * burstAmplitudes_[i];
        const float pan = burstPans_[i];
        const float gainL = std::sqrt(std::clamp(0.5f * (1.0f - pan), 0.0f, 1.0f));
        const float gainR = std::sqrt(std::clamp(0.5f * (1.0f + pan), 0.0f, 1.0f));
        preChickGain[0] += envelope * gainL;
        preChickGain[1] += envelope * gainR;
        // Each bounce also strikes the modal resonators, so they ring from the first contact.
        const float bounceStrike = age == 0 ? burstAmplitudes_[i] * 0.35f : 0.0f;
        bounceImpulses[0] += bounceStrike * gainL;
        bounceImpulses[1] += bounceStrike * gainR;
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
            const float thumpHz = (740.0f + 640.0f * std::clamp(settings.bodyTone, 0.0f, 1.0f)) * trackedPitch;
            chestThump = std::sin(2.0f * pi() * advancePhase(chestPhase_, thumpHz, sampleRate_)) * thumpEnv;
        }
    }

    const float attackEnvelope = mainAge >= 0.0f
        ? std::exp(-mainAge / std::max(1.0f, sr * 0.0018f))
        : 0.0f;
    const float mainStrike = currentSample == mainHitSample_ ? 1.0f : 0.0f;
    const float attackLevel = std::clamp(settings.chickLevel, 0.0f, 1.0f) * 1.15f;
    const float hissColor = std::clamp(settings.hissColor, -1.0f, 1.0f);
    const float tiltAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 4200.0 / sampleRate_));
    const float drive = std::clamp(settings.hissDrive, 0.0f, 1.0f);
    const float driveGain = 1.0f + 2.35f * drive;
    const float driveNorm = 1.0f / std::tanh(driveGain);
    const float bodyMix = std::clamp(settings.bodyLevel, 0.0f, 1.0f);

    std::array<float, 2> airPair{};
    std::array<float, 2> unscaledChickPair{};

    // Process independent left/right channels uniformly through the same synthesis helper chain.
    std::ranges::for_each(std::views::iota(size_t{0}, size_t{2}), [&](size_t chIdx) {
        auto& ch = channels_[chIdx];
        float stickNoise = nextRandom64(ch.prng);
        ch.stickHp += stickHpAlpha_ * (stickNoise - ch.stickHp);
        stickNoise -= ch.stickHp;
        ch.stickLp += stickLpAlpha_ * (stickNoise - ch.stickLp);
        stickNoise = ch.stickLp;
        const float strikeExcite = mainStrike + bounceImpulses[chIdx] + stickNoise * attackEnvelope * 0.055f;
        const float rawTone = generateHissChannel(ch, static_cast<int>(chIdx), strikeExcite, settings);
        // The lowpass closes over the note, the way a real cymbal loses its top.
        const float shadeNow = std::clamp(ch.shade, 0.0f, 1.0f);
        const float cutoff = std::min(1700.0f + 16000.0f * shadeNow, static_cast<float>(sampleRate_) * 0.45f);
        const float darkAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * cutoff / sampleRate_));
        ch.shadeLp += darkAlpha * (rawTone - ch.shadeLp);
        const float rawHiss = ch.shadeLp;
        const float noiseSwell = rawHiss * preChickGain[chIdx] * 0.28f + rawHiss * noiseEnvelope * settings.noiseLevel;

        // The air layer follows filter & tone controls. It only tracks MIDI pitch when key tracking is on.
        float air = stepSvf(noiseSwell, ch.hpf1, nh1Coeffs_).highpass;
        air = stepSvf(air, ch.hpf2, nh2Coeffs_).highpass;
        air = stepSvf(air, ch.lpf1, nl1Coeffs_).lowpass;
        air = stepSvf(air, ch.lpf2, nl2Coeffs_).lowpass;

        ch.tiltLp += tiltAlpha * (air - ch.tiltLp);
        const float airHigh = air - ch.tiltLp;
        air = hissColor >= 0.0f
            ? air + airHigh * (0.45f * hissColor)
            : airHigh * (1.0f + 0.45f * hissColor) + ch.tiltLp * (1.0f - 0.35f * hissColor);

        air = ch.diffusers[1].process(ch.diffusers[0].process(air));

        // Drive only the sizzle band, so the body does not intermodulate with it.
        // The post high-pass removes difference tones that fall onto the shell modes.
        const float airLow = stepSvf(air, ch.sizzleSplit, sizzleSplitCoeffs_).lowpass;
        const float sizzle = air - airLow;
        const float shaped = ch.sizzleAdaa.process(sizzle * driveGain) * driveNorm;
        const float drivenSizzle = stepSvf(shaped, ch.sizzlePostHp, sizzlePostCoeffs_).highpass;
        const float shapedAir = airLow + (drive > 0.001f ? drivenSizzle : sizzle);

        float bronzeBody = 0.0f;
        if (bodyMix > 0.001f) {
            const float warmPink = (ch.pinkB0 + ch.pinkB1) * 0.35f;
            const float shellStrike = strikeExcite * 0.72f + warmPink * bodyEnvelope * 0.004f;
            const float transientBed = (warmPink + stickNoise * 0.45f)
                * (attackEnvelope * 0.85f + preChickGain[chIdx] * 0.25f);
            const float thump = bodyEnvelope > 0.0f ? chestThump : 0.0f;
            bronzeBody = processBodyChannel(ch, static_cast<int>(chIdx), shellStrike, transientBed, thump) * bodyMix;
        }

        airPair[chIdx] = dcBlock(shapedAir + bronzeBody, ch.dcX, ch.dcY);
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
