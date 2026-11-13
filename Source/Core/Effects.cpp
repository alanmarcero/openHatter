#include "Effects.h"
#include <algorithm>
#include <cmath>
#include <numbers>

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
