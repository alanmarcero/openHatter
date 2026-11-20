#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace openhatter::core {

enum class DistortionType {
    Tape = 0,
    Tube = 1,
    HardClip = 2,
    Wavefold = 3
};

enum class ReverseSyncMode {
    Off = 0,
    BeatEnd = 1,
    Beat4Auto = 2,
    HalfBeat = 3,
    QuarterBeat = 4
};

// ============================================================================
// Anti-Aliasing Primitives (Polyphase 4x Halfband Oversampler & ADAA-1 Tanh)
// ============================================================================

struct AdaaTanh {
    float prevInput = 0.0f;
    float prevAntideriv = 0.0f;
    bool primed = false;

    void reset() noexcept {
        prevInput = 0.0f;
        prevAntideriv = 0.0f;
        primed = false;
    }

    [[nodiscard]] static float antiderivative(float input) noexcept {
        const float magnitude = std::abs(input);
        return magnitude + std::log1pf(std::exp(-2.0f * magnitude)) - 0.69314718f;
    }

    [[nodiscard]] float process(float input) noexcept {
        const float currentAntideriv = antiderivative(input);
        if (!primed) {
            primed = true;
            prevInput = input;
            prevAntideriv = currentAntideriv;
            return std::tanh(input);
        }
        const float delta = input - prevInput;
        const float midpoint = 0.5f * (input + prevInput);
        const float antiderivDiff = currentAntideriv - prevAntideriv;
        prevInput = input;
        prevAntideriv = currentAntideriv;
        if (std::abs(delta) < 1.0e-4f)
            return std::tanh(midpoint);
        return std::clamp(antiderivDiff / delta, -1.0f, 1.0f);
    }
};

struct Oversampler4x {
    static constexpr std::array<float, 3> kStage1Even{{ 0.06396838f, 0.42965512f, 0.78589107f }};
    static constexpr std::array<float, 3> kStage1Odd{{ 0.22734146f, 0.62193724f, 0.92904445f }};
    static constexpr std::array<float, 2> kStage2Even{{ 0.04634830f, 0.41198652f }};
    static constexpr std::array<float, 2> kStage2Odd{{ 0.18335817f, 0.75779003f }};

    std::array<float, 3> up1Even{};
    std::array<float, 3> up1Odd{};
    std::array<float, 2> up2Even{};
    std::array<float, 2> up2Odd{};
    std::array<float, 2> down2Even{};
    std::array<float, 2> down2Odd{};
    std::array<float, 3> down1Even{};
    std::array<float, 3> down1Odd{};

    void reset() noexcept {
        up1Even.fill(0.0f);
        up1Odd.fill(0.0f);
        up2Even.fill(0.0f);
        up2Odd.fill(0.0f);
        down2Even.fill(0.0f);
        down2Odd.fill(0.0f);
        down1Even.fill(0.0f);
        down1Odd.fill(0.0f);
    }

    template <size_t N>
    [[nodiscard]] static float stepAllpassChain(float input, std::array<float, N>& states,
                                                const std::array<float, N>& coeffs) noexcept {
        float sig = input;
        const float out0 = coeffs[0] * sig + states[0];
        states[0] = sig - coeffs[0] * out0;
        sig = out0;
        const float out1 = coeffs[1] * sig + states[1];
        states[1] = sig - coeffs[1] * out1;
        sig = out1;
        if constexpr (N == 3) {
            const float out2 = coeffs[2] * sig + states[2];
            states[2] = sig - coeffs[2] * out2;
            sig = out2;
        }
        return sig;
    }

    [[nodiscard]] std::array<float, 4> upsample(float input) noexcept {
        const float mid0 = stepAllpassChain(input, up1Even, kStage1Even);
        const float mid1 = stepAllpassChain(input, up1Odd, kStage1Odd);
        return {
            stepAllpassChain(mid0, up2Even, kStage2Even),
            stepAllpassChain(mid0, up2Odd, kStage2Odd),
            stepAllpassChain(mid1, up2Even, kStage2Even),
            stepAllpassChain(mid1, up2Odd, kStage2Odd)
        };
    }

    [[nodiscard]] float downsample(const std::array<float, 4>& highRate) noexcept {
        const float mid0 = 0.5f * (stepAllpassChain(highRate[1], down2Even, kStage2Even)
                                 + stepAllpassChain(highRate[0], down2Odd, kStage2Odd));
        const float mid1 = 0.5f * (stepAllpassChain(highRate[3], down2Even, kStage2Even)
                                 + stepAllpassChain(highRate[2], down2Odd, kStage2Odd));
        return 0.5f * (stepAllpassChain(mid1, down1Even, kStage1Even)
                     + stepAllpassChain(mid0, down1Odd, kStage1Odd));
    }
};

// ============================================================================
// Distortion / Shimmer Exciter
// ============================================================================

class DistortionShaper {
public:
    void reset() noexcept;
    [[nodiscard]] float processSample(float input, DistortionType type, float drive, float tone, double sampleRate) noexcept;

private:
    Oversampler4x oversampler_{};
    float preEmphasisState_ = 0.0f;
    float deEmphasisState_ = 0.0f;
    float toneFilterState_ = 0.0f;
    float exciterHpState_ = 0.0f;
};

// ============================================================================
// Stereo Compressor
// ============================================================================

struct CompressorSettings {
    float thresholdDb = -14.0f; // -40..0 dB
    float ratio = 3.5f;         // 1..20
    float attackMs = 4.0f;      // 0.1..50 ms
    float releaseMs = 65.0f;    // 10..500 ms
    float makeupDb = 2.0f;      // 0..24 dB
    float mix = 1.0f;           // 0..1

    bool operator==(const CompressorSettings&) const noexcept = default;
};

class StereoCompressor {
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    void process(float& left, float& right, const CompressorSettings& settings) noexcept;

private:
    double sampleRate_ = 48000.0;
    float envelopeDb_ = -100.0f;
};

// ============================================================================
// Stereo 3-Band EQ (Low Shelf, Mid Peak, High Shelf)
// ============================================================================

struct ThreeBandEqSettings {
    float lowFreqHz = 80.0f;
    float lowGainDb = 0.0f;
    float midFreqHz = 1000.0f;
    float midGainDb = 0.0f;
    float highFreqHz = 7500.0f;
    float highGainDb = 0.0f;

    bool operator==(const ThreeBandEqSettings&) const noexcept = default;
};

// Stereo 3-band EQ with smooth log-frequency glide and +/-12 dB cut/boost per band.
class ThreeBandEq {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void process(float& left, float& right, const ThreeBandEqSettings& settings) noexcept;

private:
    enum class BandType { lowShelf, peak, highShelf };
    struct Band {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1L = 0.0, z2L = 0.0, z1R = 0.0, z2R = 0.0;
        float smoothedLogFreq = 0.0f;
        float smoothedDb = 0.0f;
        bool initialized = false;
    };

    void updateBand(Band& band, BandType type, float minHz, float maxHz, float targetHz, float targetDb) noexcept;
    static void filterBand(Band& band, float& left, float& right) noexcept;

    double sampleRate_ = 48000.0;
    float glideCoeff_ = 0.001f;
    std::array<Band, 3> bands_{};
};

// ============================================================================
// Brickwall Limiter
// ============================================================================

struct LimiterSettings {
    float ceilingDb = -0.3f;    // -12..0 dB
    float releaseMs = 20.0f;    // 5..100 ms
    bool enabled = true;

    bool operator==(const LimiterSettings&) const noexcept = default;
};

class BrickwallLimiter {
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    void process(float& left, float& right, const LimiterSettings& settings) noexcept;

private:
    double sampleRate_ = 48000.0;
    float gain_ = 1.0f;
};

// ============================================================================
// Host-Synced Reverse Cymbal Swell Helper
// ============================================================================

[[nodiscard]] bool shouldTriggerReverse(
    ReverseSyncMode mode,
    bool manualCue,
    double ppqPosition,
    double beatsPerBar) noexcept;

[[nodiscard]] double reverseTargetPpq(
    ReverseSyncMode mode,
    double ppqPosition,
    double beatsPerBar) noexcept;

[[nodiscard]] int computeSyncedReverseSamples(
    ReverseSyncMode mode,
    double ppqPosition,
    double bpm,
    double beatsPerBar,
    double sampleRate) noexcept;

} // namespace openhatter::core
