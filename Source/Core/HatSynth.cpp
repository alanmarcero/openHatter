#include "HatSynth.h"
#include <algorithm>
#include <cmath>
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

constexpr std::array<std::array<float, 6>, 4> kModalRatios{{
    {{1.0f, 1.48f, 1.80f, 2.24f, 2.73f, 3.33f}},
    {{1.0f, 1.41f, 2.00f, 2.83f, 4.00f, 5.66f}},
    {{1.0f, 1.50f, 2.25f, 3.00f, 4.50f, 6.75f}},
    {{1.0f, 1.33f, 1.82f, 2.31f, 3.14f, 4.27f}}
}};

constexpr std::array<float, 6> kModalWeights{0.72f, 0.54f, 0.43f, 0.34f, 0.25f, 0.18f};
constexpr std::array<float, 6> kModalDamping{1.00f, 0.72f, 0.56f, 0.42f, 0.31f, 0.24f};

[[nodiscard]] size_t modeIndex(MetalMode mode) noexcept {
    return std::min(static_cast<size_t>(mode), kModalRatios.size() - 1);
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
    modalFrequencies_.fill(0.0f);
    modalPhases_.fill(0.0f);

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
    pinkB0_ = pinkB1_ = pinkB2_ = 0.0f;
    lofiHeld_ = 0.0f;
    lofiCounter_ = 0;

    noiseDiffL_.reset();
    noiseDiffR_.reset();
    reverb_.reset();
}

void HatSynth::trigger(const HatSynthSettings& settings, float velocity, int semitoneOffset) noexcept {
    const float rootHz = noteToHz(settings.note, settings.octave, settings.fineCents);
    triggerAtFrequency(settings, velocity, rootHz * std::pow(2.0f, static_cast<float>(semitoneOffset) / 12.0f));
}

void HatSynth::triggerMidi(const HatSynthSettings& settings, float velocity, int midiNoteNumber) noexcept {
    triggerAtFrequency(settings, velocity, midiNoteToHz(midiNoteNumber));
}

void HatSynth::triggerAtFrequency(const HatSynthSettings& settings, float velocity, float baseFrequencyHz) noexcept {
    reset();
    isPlaying_ = true;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    baseFrequencyHz_ = baseFrequencyHz;
    for (auto& phase : modalPhases_)
        phase = pi() * (nextRandomFloat(prngState_) + 1.0f);

    const auto& ratios = kModalRatios[modeIndex(settings.metalMode)];
    const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
    for (size_t i = 0; i < modalFrequencies_.size(); ++i) {
        const float frequency = baseFrequencyHz_ * ratios[i];
        modalFrequencies_[i] = frequency < nyquist ? frequency : 0.0f;
    }

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
    const float dryTailMs = std::max(settings.noiseAttackMs + settings.noiseHoldMs + settings.noiseDecayMs,
                                     settings.toneDecayMs);
    const float effectTailMs = std::max({settings.reverbDecayMs, settings.reverbGateMs, dryTailMs});
    dryDurationSamples_ = std::max(1, static_cast<int>((preRollMs + dryTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
    totalDurationSamples_ = std::max(dryDurationSamples_,
        static_cast<int>((preRollMs + effectTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
}

float HatSynth::generateNoiseSample(NoiseType type) noexcept {
    const float white = nextRandomFloat(prngState_);

    if (type == NoiseType::PinkSilk) {
        pinkB0_ = 0.99765f * pinkB0_ + white * 0.0990460f;
        pinkB1_ = 0.96300f * pinkB1_ + white * 0.2965164f;
        pinkB2_ = 0.57000f * pinkB2_ + white * 1.0526913f;
        const float pink = (pinkB0_ + pinkB1_ + pinkB2_ + white * 0.1848f) * 0.35f;
        return 0.5f * white + 0.5f * pink;
    }

    if (type == NoiseType::VelvetShimmer) {
        return std::abs(white) > 0.82f ? white * 4.0f : white * 0.22f;
    }

    if (type == NoiseType::LoFiCrunch) {
        if (lofiCounter_ <= 0) {
            lofiHeld_ = std::round(white * 31.0f) / 31.0f;
            lofiCounter_ = 3;
        }
        --lofiCounter_;
        return lofiHeld_;
    }

    return white;
}

float HatSynth::generateChickSample(ChickType type, float freqHz, int ageSamples, float level) noexcept {
    if (ageSamples < 0) return 0.0f;
    const float t = static_cast<float>(ageSamples) / static_cast<float>(sampleRate_);
    const float nyquist = static_cast<float>(sampleRate_) * 0.45f;
    const float clampedFreq = std::clamp(freqHz, 100.0f, nyquist);
    switch (type) {
        case ChickType::CrispStick: {
            if (t > 0.008f) return 0.0f;
            const float env = std::exp(-t / 0.0012f);
            const float click = ageSamples < 8 ? static_cast<float>(8 - ageSamples) * 0.125f : 0.0f;
            const float p1 = 2.0f * pi() * clampedFreq * t;
            const float p2 = 2.0f * pi() * clampedFreq * 1.62f * t;
            return (click * 0.6f + (std::sin(p1) * 0.6f + std::sin(p2) * 0.4f) * env) * level;
        }
        case ChickType::PedalFoot: {
            if (t > 0.018f) return 0.0f;
            const float env = std::exp(-t / 0.0028f);
            const float p1 = 2.0f * pi() * clampedFreq * 0.6f * t;
            const float p2 = 2.0f * pi() * clampedFreq * 1.25f * t;
            return (std::sin(p1) * 0.65f + std::sin(p2) * 0.35f) * env * level * 1.2f;
        }
        case ChickType::LaserTick: {
            if (t > 0.010f) return 0.0f;
            const float env = std::exp(-t / 0.0015f);
            const float sweepHz = clampedFreq * (1.0f + 2.8f * env);
            return std::sin(2.0f * pi() * sweepHz * t) * env * level * 1.25f;
        }
    }
    return 0.0f;
}

float HatSynth::getEnvelopeProgress() const noexcept {
    if (!isPlaying_ || totalDurationSamples_ <= 0) return 1.0f;
    return std::clamp(static_cast<float>(totalSamplesPlayed_) / static_cast<float>(totalDurationSamples_), 0.0f, 1.0f);
}

StereoFrame HatSynth::processSample(const HatSynthSettings& settings) noexcept {
    if (!isPlaying_) return {0.0f, 0.0f};

    const int currentSample = totalSamplesPlayed_++;
    if (totalSamplesPlayed_ >= totalDurationSamples_) {
        isPlaying_ = false;
        return {0.0f, 0.0f};
    }

    float preChickGainL = 0.0f;
    float preChickGainR = 0.0f;
    float preChickTickL = 0.0f;
    float preChickTickR = 0.0f;
    const float burstDecaySamples = std::max(1.0f, settings.preChickDecayMs * 0.001f * static_cast<float>(sampleRate_));
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
        const float tick = generateChickSample(settings.chickType, settings.chickFreqHz, age, burstAmplitudes_[i] * 0.55f);
        preChickTickL += tick * gainL;
        preChickTickR += tick * gainR;
    }

    float noiseEnvelope = 0.0f;
    float chick = 0.0f;
    if (mainAge >= 0.0f) {
        const float attackSamples = std::max(0.0f, settings.noiseAttackMs) * 0.001f * static_cast<float>(sampleRate_);
        const float holdSamples = std::max(0.0f, settings.noiseHoldMs) * 0.001f * static_cast<float>(sampleRate_);
        const float decaySamples = std::max(1.0f, settings.noiseDecayMs * 0.001f * static_cast<float>(sampleRate_));
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
        chick = generateChickSample(settings.chickType, settings.chickFreqHz, static_cast<int>(mainAge), settings.chickLevel);
    }

    const float rawNoiseL = generateNoiseSample(settings.noiseType);
    const float rawNoiseR = generateNoiseSample(settings.noiseType);
    float metalL = 0.0f;
    float metalR = 0.0f;
    if (mainAge >= 0.0f && settings.toneLevel > 0.001f) {
        const float decaySamples = std::max(1.0f, settings.toneDecayMs * 0.001f * static_cast<float>(sampleRate_));
        const float pitchDecaySamples = std::max(1.0f, settings.tonePitchDecayMs * 0.001f * static_cast<float>(sampleRate_));
        const float pitchShift = std::clamp(settings.tonePitchEnv, -24.0f, 24.0f)
            * std::exp(-mainAge / pitchDecaySamples);
        const float brightness = std::clamp((settings.toneFreqHz - 300.0f) / 7700.0f, 0.0f, 1.0f);
        const float focus = std::clamp(settings.toneQ / 12.0f, 0.0f, 1.0f);
        const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
        float sumL = 0.0f;
        float sumR = 0.0f;

        for (size_t i = 0; i < modalFrequencies_.size(); ++i) {
            const float baseModeHz = modalFrequencies_[i];
            if (baseModeHz <= 0.0f) continue;
            const float modeShift = i == 0 ? 1.0f : std::exp2(pitchShift * static_cast<float>(i) / 48.0f);
            const float frequency = baseModeHz * modeShift;
            if (frequency >= nyquist) continue;
            const float phaseStep = frequency / static_cast<float>(sampleRate_);
            modalPhases_[i] += phaseStep;
            if (modalPhases_[i] >= 1.0f) modalPhases_[i] -= 1.0f;

            const float modeDecay = decaySamples * kModalDamping[i] * (0.72f + 0.55f * focus);
            const float envelope = std::exp(-mainAge / std::max(1.0f, modeDecay));
            const float highModeWeight = 0.62f + brightness * 0.76f * static_cast<float>(i) / 5.0f;
            const float focusedWeight = 1.0f - focus * 0.55f * static_cast<float>(i) / 5.0f;
            const float weight = kModalWeights[i] * highModeWeight * focusedWeight;
            const float phase = 2.0f * pi() * modalPhases_[i];
            const float left = std::sin(phase);
            const float right = std::sin(phase + (i == 0 ? 0.0f : 0.035f * static_cast<float>(i)));
            float character = 1.0f;
            switch (settings.metalMode) {
                case MetalMode::Classic909:
                    character = i == 0 ? 1.0f : 0.94f;
                    break;
                case MetalMode::AnjunaSilk:
                    character = 0.82f + 0.18f * (1.0f - static_cast<float>(i) / 5.0f);
                    break;
                case MetalMode::UpliftingFMBell:
                    character = 1.10f - 0.24f * static_cast<float>(i) / 5.0f;
                    break;
                case MetalMode::Analog808Crisp:
                    character = i < 3 ? 1.08f : 0.82f;
                    break;
            }
            sumL += left * envelope * weight * character;
            sumR += right * envelope * weight * character;
        }
        metalL = sumL * settings.toneLevel * velocity_;
        metalR = sumR * settings.toneLevel * velocity_;
    }

    const float noiseSwellL = (rawNoiseL * preChickGainL * 0.4f + rawNoiseL * noiseEnvelope * settings.noiseLevel) * velocity_;
    const float noiseSwellR = (rawNoiseR * preChickGainR * 0.4f + rawNoiseR * noiseEnvelope * settings.noiseLevel) * velocity_;
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
    if (static_cast<float>(currentSample) < safeAttack) {
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

    const float dryL = (airL + metalL + (chick + preChickTickL) * velocity_) * amplitudeEnvelope;
    const float dryR = (airR + metalR + (chick + preChickTickR) * velocity_) * amplitudeEnvelope;
    StereoFrame reverbOut{0.0f, 0.0f};
    if (settings.reverbMix > 0.001f) {
        reverbOut = reverb_.process(dryL, dryR, settings.reverbSize, settings.reverbDecayMs,
                                    settings.reverbDamp, settings.reverbGateMs, sampleRate_);
    }
    return {
        dryL * (1.0f - settings.reverbMix * 0.40f) + reverbOut.left * settings.reverbMix,
        dryR * (1.0f - settings.reverbMix * 0.40f) + reverbOut.right * settings.reverbMix
    };
}

void HatSynth::renderBlock(
    std::vector<float>& leftOut,
    std::vector<float>& rightOut,
    const HatSynthSettings& settings,
    float velocity,
    int semitoneOffset) noexcept {
    trigger(settings, velocity, semitoneOffset);
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
    triggerMidi(settings, velocity, midiNoteNumber);
    leftOut.resize(totalDurationSamples_);
    rightOut.resize(totalDurationSamples_);
    for (int i = 0; i < totalDurationSamples_; ++i) {
        const auto frame = processSample(settings);
        leftOut[static_cast<size_t>(i)] = frame.left;
        rightOut[static_cast<size_t>(i)] = frame.right;
    }
}

} // namespace openhatter::core
