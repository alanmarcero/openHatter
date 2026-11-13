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

// PolyBLEP anti-aliasing residual for step discontinuities
[[nodiscard]] inline float polyBlep(float t, float dt) noexcept {
    if (dt <= 0.0f) return 0.0f;
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Bandlimited pulse oscillator with variable duty cycle and PolyBLEP
[[nodiscard]] inline float bandlimitedPulse(float phase, float dt, float duty = 0.5f) noexcept {
    float naive = (phase < duty) ? 1.0f : -1.0f;
    naive += polyBlep(phase, dt);
    naive -= polyBlep(std::fmod(phase + (1.0f - duty), 1.0f), dt);
    return naive;
}

// Biquad bandpass filter coefficients (constant 0 dB peak gain)
void computeBandpassCoeffs(float freqHz, float q, double sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.49f;
    const float clampedFreq = std::clamp(freqHz, 20.0f, nyquist);
    const float clampedQ = std::clamp(q, 0.2f, 25.0f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float alpha = std::sin(w0) / (2.0f * clampedQ);
    const float cosW0 = std::cos(w0);

    const float a0 = 1.0f + alpha;
    b0 = alpha / a0;
    b1 = 0.0f;
    b2 = -alpha / a0;
    a1 = (-2.0f * cosW0) / a0;
    a2 = (1.0f - alpha) / a0;
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

// Inharmonic circular plate Bessel vibration modes for bronze cymbals
constexpr std::array<float, 6> kBronzeRatios = {
    1.0000f, 1.4826f, 1.8015f, 2.2453f, 2.7314f, 3.3289f
};

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

    metalPhases_.fill(0.0f);
    metalDt_.fill(0.0f);

    metalBPF_L_.reset();
    metalBPF_R_.reset();
    metalHPF_L_.reset();
    metalHPF_R_.reset();

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
    lastMetalFreq_ = -1.0f;
    lastMetalQ_ = -1.0f;

    pinkB0_ = pinkB1_ = pinkB2_ = 0.0f;
    lofiHeld_ = 0.0f;
    lofiCounter_ = 0;

    noiseDiffL_.reset();
    noiseDiffR_.reset();
    reverb_.reset();
}

void HatSynth::trigger(const HatSynthSettings& settings, float velocity, int semitoneOffset) noexcept {
    reset();
    isPlaying_ = true;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    semitoneOffset_ = semitoneOffset;
    for (auto& phase : metalPhases_)
        phase = 0.5f * (nextRandomFloat(prngState_) + 1.0f);

    preChickCount_ = std::clamp(settings.preChickCount, 1, 5);
    const float spreadSamples = (settings.preChickSpreadMs * 0.001f) * static_cast<float>(sampleRate_);

    for (int i = 0; i < preChickCount_; ++i) {
        const float jitterTime = (nextRandomFloat(prngState_) * 0.5f) * settings.preChickJitter * spreadSamples;
        burstStartSamples_[i] = std::max(0, static_cast<int>(static_cast<float>(i) * spreadSamples + jitterTime));

        // Slightly attenuate earlier bounces; the final impact is at full power
        const float progression = (preChickCount_ > 1) ? (0.65f + 0.35f * static_cast<float>(i) / static_cast<float>(preChickCount_ - 1)) : 1.0f;
        const float jitterAmp = 1.0f + nextRandomFloat(prngState_) * 0.15f * settings.preChickJitter;
        burstAmplitudes_[i] = std::clamp(settings.preChickLevel * progression * jitterAmp, 0.05f, 1.0f);

        // Micro-flam panning
        const float panDir = (i % 2 == 0) ? -1.0f : 1.0f;
        const float panFrac = (preChickCount_ > 1) ? static_cast<float>(i + 1) / static_cast<float>(preChickCount_) : 0.4f;
        burstPans_[i] = std::clamp(panDir * settings.preChickFlamSpread * panFrac, -1.0f, 1.0f);
    }

    mainHitSample_ = (preChickCount_ > 1)
        ? static_cast<int>(static_cast<float>(preChickCount_ - 1) * spreadSamples)
        : 0;

    const float preRollMs = settings.preChickSpreadMs * static_cast<float>(preChickCount_);
    const float dryTailMs = std::max(settings.noiseAttackMs + settings.noiseHoldMs + settings.noiseDecayMs,
                                     settings.toneDecayMs);
    const float effectTailMs = std::max({ settings.reverbDecayMs, settings.reverbGateMs, dryTailMs });
    dryDurationSamples_ = std::max(1, static_cast<int>((preRollMs + dryTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
    totalDurationSamples_ = std::max(dryDurationSamples_, static_cast<int>((preRollMs + effectTailMs + 0.08f) * 0.001f * static_cast<float>(sampleRate_)));
}

float HatSynth::generateNoiseSample(NoiseType type) noexcept {
    const float white = nextRandomFloat(prngState_);

    if (type == NoiseType::PinkSilk) {
        // Kellet 3-pole pinking filter with smooth, luxurious top-end sheen
        pinkB0_ = 0.99765f * pinkB0_ + white * 0.0990460f;
        pinkB1_ = 0.96300f * pinkB1_ + white * 0.2965164f;
        pinkB2_ = 0.57000f * pinkB2_ + white * 1.0526913f;
        const float pink = (pinkB0_ + pinkB1_ + pinkB2_ + white * 0.1848f) * 0.35f;
        return 0.5f * white + 0.5f * pink;
    }

    if (type == NoiseType::VelvetShimmer) {
        // High-density velvet noise: randomized bipolar impulses for transparent, silky shimmer
        const float r = nextRandomFloat(prngState_);
        if (std::abs(r) > 0.40f) {
            return (r > 0.0f ? 1.0f : -1.0f) * (0.8f + 0.2f * std::abs(white));
        }
        return white * 0.15f;
    }

    if (type == NoiseType::LoFiCrunch) {
        // Vintage 909 6-bit PCM character with non-linear quantization
        if (++lofiCounter_ >= 2) {
            lofiCounter_ = 0;
            lofiHeld_ = std::round(white * 31.0f) / 31.0f;
        }
        return lofiHeld_;
    }

    // CrispWhite: High-sheen colored noise
    return white;
}

float HatSynth::generateChickSample(ChickType type, float freqHz, int ageSamples, float level) noexcept {
    if (ageSamples < 0) return 0.0f;

    const float t = static_cast<float>(ageSamples) / static_cast<float>(sampleRate_);
    const float clampedFreq = std::clamp(freqHz, 500.0f, static_cast<float>(sampleRate_) * 0.45f);

    switch (type) {
        case ChickType::CrispStick: {
            // Snappy wooden stick contact: rapid click + resonant bandpass ping
            if (t > 0.008f) return 0.0f;
            const float env = std::exp(-t / 0.0012f);
            const float click = (ageSamples < 8) ? ((8 - ageSamples) * 0.125f) : 0.0f;
            const float p1 = 2.0f * pi() * clampedFreq * t;
            const float p2 = 2.0f * pi() * (clampedFreq * 1.62f) * t;
            return (click * 0.6f + (std::sin(p1) * 0.6f + std::sin(p2) * 0.4f) * env) * level;
        }
        case ChickType::PedalFoot: {
            // Mechanical dual-cymbal contact: heavier body punch with low-mid focus
            if (t > 0.018f) return 0.0f;
            const float env = std::exp(-t / 0.0028f);
            const float p1 = 2.0f * pi() * (clampedFreq * 0.6f) * t;
            const float p2 = 2.0f * pi() * (clampedFreq * 1.25f) * t;
            return (std::sin(p1) * 0.65f + std::sin(p2) * 0.35f) * env * level * 1.2f;
        }
        case ChickType::LaserTick: {
            // Fast downward exponential frequency sweep for modern EDM snap
            if (t > 0.010f) return 0.0f;
            const float env = std::exp(-t / 0.0015f);
            const float sweepHz = clampedFreq * (1.0f + 2.8f * env);
            const float p = 2.0f * pi() * sweepHz * t;
            return std::sin(p) * env * level * 1.25f;
        }
    }
    return 0.0f;
}

float HatSynth::getEnvelopeProgress() const noexcept {
    if (!isPlaying_ || totalDurationSamples_ <= 0) {
        return 1.0f;
    }
    return std::clamp(static_cast<float>(totalSamplesPlayed_) / static_cast<float>(totalDurationSamples_), 0.0f, 1.0f);
}

StereoFrame HatSynth::processSample(const HatSynthSettings& settings) noexcept {
    if (!isPlaying_) {
        return { 0.0f, 0.0f };
    }

    const int currentSample = totalSamplesPlayed_++;
    if (totalSamplesPlayed_ >= totalDurationSamples_) {
        isPlaying_ = false;
        return { 0.0f, 0.0f };
    }

    // 1. Evaluate Stick Pre-Chick Bounces with stereo micro-panning
    float preChickGainL = 0.0f;
    float preChickGainR = 0.0f;
    float preChickTickL = 0.0f;
    float preChickTickR = 0.0f;
    const float effectiveBurstDecayMs = std::max(2.0f, std::min(settings.preChickDecayMs, settings.preChickSpreadMs * 0.8f));
    const float burstDecaySamples = (effectiveBurstDecayMs * 0.001f) * static_cast<float>(sampleRate_);

    for (int i = 0; i < preChickCount_; ++i) {
        const int age = currentSample - burstStartSamples_[i];
        if (age >= 0 && static_cast<float>(age) < burstDecaySamples * 4.0f) {
            const float env = std::exp(-static_cast<float>(age) / std::max(1.0f, burstDecaySamples));
            const float pan = burstPans_[i];
            const float gainL = std::clamp(0.5f * (1.0f - pan), 0.0f, 1.0f);
            const float gainR = std::clamp(0.5f * (1.0f + pan), 0.0f, 1.0f);
            const float amp = env * burstAmplitudes_[i];
            preChickGainL += amp * gainL * 1.414f;
            preChickGainR += amp * gainR * 1.414f;

            // Micro-impact tick on each bounce
            const float tick = generateChickSample(settings.chickType, settings.chickFreqHz, age, burstAmplitudes_[i] * 0.6f);
            preChickTickL += tick * gainL;
            preChickTickR += tick * gainR;
        }
    }

    // 2. Evaluate Main Open Hi-Hat Envelopes
    const int mainAge = currentSample - mainHitSample_;
    float noiseEnv = 0.0f;
    float chickSample = 0.0f;

    if (mainAge >= 0) {
        const float attackSamples = (settings.noiseAttackMs * 0.001f) * static_cast<float>(sampleRate_);
        const float holdSamples = (settings.noiseHoldMs * 0.001f) * static_cast<float>(sampleRate_);
        const float decaySamples = (settings.noiseDecayMs * 0.001f) * static_cast<float>(sampleRate_);
        const float metalDecaySamples = (settings.toneDecayMs * 0.001f) * static_cast<float>(sampleRate_);

        const float t = static_cast<float>(mainAge);
        if (t < attackSamples && attackSamples > 0.0f) {
            // Smooth cosine attack ramp
            const float ramp = 0.5f * (1.0f - std::cos(pi() * t / attackSamples));
            noiseEnv = ramp;
        } else if (t < (attackSamples + holdSamples)) {
            noiseEnv = 1.0f;
        } else {
            const float decayT = t - (attackSamples + holdSamples);
            const float linearFraction = std::clamp(decayT / std::max(1.0f, decaySamples), 0.0f, 1.0f);
            const float expDecay = std::exp(-decayT / std::max(1.0f, decaySamples * 0.35f));

            if (settings.noiseCurve < 0.0f) {
                // Negative curve: steep, punchy club decay
                const float factor = -settings.noiseCurve;
                noiseEnv = (1.0f - factor) * (1.0f - linearFraction) + factor * expDecay;
            } else {
                // Positive curve: sustained open shimmer
                const float factor = settings.noiseCurve;
                noiseEnv = (1.0f - factor) * expDecay + factor * (1.0f - linearFraction);
            }
        }

        // Main stick chick transient
        chickSample = generateChickSample(settings.chickType, settings.chickFreqHz, mainAge, settings.chickLevel);
    }

    // 3. Tuned Bronze Metallic Cymbal Matrix
    float metalL = 0.0f;
    float metalR = 0.0f;
    const float rawNoiseL = generateNoiseSample(settings.noiseType);
    const float rawNoiseR = generateNoiseSample(settings.noiseType);

    if (mainAge >= 0 && settings.toneLevel > 0.001f) {
        const float metalDecaySamples = settings.toneDecayMs * 0.001f * static_cast<float>(sampleRate_);
        const float pitchDecaySamples = (settings.tonePitchDecayMs * 0.001f) * static_cast<float>(sampleRate_);
        const float pitchEnvFrac = std::exp(-static_cast<float>(mainAge) / std::max(1.0f, pitchDecaySamples));
        const float pitchModSemitones = static_cast<float>(semitoneOffset_) + settings.tonePitchEnv * pitchEnvFrac;

        // Base tuned fundamental from Note, Octave, and Fine cents
        const float baseHz = noteToHz(settings.note, settings.octave, settings.fineCents);
        const float fundamentalHz = std::clamp(
            baseHz * std::pow(2.0f, pitchModSemitones / 12.0f),
            60.0f,
            static_cast<float>(sampleRate_) * 0.42f);

        // Advance 6 PolyBLEP inharmonic bronze oscillators
        for (size_t i = 0; i < 6; ++i) {
            const float oscHz = std::min(fundamentalHz * kBronzeRatios[i], static_cast<float>(sampleRate_) * 0.46f);
            metalDt_[i] = oscHz / static_cast<float>(sampleRate_);
            metalPhases_[i] += metalDt_[i];
            if (metalPhases_[i] >= 1.0f) {
                metalPhases_[i] -= 1.0f;
            }
        }

        // A struck, inharmonic mode bank replaces the old phase-locked oscillator
        // cluster. Each partial starts at an independent phase and damps at a
        // different rate, so the body opens with a broadband metallic transient
        // and settles into the selected tuned colour.
        constexpr std::array<float, 6> kPartialWeights { 0.44f, 0.28f, 0.19f, 0.14f, 0.10f, 0.07f };
        constexpr std::array<float, 6> kPartialDamping { 0.88f, 0.68f, 0.52f, 0.39f, 0.30f, 0.23f };
        constexpr std::array<float, 6> kPulseDuties { 0.38f, 0.44f, 0.50f, 0.54f, 0.42f, 0.48f };
        std::array<float, 6> partials{};
        const float modalAge = static_cast<float>(mainAge);
        const float safeDecay = std::max(1.0f, metalDecaySamples);
        for (size_t i = 0; i < partials.size(); ++i) {
            const float envelope = std::exp(-modalAge / (safeDecay * kPartialDamping[i]));
            const float phase = 2.0f * pi() * metalPhases_[i];
            const float sine = std::sin(phase);
            const float upper = std::sin(phase * 2.0f + 0.17f * static_cast<float>(i));
            switch (settings.metalMode) {
                case MetalMode::Classic909:
                    partials[i] = (0.72f * bandlimitedPulse(metalPhases_[i], metalDt_[i], 0.5f)
                                   + 0.28f * sine) * envelope;
                    break;
                case MetalMode::AnjunaSilk:
                    partials[i] = std::tanh((sine + 0.28f * upper) * 1.24f) * envelope;
                    break;
                case MetalMode::UpliftingFMBell: {
                    const size_t modulator = (i + 3) % partials.size();
                    const float modulation = (1.25f + 1.5f * pitchEnvFrac) * std::sin(2.0f * pi() * metalPhases_[modulator]);
                    partials[i] = std::sin(phase + modulation) * envelope;
                    break;
                }
                case MetalMode::Analog808Crisp:
                    partials[i] = (0.82f * bandlimitedPulse(metalPhases_[i], metalDt_[i], kPulseDuties[i])
                                   + 0.18f * sine) * envelope;
                    break;
            }
        }

        float oscL = 0.0f;
        float oscR = 0.0f;
        for (size_t i = 0; i < partials.size(); ++i) {
            const float weight = kPartialWeights[i];
            oscL += partials[i] * weight;
            const float stereoPhase = 0.013f * static_cast<float>(i + 1);
            const float rightTone = settings.metalMode == MetalMode::AnjunaSilk
                ? std::sin(2.0f * pi() * (metalPhases_[i] + stereoPhase))
                : partials[i];
            oscR += rightTone * weight;
        }

        if (settings.metalMode == MetalMode::Classic909) {
            oscL += rawNoiseL * 0.035f;
            oscR += rawNoiseR * 0.035f;
        } else if (settings.metalMode == MetalMode::UpliftingFMBell) {
            const float bellBody = std::sin(2.0f * pi() * metalPhases_[0] + 2.2f * std::sin(2.0f * pi() * metalPhases_[4]));
            oscL = 0.72f * oscL + 0.28f * bellBody;
            oscR = 0.72f * oscR - 0.28f * bellBody;
        }

        // Resonant cymbal body bandpass filter centered around toneFreqHz
        if (std::abs(settings.toneFreqHz - lastMetalFreq_) > 1.0f || std::abs(settings.toneQ - lastMetalQ_) > 0.05f) {
            lastMetalFreq_ = settings.toneFreqHz;
            lastMetalQ_ = settings.toneQ;
            computeBandpassCoeffs(lastMetalFreq_, lastMetalQ_, sampleRate_, b0_mb_, b1_mb_, b2_mb_, a1_mb_, a2_mb_);
            computeHighpassCoeffs(std::max(800.0f, fundamentalHz * 0.8f), 0.7071f, sampleRate_, b0_mh_, b1_mh_, b2_mh_, a1_mh_, a2_mh_);
        }

        const float bpL = processBiquad(oscL, metalBPF_L_, b0_mb_, b1_mb_, b2_mb_, a1_mb_, a2_mb_);
        const float bpR = processBiquad(oscR, metalBPF_R_, b0_mb_, b1_mb_, b2_mb_, a1_mb_, a2_mb_);

        const float hpL = processBiquad(oscL, metalHPF_L_, b0_mh_, b1_mh_, b2_mh_, a1_mh_, a2_mh_);
        const float hpR = processBiquad(oscR, metalHPF_R_, b0_mh_, b1_mh_, b2_mh_, a1_mh_, a2_mh_);

        metalL = (bpL * 1.25f + hpL * 0.45f) * settings.toneLevel * velocity_;
        metalR = (bpR * 1.25f + hpR * 0.45f) * settings.toneLevel * velocity_;
    }

    // 4. Air Sizzle & Shimmer Noise Stage (4-pole 24dB/octave filtering)
    const float noiseSwellL = (rawNoiseL * preChickGainL * 0.4f + rawNoiseL * noiseEnv * settings.noiseLevel) * velocity_;
    const float noiseSwellR = (rawNoiseR * preChickGainR * 0.4f + rawNoiseR * noiseEnv * settings.noiseLevel) * velocity_;

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

    // 24 dB/oct steep highpass
    float airL = processBiquad(noiseSwellL, noiseHPF1_L_, b0_nh1_, b1_nh1_, b2_nh1_, a1_nh1_, a2_nh1_);
    airL = processBiquad(airL, noiseHPF2_L_, b0_nh2_, b1_nh2_, b2_nh2_, a1_nh2_, a2_nh2_);

    float airR = processBiquad(noiseSwellR, noiseHPF1_R_, b0_nh1_, b1_nh1_, b2_nh1_, a1_nh1_, a2_nh1_);
    airR = processBiquad(airR, noiseHPF2_R_, b0_nh2_, b1_nh2_, b2_nh2_, a1_nh2_, a2_nh2_);

    // 24 dB/oct steep lowpass
    airL = processBiquad(airL, noiseLPF1_L_, b0_nl1_, b1_nl1_, b2_nl1_, a1_nl1_, a2_nl1_);
    airL = processBiquad(airL, noiseLPF2_L_, b0_nl2_, b1_nl2_, b2_nl2_, a1_nl2_, a2_nl2_);

    airR = processBiquad(airR, noiseLPF1_R_, b0_nl1_, b1_nl1_, b2_nl1_, a1_nl1_, a2_nl1_);
    airR = processBiquad(airR, noiseLPF2_R_, b0_nl2_, b1_nl2_, b2_nl2_, a1_nl2_, a2_nl2_);

    // Diffuse stereo air for silk textures
    if (settings.noiseType == NoiseType::PinkSilk || settings.noiseType == NoiseType::VelvetShimmer) {
        airL = noiseDiffL_.process(airL);
        airR = noiseDiffR_.process(airR);
    }

    // 5. Stereo Width (Phase-coherent Mid/Side)
    if (settings.noiseStereoWidth != 1.0f) {
        const float mid = (airL + airR) * 0.5f;
        const float side = (airL - airR) * 0.5f * settings.noiseStereoWidth;
        airL = mid + side;
        airR = mid - side;
    }

    // Apply the global one-shot envelope to the generated hit before the space bus.
    const float dryAge = static_cast<float>(currentSample);
    const float dryDuration = static_cast<float>(dryDurationSamples_);
    const float attackSamples = std::clamp(settings.ampAttackMs, 0.05f, 20.0f) * 0.001f * static_cast<float>(sampleRate_);
    const float releaseSamples = std::clamp(settings.ampReleaseMs, 5.0f, 800.0f) * 0.001f * static_cast<float>(sampleRate_);
    const float safeAttack = std::min(attackSamples, dryDuration * 0.30f);
    const float safeRelease = std::min(releaseSamples, dryDuration * 0.35f);
    const float decaySamples = std::clamp(settings.ampDecayMs, 1.0f, 600.0f) * 0.001f * static_cast<float>(sampleRate_);
    const float safeDecay = std::min(decaySamples, std::max(0.0f, dryDuration - safeAttack - safeRelease));
    const float sustain = std::clamp(settings.ampSustain, 0.0f, 1.0f);
    const float releaseStart = std::max(safeAttack + safeDecay, dryDuration - safeRelease);
    float ampEnvelope = 0.0f;
    if (dryAge < safeAttack) {
        ampEnvelope = dryAge / std::max(1.0f, safeAttack);
    } else if (dryAge < safeAttack + safeDecay) {
        const float decayPhase = (dryAge - safeAttack) / std::max(1.0f, safeDecay);
        ampEnvelope = 1.0f + (sustain - 1.0f) * decayPhase;
    } else if (dryAge < releaseStart) {
        ampEnvelope = sustain;
    } else if (dryAge < dryDuration) {
        ampEnvelope = sustain * (dryDuration - dryAge) / std::max(1.0f, dryDuration - releaseStart);
    }

    const float dryL = (airL + metalL + (chickSample + preChickTickL) * velocity_) * ampEnvelope;
    const float dryR = (airR + metalR + (chickSample + preChickTickR) * velocity_) * ampEnvelope;

    // 6. Gated Shimmer Reverb Tail
    StereoFrame reverbOut = { 0.0f, 0.0f };
    if (settings.reverbMix > 0.001f) {
        reverbOut = reverb_.process(
            dryL, dryR,
            settings.reverbSize,
            settings.reverbDecayMs,
            settings.reverbDamp,
            settings.reverbGateMs,
            sampleRate_);
    }

    const float outL = dryL * (1.0f - settings.reverbMix * 0.40f) + reverbOut.left * settings.reverbMix;
    const float outR = dryR * (1.0f - settings.reverbMix * 0.40f) + reverbOut.right * settings.reverbMix;

    return { outL, outR };
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
        leftOut[i] = frame.left;
        rightOut[i] = frame.right;
    }
}

} // namespace openhatter::core
