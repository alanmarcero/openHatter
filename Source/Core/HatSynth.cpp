#include "HatSynth.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace openhatter::core {

namespace {

[[nodiscard]] constexpr float pi() noexcept {
    return std::numbers::pi_v<float>;
}

// Fast xorshift32 PRNG
[[nodiscard]] inline float nextRandomFloat(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<float>(state) * (2.0f / 4294967296.0f)) - 1.0f;
}

// Biquad bandpass filter coefficients (constant 0 dB peak gain)
void computeBandpassCoeffs(float freqHz, float q, double sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.5f;
    const float clampedFreq = std::clamp(freqHz, 20.0f, nyquist * 0.95f);
    const float clampedQ = std::clamp(q, 0.1f, 30.0f);
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

// Biquad highpass filter coefficients (12 dB/oct)
void computeHighpassCoeffs(float freqHz, double sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.5f;
    const float clampedFreq = std::clamp(freqHz, 10.0f, nyquist * 0.95f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) * 0.70710678f;

    const float a0 = 1.0f + alpha;
    b0 = ((1.0f + cosW0) * 0.5f) / a0;
    b1 = (-(1.0f + cosW0)) / a0;
    b2 = ((1.0f + cosW0) * 0.5f) / a0;
    a1 = (-2.0f * cosW0) / a0;
    a2 = (1.0f - alpha) / a0;
}

// Biquad lowpass filter coefficients (12 dB/oct)
void computeLowpassCoeffs(float freqHz, double sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) noexcept {
    const float nyquist = static_cast<float>(sampleRate) * 0.5f;
    const float clampedFreq = std::clamp(freqHz, 20.0f, nyquist * 0.95f);
    const float w0 = 2.0f * pi() * clampedFreq / static_cast<float>(sampleRate);
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) * 0.70710678f;

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

// Non-harmonic bronze cymbal frequency ratios inspired by TR-909 / 808 / acoustic Zildjian partials
constexpr std::array<float, 6> kMetallicRatios = {
    1.0000f, 1.3420f, 1.6180f, 1.9285f, 2.5142f, 3.1970f
};

} // namespace

// ============================================================================
// GatedStereoReverb
// ============================================================================

float GatedStereoReverb::readHermite(const std::vector<float>& buffer, double delaySamples, int writePos) noexcept {
    const double size = static_cast<double>(buffer.size());
    double readPos = static_cast<double>(writePos) - delaySamples;
    while (readPos < 0.0) readPos += size;
    while (readPos >= size) readPos -= size;

    const int i1 = static_cast<int>(readPos);
    const float frac = static_cast<float>(readPos - static_cast<double>(i1));
    const int i0 = (i1 - 1 + static_cast<int>(buffer.size())) % static_cast<int>(buffer.size());
    const int i2 = (i1 + 1) % static_cast<int>(buffer.size());
    const int i3 = (i1 + 2) % static_cast<int>(buffer.size());

    const float y0 = buffer[static_cast<size_t>(i0)];
    const float y1 = buffer[static_cast<size_t>(i1)];
    const float y2 = buffer[static_cast<size_t>(i2)];
    const float y3 = buffer[static_cast<size_t>(i3)];

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void GatedStereoReverb::prepare(double sampleRate) {
    diffL1_.prepare(std::max(8, static_cast<int>(sampleRate * 0.0029)), 0.65f);
    diffL2_.prepare(std::max(8, static_cast<int>(sampleRate * 0.0073)), 0.60f);
    diffL3_.prepare(std::max(8, static_cast<int>(sampleRate * 0.0109)), 0.55f);
    diffR1_.prepare(std::max(8, static_cast<int>(sampleRate * 0.0023)), 0.65f);
    diffR2_.prepare(std::max(8, static_cast<int>(sampleRate * 0.0061)), 0.60f);
    diffR3_.prepare(std::max(8, static_cast<int>(sampleRate * 0.0097)), 0.55f);

    const int margin = static_cast<int>(sampleRate * 0.003);
    delayL1_.assign(std::max(16, static_cast<int>(sampleRate * 0.0263)) + margin, 0.0f);
    delayL2_.assign(std::max(16, static_cast<int>(sampleRate * 0.0347)) + margin, 0.0f);
    delayL3_.assign(std::max(16, static_cast<int>(sampleRate * 0.0419)) + margin, 0.0f);
    delayR1_.assign(std::max(16, static_cast<int>(sampleRate * 0.0289)) + margin, 0.0f);
    delayR2_.assign(std::max(16, static_cast<int>(sampleRate * 0.0383)) + margin, 0.0f);
    delayR3_.assign(std::max(16, static_cast<int>(sampleRate * 0.0461)) + margin, 0.0f);
    reset();
}

void GatedStereoReverb::reset() noexcept {
    diffL1_.reset();
    diffL2_.reset();
    diffL3_.reset();
    diffR1_.reset();
    diffR2_.reset();
    diffR3_.reset();
    std::fill(delayL1_.begin(), delayL1_.end(), 0.0f);
    std::fill(delayL2_.begin(), delayL2_.end(), 0.0f);
    std::fill(delayL3_.begin(), delayL3_.end(), 0.0f);
    std::fill(delayR1_.begin(), delayR1_.end(), 0.0f);
    std::fill(delayR2_.begin(), delayR2_.end(), 0.0f);
    std::fill(delayR3_.begin(), delayR3_.end(), 0.0f);
    writeL1_ = writeL2_ = writeL3_ = 0;
    writeR1_ = writeR2_ = writeR3_ = 0;
    dampStateL1_ = dampStateL2_ = 0.0f;
    dampStateR1_ = dampStateR2_ = 0.0f;
    hpfStateL_ = hpfStateR_ = 0.0f;
    lfoPhaseL_ = 0.0;
    lfoPhaseR_ = 0.25;
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
    if (delayL1_.empty()) {
        return { inLeft, inRight };
    }

    sampleCounter_++;

    if (gateMs > 0.0f) {
        const float gateSamples = (gateMs * 0.001f) * static_cast<float>(sampleRate);
        const float releaseSamples = (18.0f * 0.001f) * static_cast<float>(sampleRate);
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

    // Input highpass filter (220 Hz) to keep hi-hat shimmer airy and mud-free
    const float hpfCoeff = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 220.0 / sampleRate));
    hpfStateL_ += hpfCoeff * (inLeft - hpfStateL_);
    const float cleanInL = inLeft - hpfStateL_;
    hpfStateR_ += hpfCoeff * (inRight - hpfStateR_);
    const float cleanInR = inRight - hpfStateR_;

    const float diffInL = diffL3_.process(diffL2_.process(diffL1_.process(cleanInL)));
    const float diffInR = diffR3_.process(diffR2_.process(diffR1_.process(cleanInR)));

    const float feedback = std::clamp(std::pow(0.001f, 30.0f / std::max(10.0f, decayMs)), 0.1f, 0.94f) * (0.6f + 0.4f * size);
    const float dampFactor = std::clamp(damp, 0.0f, 0.85f);

    lfoPhaseL_ += 2.0 * pi() * 0.81 / sampleRate;
    if (lfoPhaseL_ > 2.0 * pi()) lfoPhaseL_ -= 2.0 * pi();
    lfoPhaseR_ += 2.0 * pi() * 0.97 / sampleRate;
    if (lfoPhaseR_ > 2.0 * pi()) lfoPhaseR_ -= 2.0 * pi();

    const double modL = std::sin(lfoPhaseL_) * (0.0007 * sampleRate);
    const double modR = std::cos(lfoPhaseR_) * (0.0007 * sampleRate);

    const double baseL1 = sampleRate * 0.0263;
    const double baseL2 = sampleRate * 0.0347;
    const double baseL3 = sampleRate * 0.0419;
    const double baseR1 = sampleRate * 0.0289;
    const double baseR2 = sampleRate * 0.0383;
    const double baseR3 = sampleRate * 0.0461;

    const float outL1 = readHermite(delayL1_, baseL1 + modL, writeL1_);
    const float outL2 = readHermite(delayL2_, baseL2 - modR, writeL2_);
    const float outL3 = readHermite(delayL3_, baseL3 + modR * 0.5, writeL3_);

    const float outR1 = readHermite(delayR1_, baseR1 + modR, writeR1_);
    const float outR2 = readHermite(delayR2_, baseR2 - modL, writeR2_);
    const float outR3 = readHermite(delayR3_, baseR3 - modL * 0.5, writeR3_);

    const float wetL = 0.35f * (outL1 + outL2 + outL3);
    const float wetR = 0.35f * (outR1 + outR2 + outR3);

    dampStateL1_ = dampStateL1_ * dampFactor + wetL * (1.0f - dampFactor);
    dampStateR1_ = dampStateR1_ * dampFactor + wetR * (1.0f - dampFactor);

    delayL1_[static_cast<size_t>(writeL1_)] = diffInL + dampStateR1_ * feedback;
    delayL2_[static_cast<size_t>(writeL2_)] = diffInL * 0.65f + dampStateL1_ * feedback * 0.75f;
    delayL3_[static_cast<size_t>(writeL3_)] = diffInL * 0.5f + (dampStateR1_ - dampStateL1_) * feedback * 0.45f;

    delayR1_[static_cast<size_t>(writeR1_)] = diffInR + dampStateL1_ * feedback;
    delayR2_[static_cast<size_t>(writeR2_)] = diffInR * 0.65f + dampStateR1_ * feedback * 0.75f;
    delayR3_[static_cast<size_t>(writeR3_)] = diffInR * 0.5f + (dampStateL1_ - dampStateR1_) * feedback * 0.45f;

    writeL1_ = (writeL1_ + 1) % static_cast<int>(delayL1_.size());
    writeL2_ = (writeL2_ + 1) % static_cast<int>(delayL2_.size());
    writeL3_ = (writeL3_ + 1) % static_cast<int>(delayL3_.size());
    writeR1_ = (writeR1_ + 1) % static_cast<int>(delayR1_.size());
    writeR2_ = (writeR2_ + 1) % static_cast<int>(delayR2_.size());
    writeR3_ = (writeR3_ + 1) % static_cast<int>(delayR3_.size());

    const float outLeft = (wetL - wetR * 0.25f) * gateEnvelope_;
    const float outRight = (wetR - wetL * 0.25f) * gateEnvelope_;
    return { outLeft, outRight };
}

// ============================================================================
// HatSynth
// ============================================================================

void HatSynth::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    reverb_.prepare(sampleRate);
    const int maxDelay = static_cast<int>(sampleRate * 0.05);
    widthDelayBuffer_.assign(std::max(64, maxDelay), 0.0f);
    reset();
}

void HatSynth::reset() noexcept {
    isPlaying_ = false;
    totalSamplesPlayed_ = 0;
    totalDurationSamples_ = 0;
    metalPhases_.fill(0.0f);
    lastHPF_ = -1.0f;
    lastLPF_ = -1.0f;
    metalFilterL_.reset();
    metalFilterR_.reset();
    metalHPF_L_.reset();
    metalHPF_R_.reset();
    noiseHPF_L_.reset();
    noiseHPF_R_.reset();
    noiseLPF_L_.reset();
    noiseLPF_R_.reset();
    chickBPF_.reset();
    reverb_.reset();
    pinkB0_ = pinkB1_ = pinkB2_ = 0.0f;
    lofiHeld_ = 0.0f;
    lofiCounter_ = 0;
    std::fill(widthDelayBuffer_.begin(), widthDelayBuffer_.end(), 0.0f);
    widthDelayWritePos_ = 0;
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
        const float jitterAmp = 1.0f + nextRandomFloat(prngState_) * 0.18f * settings.preChickJitter;
        burstAmplitudes_[i] = std::clamp(settings.preChickLevel * jitterAmp, 0.1f, 1.0f);

        const float dir = (i % 2 == 0) ? -1.0f : 1.0f;
        const float spreadFrac = (preChickCount_ > 1) ? static_cast<float>(i + 1) / static_cast<float>(preChickCount_) : 0.35f;
        burstPans_[i] = std::clamp(dir * settings.preChickFlamSpread * spreadFrac, -1.0f, 1.0f);
    }

    mainHitSample_ = (preChickCount_ > 1)
        ? static_cast<int>(static_cast<float>(preChickCount_ - 1) * spreadSamples)
        : 0;

    const float totalTimeMs = settings.preChickSpreadMs * static_cast<float>(preChickCount_)
        + std::max({ settings.noiseDecayMs, settings.toneDecayMs, settings.reverbDecayMs, settings.reverbGateMs });
    totalDurationSamples_ = static_cast<int>((totalTimeMs * 0.001f + 0.1f) * static_cast<float>(sampleRate_));
}

float HatSynth::generateNoiseSample(NoiseType type) noexcept {
    const float white = nextRandomFloat(prngState_);

    if (type == NoiseType::PinkSilk) {
        // Silky high-biased trance air noise
        pinkB0_ = 0.99765f * pinkB0_ + white * 0.0990460f;
        pinkB1_ = 0.96300f * pinkB1_ + white * 0.2965164f;
        pinkB2_ = 0.57000f * pinkB2_ + white * 1.0526913f;
        const float pink = (pinkB0_ + pinkB1_ + pinkB2_ + white * 0.1848f) * 0.35f;
        return 0.55f * white + 0.45f * pink;
    }

    if (type == NoiseType::VelvetShimmer) {
        // High-density shimmering velvet air pulses
        const float r = nextRandomFloat(prngState_);
        if (std::abs(r) > 0.45f) {
            return (r > 0.0f ? 1.0f : -1.0f) * (0.75f + 0.25f * std::abs(white));
        }
        return white * 0.25f;
    }

    if (type == NoiseType::LoFiCrunch) {
        // Classic 909 6-bit/24kHz-style sample-and-hold sizzle
        if (++lofiCounter_ >= 2) {
            lofiCounter_ = 0;
            lofiHeld_ = std::round(white * 31.0f) / 31.0f;
        }
        return lofiHeld_;
    }

    return white;
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
    const float effectiveBurstDecayMs = std::max(2.0f, std::min(settings.preChickDecayMs, settings.preChickSpreadMs * 0.75f));
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
            noiseEnv = t / attackSamples;
        } else if (t < (attackSamples + holdSamples)) {
            noiseEnv = 1.0f;
        } else {
            const float decayT = t - (attackSamples + holdSamples);
            const float linearFraction = std::clamp(decayT / std::max(1.0f, decaySamples), 0.0f, 1.0f);
            const float expDecay = std::exp(-decayT / std::max(1.0f, decaySamples * 0.38f));
            if (settings.noiseCurve < 0.0f) {
                const float factor = -settings.noiseCurve;
                noiseEnv = (1.0f - factor) * (1.0f - linearFraction) + factor * expDecay;
            } else {
                const float factor = settings.noiseCurve;
                noiseEnv = (1.0f - factor) * expDecay + factor * (1.0f - linearFraction);
            }
        }

        // Metallic cymbal body envelope
        metalEnv = std::exp(-t / std::max(1.0f, metalDecaySamples));

        // Stick Chick transient
        if (mainAge < static_cast<int>(sampleRate_ * 0.006)) {
            const float chickT = static_cast<float>(mainAge) / static_cast<float>(sampleRate_);
            const float envFast = std::exp(-static_cast<float>(mainAge) / (0.0009f * static_cast<float>(sampleRate_)));
            switch (settings.chickType) {
                case ChickType::CrispStick: {
                    const float p1 = 2.0f * pi() * settings.chickFreqHz * chickT;
                    const float p2 = 2.0f * pi() * (settings.chickFreqHz * 1.47f) * chickT;
                    chickSample = (std::sin(p1) * 0.65f + std::sin(p2) * 0.35f) * envFast * settings.chickLevel;
                    break;
                }
                case ChickType::PedalFoot: {
                    const float p1 = 2.0f * pi() * (settings.chickFreqHz * 0.55f) * chickT;
                    const float p2 = 2.0f * pi() * (settings.chickFreqHz * 1.18f) * chickT;
                    const float envPedal = std::exp(-static_cast<float>(mainAge) / (0.0022f * static_cast<float>(sampleRate_)));
                    chickSample = (std::sin(p1) * 0.6f + std::sin(p2) * 0.4f) * envPedal * settings.chickLevel * 1.15f;
                    break;
                }
                case ChickType::LaserTick: {
                    const float sweepHz = settings.chickFreqHz * (1.0f + 1.8f * envFast);
                    const float p = 2.0f * pi() * sweepHz * chickT;
                    chickSample = std::sin(p) * envFast * settings.chickLevel * 1.2f;
                    break;
                }
            }
        }
    }

    // 3. Tuned 6-Oscillator Metallic Cymbal Matrix
    float metalL = 0.0f;
    float metalR = 0.0f;
    const float rawNoiseL = generateNoiseSample(settings.noiseType);
    const float rawNoiseR = generateNoiseSample(settings.noiseType);

    if (mainAge >= 0 && settings.toneLevel > 0.001f) {
        const float pitchDecaySamples = (settings.tonePitchDecayMs * 0.001f) * static_cast<float>(sampleRate_);
        const float pitchEnvFrac = std::exp(-static_cast<float>(mainAge) / std::max(1.0f, pitchDecaySamples));
        const float pitchSemitones = static_cast<float>(semitoneOffset_) + settings.tonePitchEnv * pitchEnvFrac;

        // Compute base tuned frequency from Note, Octave, and Fine cents
        const float baseHz = noteToHz(settings.note, settings.octave, settings.fineCents);
        const float fundamentalHz = std::clamp(baseHz * std::pow(2.0f, pitchSemitones / 12.0f), 80.0f, static_cast<float>(sampleRate_) * 0.42f);

        // Advance 6 metallic cymbal oscillators
        for (size_t i = 0; i < 6; ++i) {
            const float oscHz = std::min(fundamentalHz * kMetallicRatios[i], static_cast<float>(sampleRate_) * 0.46f);
            metalPhases_[i] += 2.0f * pi() * oscHz / static_cast<float>(sampleRate_);
            if (metalPhases_[i] >= 2.0f * pi()) {
                metalPhases_[i] -= 2.0f * pi();
            }
        }

        float metalSumL = 0.0f;
        float metalSumR = 0.0f;

        switch (settings.metalMode) {
            case MetalMode::Classic909: {
                // 6-oscillator non-harmonic square matrix mixed with crisp sizzle
                for (size_t i = 0; i < 6; ++i) {
                    const float sq = (std::sin(metalPhases_[i]) >= 0.0f) ? 1.0f : -1.0f;
                    metalSumL += sq * ((i % 2 == 0) ? 0.22f : 0.14f);
                    metalSumR += sq * ((i % 2 == 1) ? 0.22f : 0.14f);
                }
                metalSumL = metalSumL * 0.65f + rawNoiseL * 0.35f;
                metalSumR = metalSumR * 0.65f + rawNoiseR * 0.35f;
                break;
            }
            case MetalMode::AnjunaSilk: {
                // Phase-dispersed harmonic shimmer matrix for silky progressive trance off-beats
                for (size_t i = 0; i < 6; ++i) {
                    const float s1 = std::sin(metalPhases_[i]);
                    const float s2 = std::sin(metalPhases_[i] * 2.0f + 0.5f);
                    const float shimmer = std::tanh((s1 + 0.5f * s2) * 1.6f);
                    metalSumL += shimmer * (0.20f - 0.015f * static_cast<float>(i));
                    metalSumR += std::sin(metalPhases_[i] + 0.35f) * (0.20f - 0.015f * static_cast<float>(i));
                }
                break;
            }
            case MetalMode::UpliftingFMBell: {
                // 3-pair cross-modulated FM cymbal partials for 138 BPM uplifting shimmer
                const float fm1 = std::sin(metalPhases_[0] + 1.8f * std::sin(metalPhases_[3]));
                const float fm2 = std::sin(metalPhases_[1] + 2.1f * std::sin(metalPhases_[4]));
                const float fm3 = std::sin(metalPhases_[2] + 1.5f * std::sin(metalPhases_[5]));
                metalSumL = (fm1 * 0.40f + fm2 * 0.35f + fm3 * 0.25f);
                metalSumR = (fm1 * 0.35f + fm2 * 0.40f - fm3 * 0.25f);
                break;
            }
            case MetalMode::Analog808Crisp: {
                // Classic 808 asymmetric pulse matrix
                for (size_t i = 0; i < 6; ++i) {
                    const float pulse = (std::sin(metalPhases_[i]) > 0.25f) ? 1.0f : -0.6f;
                    metalSumL += pulse * 0.18f;
                    metalSumR += pulse * ((i < 3) ? 0.20f : -0.16f);
                }
                break;
            }
        }

        // Resonant cymbal body bandpass filter centered around the tuned frequency / toneFreqHz
        const float bodyCutoff = std::clamp(
            fundamentalHz * (settings.toneFreqHz / 1100.0f),
            250.0f,
            static_cast<float>(sampleRate_) * 0.45f);
        float b0_t, b1_t, b2_t, a1_t, a2_t;
        computeBandpassCoeffs(bodyCutoff, settings.toneQ, sampleRate_, b0_t, b1_t, b2_t, a1_t, a2_t);

        const float bpL = processBiquad(metalSumL * metalEnv, metalFilterL_, b0_t, b1_t, b2_t, a1_t, a2_t);
        const float bpR = processBiquad(metalSumR * metalEnv, metalFilterR_, b0_t, b1_t, b2_t, a1_t, a2_t);

        // High-pass metallic overtones so cymbal shimmer stays crisp
        float b0_mh, b1_mh, b2_mh, a1_mh, a2_mh;
        computeHighpassCoeffs(std::max(1800.0f, fundamentalHz), sampleRate_, b0_mh, b1_mh, b2_mh, a1_mh, a2_mh);
        const float hpL = processBiquad(metalSumL * metalEnv, metalHPF_L_, b0_mh, b1_mh, b2_mh, a1_mh, a2_mh);
        const float hpR = processBiquad(metalSumR * metalEnv, metalHPF_R_, b0_mh, b1_mh, b2_mh, a1_mh, a2_mh);

        metalL = (bpL * 1.35f + hpL * 0.55f) * settings.toneLevel * velocity_;
        metalR = (bpR * 1.35f + hpR * 0.55f) * settings.toneLevel * velocity_;
    }

    // 4. Air Sizzle & High-Sheen Noise Stage
    const float totalNoiseL = (rawNoiseL * preChickGainL * 0.5f + rawNoiseL * noiseEnv * settings.noiseLevel) * velocity_;
    const float totalNoiseR = (rawNoiseR * preChickGainR * 0.5f + rawNoiseR * noiseEnv * settings.noiseLevel) * velocity_;

    if (std::abs(settings.noiseHPF - lastHPF_) > 0.1f) {
        lastHPF_ = settings.noiseHPF;
        computeHighpassCoeffs(lastHPF_, sampleRate_, b0_h_, b1_h_, b2_h_, a1_h_, a2_h_);
    }
    const float filteredHP_L = processBiquad(totalNoiseL, noiseHPF_L_, b0_h_, b1_h_, b2_h_, a1_h_, a2_h_);
    const float filteredHP_R = processBiquad(totalNoiseR, noiseHPF_R_, b0_h_, b1_h_, b2_h_, a1_h_, a2_h_);

    if (std::abs(settings.noiseLPF - lastLPF_) > 0.1f) {
        lastLPF_ = settings.noiseLPF;
        computeLowpassCoeffs(lastLPF_, sampleRate_, b0_l_, b1_l_, b2_l_, a1_l_, a2_l_);
    }
    float filteredNoiseL = processBiquad(filteredHP_L, noiseLPF_L_, b0_l_, b1_l_, b2_l_, a1_l_, a2_l_);
    float filteredNoiseR = processBiquad(filteredHP_R, noiseLPF_R_, b0_l_, b1_l_, b2_l_, a1_l_, a2_l_);

    // 5. Stereo Width (Mid/Side processing)
    if (settings.noiseStereoWidth != 1.0f) {
        const float mid = (filteredNoiseL + filteredNoiseR) * 0.5f;
        const float side = (filteredNoiseL - filteredNoiseR) * 0.5f * settings.noiseStereoWidth;
        filteredNoiseL = mid + side;
        filteredNoiseR = mid - side;
    }

    const float dryL = filteredNoiseL + metalL + chickSample * velocity_;
    const float dryR = filteredNoiseR + metalR + chickSample * velocity_;

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

    const float outL = dryL * (1.0f - settings.reverbMix * 0.45f) + reverbOut.left * settings.reverbMix;
    const float outR = dryR * (1.0f - settings.reverbMix * 0.45f) + reverbOut.right * settings.reverbMix;

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
