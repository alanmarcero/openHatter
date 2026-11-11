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

    // Dynamic feedback calculated from decayMs and size
    const float feedback = std::clamp(std::pow(0.001f, 32.0f / std::max(15.0f, decayMs)), 0.15f, 0.94f) * (0.65f + 0.35f * size);
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
        delayLines_[i][writeIndices_[i]] = inSig * 0.7f + dampStates_[i] * feedback;
        writeIndices_[i] = (writeIndices_[i] + 1) % delayLengths_[i];
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

    const float totalTimeMs = settings.preChickSpreadMs * static_cast<float>(preChickCount_)
        + std::max({ settings.noiseDecayMs, settings.toneDecayMs, settings.reverbDecayMs, settings.reverbGateMs });
    totalDurationSamples_ = static_cast<int>((totalTimeMs * 0.001f + 0.08f) * static_cast<float>(sampleRate_));
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
    float metalEnv = 0.0f;
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

        // Metallic cymbal body decay
        metalEnv = std::exp(-t / std::max(1.0f, metalDecaySamples));

        // Main stick chick transient
        chickSample = generateChickSample(settings.chickType, settings.chickFreqHz, mainAge, settings.chickLevel);
    }

    // 3. Tuned Bronze Metallic Cymbal Matrix
    float metalL = 0.0f;
    float metalR = 0.0f;
    const float rawNoiseL = generateNoiseSample(settings.noiseType);
    const float rawNoiseR = generateNoiseSample(settings.noiseType);

    if (mainAge >= 0 && settings.toneLevel > 0.001f) {
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

        float oscL = 0.0f;
        float oscR = 0.0f;

        switch (settings.metalMode) {
            case MetalMode::Classic909: {
                // PolyBLEP bandlimited pulse waves with pairwise ring modulation
                // Produces the authentic, rich inharmonic bronze cluster of the TR-909 without digital aliasing
                std::array<float, 6> pulses{};
                for (size_t i = 0; i < 6; ++i) {
                    pulses[i] = bandlimitedPulse(metalPhases_[i], metalDt_[i], 0.5f);
                }

                // Pairwise cross-ring modulation: (0 x 3), (1 x 4), (2 x 5)
                const float rm1 = pulses[0] * pulses[3];
                const float rm2 = pulses[1] * pulses[4];
                const float rm3 = pulses[2] * pulses[5];

                oscL = (rm1 * 0.35f + rm2 * 0.30f + rm3 * 0.25f + pulses[0] * 0.15f);
                oscR = (rm2 * 0.35f + rm1 * 0.30f + rm3 * 0.25f + pulses[1] * 0.15f);

                // Subtle noise excitation of bronze surface
                oscL = oscL * 0.85f + rawNoiseL * 0.15f;
                oscR = oscR * 0.85f + rawNoiseR * 0.15f;
                break;
            }
            case MetalMode::AnjunaSilk: {
                // Modal resonator cluster with phase dispersion and soft saturation for modern progressive trance
                for (size_t i = 0; i < 6; ++i) {
                    const float s1 = std::sin(2.0f * pi() * metalPhases_[i]);
                    const float s2 = std::sin(4.0f * pi() * metalPhases_[i] + 0.4f);
                    const float silk = std::tanh((s1 + 0.45f * s2) * 1.5f);
                    const float w = 0.22f - 0.015f * static_cast<float>(i);
                    oscL += silk * w;
                    oscR += std::sin(2.0f * pi() * (metalPhases_[i] + 0.06f * static_cast<float>(i))) * w;
                }
                break;
            }
            case MetalMode::UpliftingFMBell: {
                // 3-operator cross-FM bell matrix with dynamic modulation index decay
                // Pierces through massive 138 BPM uplifting trance supersaws with singing bronze clarity
                const float modIndex = 1.6f + 2.0f * pitchEnvFrac;
                const float fm1 = std::sin(2.0f * pi() * metalPhases_[0] + modIndex * std::sin(2.0f * pi() * metalPhases_[3]));
                const float fm2 = std::sin(2.0f * pi() * metalPhases_[1] + (modIndex * 1.2f) * std::sin(2.0f * pi() * metalPhases_[4]));
                const float fm3 = std::sin(2.0f * pi() * metalPhases_[2] + (modIndex * 0.9f) * std::sin(2.0f * pi() * metalPhases_[5]));
                oscL = (fm1 * 0.40f + fm2 * 0.35f + fm3 * 0.25f);
                oscR = (fm1 * 0.35f + fm2 * 0.40f - fm3 * 0.25f);
                break;
            }
            case MetalMode::Analog808Crisp: {
                // Classic 808 asymmetric pulse matrix with analog punch
                constexpr std::array<float, 6> kDuties = { 0.38f, 0.44f, 0.50f, 0.54f, 0.42f, 0.48f };
                for (size_t i = 0; i < 6; ++i) {
                    const float pulse = bandlimitedPulse(metalPhases_[i], metalDt_[i], kDuties[i]);
                    oscL += pulse * ((i % 2 == 0) ? 0.22f : 0.14f);
                    oscR += pulse * ((i % 2 == 1) ? 0.22f : 0.14f);
                }
                break;
            }
        }

        // Resonant cymbal body bandpass filter centered around toneFreqHz
        if (std::abs(settings.toneFreqHz - lastMetalFreq_) > 1.0f || std::abs(settings.toneQ - lastMetalQ_) > 0.05f) {
            lastMetalFreq_ = settings.toneFreqHz;
            lastMetalQ_ = settings.toneQ;
            computeBandpassCoeffs(lastMetalFreq_, lastMetalQ_, sampleRate_, b0_mb_, b1_mb_, b2_mb_, a1_mb_, a2_mb_);
            computeHighpassCoeffs(std::max(800.0f, fundamentalHz * 0.8f), 0.7071f, sampleRate_, b0_mh_, b1_mh_, b2_mh_, a1_mh_, a2_mh_);
        }

        const float bpL = processBiquad(oscL * metalEnv, metalBPF_L_, b0_mb_, b1_mb_, b2_mb_, a1_mb_, a2_mb_);
        const float bpR = processBiquad(oscR * metalEnv, metalBPF_R_, b0_mb_, b1_mb_, b2_mb_, a1_mb_, a2_mb_);

        const float hpL = processBiquad(oscL * metalEnv, metalHPF_L_, b0_mh_, b1_mh_, b2_mh_, a1_mh_, a2_mh_);
        const float hpR = processBiquad(oscR * metalEnv, metalHPF_R_, b0_mh_, b1_mh_, b2_mh_, a1_mh_, a2_mh_);

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

    // Combine dry elements
    const float dryL = airL + metalL + (chickSample + preChickTickL) * velocity_;
    const float dryR = airR + metalR + (chickSample + preChickTickR) * velocity_;

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
