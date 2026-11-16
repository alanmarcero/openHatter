#pragma once

#include "HatSynth.h"
#include "Effects.h"
#include <atomic>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace openhatter::core {

struct Transport {
    double bpm = 138.0;
    std::optional<double> ppqPosition;
    double beatsPerBar = 4.0;
    bool isPlaying = true;
};

struct MidiTrigger {
    int sampleOffset = 0;
    int noteNumber = 66; // F#4
    float velocity = 1.0f;
    bool audition = false;
    bool choke = false;
};

struct MidiCcEvent {
    int sampleOffset = 0;
    int controller = 0;
    int value = 0;
};

struct HatEffectSettings {
    DistortionType distType = DistortionType::Tape;
    float distDrive = 0.08f;       // 0..1
    float distTone = 0.15f;        // -1..+1
    float distMix = 0.15f;         // 0..1

    CompressorSettings compressor{};
    LimiterSettings limiter{};

    ReverseSyncMode reverseMode = ReverseSyncMode::Off;
    float reverseMix = 0.5f;       // 0..1

    float stereoWidth = 1.15f;     // 0..2
    float outputGainDb = 0.0f;     // -36..+12 dB
};

struct EngineSettings {
    HatSynthSettings synth{};
    HatEffectSettings effects{};
    int midiChokeNote = 42;
    int midiChokeCc = 4;
    bool rollEnabled = false;
    int rollDivision = 1; // 0 = 1/8, 1 = 1/16, 2 = 1/32
    float rollSwing = 0.0f;
    float rollAccent = 0.25f;
    bool bypass = false;
};

struct LayerOutputBuffers {
    float* metalLeft = nullptr;
    float* metalRight = nullptr;
    float* airLeft = nullptr;
    float* airRight = nullptr;
    float* chickLeft = nullptr;
    float* chickRight = nullptr;
    float* reverbLeft = nullptr;
    float* reverbRight = nullptr;
};

// Internal 32-bit float stereo WAV cache for deterministic Lock-In mode
class HatWavCache {
public:
    void clear() noexcept;
    void capture(const std::vector<float>& left, const std::vector<float>& right, double sampleRate);
    [[nodiscard]] bool isLocked() const noexcept { return isLocked_; }
    [[nodiscard]] double sampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] const std::vector<float>& left() const noexcept { return left_; }
    [[nodiscard]] const std::vector<float>& right() const noexcept { return right_; }

    [[nodiscard]] std::vector<uint8_t> toWavBytes() const;
    bool fromWavBytes(const std::vector<uint8_t>& wavBytes);

private:
    bool isLocked_ = false;
    double sampleRate_ = 48000.0;
    std::vector<float> left_;
    std::vector<float> right_;
};

class HatEngine {
public:
    static constexpr int kLatencySamples = 0;

    void prepare(double sampleRate);
    void reset() noexcept;

    void process(
        const EngineSettings& settings,
        const Transport& transport,
        std::span<const MidiTrigger> midiTriggers,
        float* const* channels,
        int numChannels,
        int numSamples,
        std::span<const MidiCcEvent> controlChanges = {},
        const LayerOutputBuffers* layerOutputs = nullptr) noexcept;

    void lockCurrentHat(const EngineSettings& settings);
    void unlockHat() noexcept;
    bool restoreLockedWav(const std::vector<uint8_t>& wavBytes);

    [[nodiscard]] bool isHatLocked() const noexcept { return wavCache_.isLocked(); }
    [[nodiscard]] const HatWavCache& wavCache() const noexcept { return wavCache_; }

    [[nodiscard]] float currentPeak() const noexcept { return currentPeak_; }
    [[nodiscard]] float currentBeatPhase() const noexcept { return currentBeatPhase_; }
    [[nodiscard]] float currentHatProgress() const noexcept { return currentHatProgress_; }
    [[nodiscard]] uint32_t triggerSerial() const noexcept { return triggerSerial_; }
    [[nodiscard]] bool isReversing() const noexcept { return isReversing_; }
    [[nodiscard]] uint64_t reverseCallbackWorkUnits() const noexcept { return reverseCallbackWorkUnits_; }

private:
    void triggerHat(
        const EngineSettings& settings,
        double samplePpq,
        double bpm,
        double beatsPerBar,
        float velocity,
        int midiNoteNumber,
        bool reverseCue) noexcept;

    double sampleRate_ = 48000.0;
    double fallbackPpq_ = 0.0;

    HatSynth synth_;
    DistortionShaper distL_;
    DistortionShaper distR_;
    StereoCompressor compressor_;
    BrickwallLimiter limiter_;

    HatWavCache wavCache_;

    // Reversed audio is read from preallocated dry-synth history. The ring is
    // twice the maximum reverse duration so its reader cannot collide with the
    // writer during a reverse pass.
    std::vector<float> reverseHistoryL_;
    std::vector<float> reverseHistoryR_;
    int reverseHistoryWriteIndex_ = 0;
    int reverseHistorySamplesAvailable_ = 0;
    int reverseSourceStart_ = 0;
    int reverseSourceSamplesAvailable_ = 0;
    int reversePlayHead_ = -1;
    int reverseDuration_ = 0;
    float reverseMix_ = 0.5f;
    bool isReversing_ = false;
    uint64_t reverseCallbackWorkUnits_ = 0;

    bool rollActive_ = false;
    int rollNoteNumber_ = 66;
    float rollVelocity_ = 1.0f;
    int rollStep_ = 0;
    double nextRollPpq_ = 0.0;
    bool rollTransportWasPlaying_ = false;

    // Performance CCs
    float ccVolume_ = 1.0f;
    float ccBrightness_ = 1.0f;

    // Metrics for GUI animation
    float currentPeak_ = 0.0f;
    float currentBeatPhase_ = 0.0f;
    float currentHatProgress_ = 1.0f;
    uint32_t triggerSerial_ = 0;
};

} // namespace openhatter::core
