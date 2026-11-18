#include "Effects.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>

namespace openhatter::core {

namespace {

[[nodiscard]] constexpr float pi() noexcept { return std::numbers::pi_v<float>; }
[[nodiscard]] inline float dbToLin(float db) noexcept { return std::pow(10.0f, db / 20.0f); }
[[nodiscard]] inline float linToDb(float lin) noexcept { return 20.0f * std::log10(std::max(lin, 1.0e-6f)); }
[[nodiscard]] inline float finiteOrZero(float value) noexcept { return std::isfinite(value) ? value : 0.0f; }

} // namespace

void DistortionShaper::reset() noexcept {
    toneFilterState_ = 0.0f;
    exciterHpState_ = 0.0f;
}

float DistortionShaper::processSample(float input, DistortionType type, float drive,
                                      float tone, double sampleRate) noexcept {
    input = finiteOrZero(input);
    drive = std::clamp(finiteOrZero(drive), 0.0f, 1.0f);
    tone = std::clamp(finiteOrZero(tone), -1.0f, 1.0f);
    if (drive < 0.001f && std::abs(tone) < 0.001f)
        return input;

    const float preGain = std::exp2(drive * 4.0f);
    const float x = std::clamp(input * preGain, -12.0f, 12.0f);
    float saturated = x;
    switch (type) {
        case DistortionType::Tape: {
            const float rounded = std::tanh(1.35f * x);
            const float bias = 0.035f * (1.0f - std::exp(-x * x));
            saturated = rounded + bias;
            break;
        }
        case DistortionType::Tube: {
            const float evenHarmonics = 0.12f * x * x / (1.0f + x * x);
            saturated = std::tanh(1.1f * x) + evenHarmonics;
            break;
        }
        case DistortionType::HardClip:
            saturated = std::clamp(x, -0.92f, 0.92f);
            break;
        case DistortionType::Wavefold: {
            const double sr = std::max(1000.0, sampleRate);
            const float hpAlpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * 3200.0 / sr));
            exciterHpState_ += hpAlpha * (x - exciterHpState_);
            const float highBand = x - exciterHpState_;
            const float folded = std::sin(std::clamp(x * (1.0f + 2.3f * drive), -pi(), pi()));
            saturated = 0.68f * folded + 0.32f * std::tanh(highBand * (1.0f + 3.0f * drive));
            break;
        }
    }

    const float driveExcess = preGain - 1.0f;
    const float trim = 1.0f / std::sqrt(1.0f + 0.08f * driveExcess * driveExcess);
    float output = saturated * trim;
    if (std::abs(tone) > 0.001f) {
        const double sr = std::max(1000.0, sampleRate);
        const float cutoff = 1450.0f * std::exp2(tone * 2.8f);
        const float alpha = static_cast<float>(1.0 - std::exp(-2.0 * pi() * cutoff / sr));
        toneFilterState_ += alpha * (output - toneFilterState_);
        const float highBand = output - toneFilterState_;
        output = tone < 0.0f ? toneFilterState_ + highBand * (1.0f + tone)
                             : toneFilterState_ + highBand * (1.0f + 2.2f * tone);
    }
    return finiteOrZero(output);
}

void StereoCompressor::prepare(double sampleRate) {
    sampleRate_ = std::max(1000.0, sampleRate);
    reset();
}

void StereoCompressor::reset() noexcept { envelopeDb_ = -100.0f; }

void StereoCompressor::process(float& left, float& right, const CompressorSettings& settings) noexcept {
    left = finiteOrZero(left);
    right = finiteOrZero(right);
    const float dryL = left;
    const float dryR = right;
    const float peak = std::max(std::abs(left), std::abs(right));
    const float detectorDb = linToDb(peak);
    const float attack = std::max(0.1f, settings.attackMs) * 0.001f;
    const float release = std::max(1.0f, settings.releaseMs) * 0.001f;
    const float tau = detectorDb > envelopeDb_ ? attack : release;
    const float coefficient = std::exp(-1.0f / (tau * static_cast<float>(sampleRate_)));
    envelopeDb_ = detectorDb + coefficient * (envelopeDb_ - detectorDb);

    const float threshold = std::clamp(settings.thresholdDb, -60.0f, 0.0f);
    const float ratio = std::clamp(settings.ratio, 1.0f, 20.0f);
    constexpr float kneeDb = 6.0f;
    const float over = envelopeDb_ - threshold;
    float reductionDb = 0.0f;
    if (over > -kneeDb * 0.5f && ratio > 1.0f) {
        const float slope = 1.0f - 1.0f / ratio;
        if (over >= kneeDb * 0.5f)
            reductionDb = -slope * over;
        else {
            const float knee = over + kneeDb * 0.5f;
            reductionDb = -slope * knee * knee / (2.0f * kneeDb);
        }
    }
    const float makeup = std::clamp(settings.makeupDb, 0.0f, 24.0f);
    const float mix = std::clamp(settings.mix, 0.0f, 1.0f);
    const float gain = dbToLin(reductionDb + makeup);
    left = dryL + (dryL * gain - dryL) * mix;
    right = dryR + (dryR * gain - dryR) * mix;
}

void ThreeBandEq::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(8000.0, sampleRate);
    glideCoeff_ = 1.0f - std::exp(-1.0f / (0.020f * static_cast<float>(sampleRate_)));
    reset();
}

void ThreeBandEq::reset() noexcept {
    std::ranges::for_each(bands_, [](Band& band) {
        band.z1L = band.z2L = band.z1R = band.z2R = 0.0;
        band.b0 = 1.0;
        band.b1 = band.b2 = band.a1 = band.a2 = 0.0;
        band.initialized = false;
    });
}

void ThreeBandEq::updateBand(Band& band, BandType type, float minHz, float maxHz,
                             float targetHz, float targetDb) noexcept {
    const float clampedHz = std::clamp(std::isfinite(targetHz) ? targetHz : minHz,
                                       minHz, std::min(maxHz, static_cast<float>(sampleRate_ * 0.45)));
    const float targetLogFreq = std::log(clampedHz);
    const float clampedDb = std::clamp(std::isfinite(targetDb) ? targetDb : 0.0f, -12.0f, 12.0f);
    if (!band.initialized) {
        band.smoothedLogFreq = targetLogFreq;
        band.smoothedDb = clampedDb;
        band.initialized = true;
    }
    band.smoothedLogFreq += glideCoeff_ * (targetLogFreq - band.smoothedLogFreq);
    band.smoothedDb += glideCoeff_ * (clampedDb - band.smoothedDb);

    if (std::abs(band.smoothedDb) < 0.005f) {
        band.b0 = 1.0;
        band.b1 = band.b2 = band.a1 = band.a2 = 0.0;
        return;
    }

    const double freq = static_cast<double>(std::exp(band.smoothedLogFreq));
    const double omega = 2.0 * std::numbers::pi * std::clamp(freq / sampleRate_, 1.0e-5, 0.45);
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double amp = std::pow(10.0, static_cast<double>(band.smoothedDb) / 40.0);

    if (type == BandType::peak) {
        const double alpha = sn / (2.0 * 0.9);
        const double a0 = 1.0 + alpha / amp;
        band.b0 = (1.0 + alpha * amp) / a0;
        band.b1 = (-2.0 * cs) / a0;
        band.b2 = (1.0 - alpha * amp) / a0;
        band.a1 = (-2.0 * cs) / a0;
        band.a2 = (1.0 - alpha / amp) / a0;
        return;
    }

    const double alpha = sn * 0.5 * std::sqrt(2.0);
    const double twoRootAmpAlpha = 2.0 * std::sqrt(amp) * alpha;
    if (type == BandType::lowShelf) {
        const double a0 = (amp + 1.0) + (amp - 1.0) * cs + twoRootAmpAlpha;
        band.b0 = (amp * ((amp + 1.0) - (amp - 1.0) * cs + twoRootAmpAlpha)) / a0;
        band.b1 = (2.0 * amp * ((amp - 1.0) - (amp + 1.0) * cs)) / a0;
        band.b2 = (amp * ((amp + 1.0) - (amp - 1.0) * cs - twoRootAmpAlpha)) / a0;
        band.a1 = (-2.0 * ((amp - 1.0) + (amp + 1.0) * cs)) / a0;
        band.a2 = ((amp + 1.0) + (amp - 1.0) * cs - twoRootAmpAlpha) / a0;
        return;
    }

    const double a0 = (amp + 1.0) - (amp - 1.0) * cs + twoRootAmpAlpha;
    band.b0 = (amp * ((amp + 1.0) + (amp - 1.0) * cs + twoRootAmpAlpha)) / a0;
    band.b1 = (-2.0 * amp * ((amp - 1.0) + (amp + 1.0) * cs)) / a0;
    band.b2 = (amp * ((amp + 1.0) + (amp - 1.0) * cs - twoRootAmpAlpha)) / a0;
    band.a1 = (2.0 * ((amp - 1.0) - (amp + 1.0) * cs)) / a0;
    band.a2 = ((amp + 1.0) - (amp - 1.0) * cs - twoRootAmpAlpha) / a0;
}

void ThreeBandEq::filterBand(Band& band, float& left, float& right) noexcept {
    const double xL = static_cast<double>(left);
    const double yL = band.b0 * xL + band.z1L;
    band.z1L = band.b1 * xL - band.a1 * yL + band.z2L;
    band.z2L = band.b2 * xL - band.a2 * yL;
    left = std::isfinite(yL) ? static_cast<float>(yL) : 0.0f;

    const double xR = static_cast<double>(right);
    const double yR = band.b0 * xR + band.z1R;
    band.z1R = band.b1 * xR - band.a1 * yR + band.z2R;
    band.z2R = band.b2 * xR - band.a2 * yR;
    right = std::isfinite(yR) ? static_cast<float>(yR) : 0.0f;
}

void ThreeBandEq::process(float& left, float& right, const ThreeBandEqSettings& settings) noexcept {
    updateBand(bands_[0], BandType::lowShelf, 25.0f, 400.0f, settings.lowFreqHz, settings.lowGainDb);
    updateBand(bands_[1], BandType::peak, 200.0f, 6000.0f, settings.midFreqHz, settings.midGainDb);
    updateBand(bands_[2], BandType::highShelf, 2000.0f, 18000.0f, settings.highFreqHz, settings.highGainDb);
    filterBand(bands_[0], left, right);
    filterBand(bands_[1], left, right);
    filterBand(bands_[2], left, right);
}

void BrickwallLimiter::prepare(double sampleRate) {
    sampleRate_ = std::max(1000.0, sampleRate);
    reset();
}

void BrickwallLimiter::reset() noexcept { gain_ = 1.0f; }

void BrickwallLimiter::process(float& left, float& right, const LimiterSettings& settings) noexcept {
    left = finiteOrZero(left);
    right = finiteOrZero(right);
    if (!settings.enabled)
        return;

    const float ceiling = dbToLin(std::clamp(settings.ceilingDb, -24.0f, 0.0f));
    const float peak = std::max(std::abs(left), std::abs(right));
    const float requestedGain = peak > ceiling && peak > 1.0e-8f ? ceiling / peak : 1.0f;
    if (requestedGain < gain_) {
        // Instant attack is the safety stage; release is smoothed to avoid
        // pumping between the closely spaced transients of a bounce pattern.
        gain_ = requestedGain;
    } else {
        const float release = std::max(1.0f, settings.releaseMs) * 0.001f;
        const float coefficient = std::exp(-1.0f / (release * static_cast<float>(sampleRate_)));
        gain_ = 1.0f + coefficient * (gain_ - 1.0f);
    }
    left *= gain_;
    right *= gain_;
    const float postPeak = std::max(std::abs(left), std::abs(right));
    if (postPeak > ceiling && postPeak > 1.0e-8f) {
        const float finalGain = ceiling / postPeak;
        left *= finalGain;
        right *= finalGain;
    }
}

bool shouldTriggerReverse(ReverseSyncMode mode, bool manualCue,
                          double ppqPosition, double beatsPerBar) noexcept {
    if (mode == ReverseSyncMode::Off)
        return manualCue;
    if (mode == ReverseSyncMode::Beat4Auto) {
        const double barBeats = std::max(1.0, beatsPerBar);
        const double beatInBar = std::fmod(std::max(0.0, ppqPosition), barBeats);
        return beatInBar >= barBeats - 1.0 - 0.05;
    }
    return true;
}

double reverseTargetPpq(ReverseSyncMode mode, double ppqPosition, double beatsPerBar) noexcept {
    const double barBeats = std::max(1.0, beatsPerBar);
    switch (mode) {
        case ReverseSyncMode::Beat4Auto:
            return (std::floor(ppqPosition / barBeats) + 1.0) * barBeats;
        case ReverseSyncMode::BeatEnd:
            return std::floor(ppqPosition) + 1.0;
        case ReverseSyncMode::HalfBeat:
            return (std::floor(ppqPosition * 2.0) + 1.0) / 2.0;
        case ReverseSyncMode::QuarterBeat:
            return (std::floor(ppqPosition * 4.0) + 1.0) / 4.0;
        case ReverseSyncMode::Off:
        default:
            return ppqPosition + 1.0;
    }
}

int computeSyncedReverseSamples(ReverseSyncMode mode, double ppqPosition, double bpm,
                                double beatsPerBar, double sampleRate) noexcept {
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
        return 32;
    if (mode == ReverseSyncMode::Off || !std::isfinite(bpm) || bpm <= 0.0)
        return std::max(32, static_cast<int>(sampleRate * 0.45));
    const double targetPpq = reverseTargetPpq(mode, ppqPosition, beatsPerBar);
    const double beatsUntilTarget = std::max(0.01, targetPpq - ppqPosition);
    const double secondsUntilTarget = beatsUntilTarget * (60.0 / bpm);
    return std::max(32, static_cast<int>(secondsUntilTarget * sampleRate));
}

} // namespace openhatter::core
