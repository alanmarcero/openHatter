#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <span>
#include <string_view>
#include <vector>

namespace openhatter::core {

enum class NoiseType {
    CrispWhite = 0,
    PinkSilk = 1,
    VelvetShimmer = 2,
    LoFiCrunch = 3
};

enum class ChickType {
    CrispStick = 0,
    PedalFoot = 1,
    LaserTick = 2
};

struct HatSynthSettings {
    // Pre-Chick / Stick Bounce Articulation
    int preChickCount = 1;        // 1..5 micro-bounces before open ring
    float preChickSpreadMs = 8.0f;// 2..45 ms spacing between bounces
    float preChickDecayMs = 6.0f; // 2..35 ms decay of each bounce
    float preChickJitter = 0.08f; // 0..1 timing humanization
    float preChickFlamSpread = 0.35f; // 0..1 stereo micro-panning spread
    float preChickLevel = 0.65f;  // 0..1 volume of stick bounces

    float decayScale = 1.0f;      // 0.25..2.0 global open/closed decay morph macro

    bool fullAdsrEnabled = false;
    float ampAttackMs = 0.35f;
    float ampDecayMs = 72.0f;
    float ampSustain = 0.70f;
    float ampReleaseMs = 180.0f;

    // Air Sizzle & Shimmer Noise
    NoiseType noiseType = NoiseType::CrispWhite;
    float noiseAttackMs = 1.5f;   // 0..40 ms (allows off-beat pocket swell)
    float noiseHoldMs = 3.0f;     // 0..30 ms
    float noiseDecayMs = 260.0f;  // 20..1200 ms open hat tail decay
    float noiseCurve = -0.35f;    // -1..+1 shape curve (-1 exp, 0 lin, +1 log)
    float noiseHPF = 1200.0f;     // 100..6000 Hz high-pass filter cutoff
    float noiseLPF = 16500.0f;    // 1500..20000 Hz low-pass filter cutoff
    float noiseStereoWidth = 1.2f;// 0 (mono) .. 2 (ultra-wide stereo shimmer)
    float noiseLevel = 0.88f;     // 0..1
    float velocityAir = 1.0f;

    // Stick Chick / Transient
    ChickType chickType = ChickType::CrispStick;
    float chickLevel = 0.45f;     // 0..1
    float chickFreqHz = 6800.0f;  // 1000..14000 Hz
    float velocityChick = 1.0f;

    // Shimmer Reverb & Choke Gate Tail
    float reverbSize = 0.55f;     // 0.1..1.0
    float reverbDecayMs = 380.0f; // 50..2500 ms
    float reverbDamp = 0.25f;     // 0..1 high damping
    float reverbGateMs = 180.0f;  // 0 (off) .. 600 ms
    float reverbMix = 0.24f;      // 0..1
    float velocityReverb = 1.0f;

    bool operator==(const HatSynthSettings&) const noexcept = default;
};

[[nodiscard]] inline float midiNoteToHz(int noteNumber) noexcept {
    const int midiNote = std::clamp(noteNumber, 0, 127);
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

// Biquad filter state for audio filtering
struct BiquadState {
    float x1 = 0.0f;
    float x2 = 0.0f;
    float y1 = 0.0f;
    float y2 = 0.0f;

    void reset() noexcept {
        x1 = x2 = y1 = y2 = 0.0f;
    }
};

// Clean stereo frame
struct StereoFrame {
    float left = 0.0f;
    float right = 0.0f;
};

struct HatSynthLayers {
    StereoFrame air;
    StereoFrame chick;
    StereoFrame reverb;
    StereoFrame output;
};

// Stereo allpass diffuser for smooth cymbal shimmer dispersion
struct AllpassDiffuser {
    std::vector<float> buffer;
    size_t writeIndex = 0;
    float feedback = 0.6f;

    void prepare(size_t delaySamples, float g = 0.6f) {
        buffer.assign(std::max<size_t>(1, delaySamples), 0.0f);
        writeIndex = 0;
        feedback = g;
    }

    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    [[nodiscard]] float process(float in) noexcept {
        if (buffer.empty()) {
            return in;
        }
        const float delayed = buffer[writeIndex];
        const float out = -feedback * in + delayed;
        buffer[writeIndex] = in + feedback * delayed;
        writeIndex = (writeIndex + 1) % buffer.size();
        return out;
    }
};

// Algorithmic 4-line feedback delay network (FDN) shimmer reverb with gate choke
class GatedStereoReverb {
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    [[nodiscard]] StereoFrame process(
        float inLeft, float inRight,
        float size, float decayMs, float damp, float gateMs,
        double sampleRate) noexcept;

private:
    std::array<AllpassDiffuser, 2> inputDiffusersL_;
    std::array<AllpassDiffuser, 2> inputDiffusersR_;

    std::array<std::vector<float>, 4> delayLines_;
    std::array<size_t, 4> writeIndices_{};
    std::array<size_t, 4> delayLengths_{};
    std::array<float, 4> dampStates_{};

    float hpfStateL_ = 0.0f;
    float hpfStateR_ = 0.0f;
    int sampleCounter_ = 0;
    float gateEnvelope_ = 1.0f;
};

// Studio-Grade Tunable Open Hi-Hat Synthesizer
class HatSynth {
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    void trigger(const HatSynthSettings& settings, float velocity = 1.0f) noexcept;
    void triggerMidi(const HatSynthSettings& settings, float velocity, int midiNoteNumber, int midiChannel = 1) noexcept;
    void noteOff(int midiNoteNumber, int midiChannel, const HatSynthSettings& settings) noexcept;
    [[nodiscard]] StereoFrame processSample(const HatSynthSettings& settings) noexcept;
    [[nodiscard]] HatSynthLayers processSampleLayers(const HatSynthSettings& settings) noexcept;
    void choke() noexcept;
    [[nodiscard]] bool isPlaying() const noexcept { return isPlaying_; }
    [[nodiscard]] float getEnvelopeProgress() const noexcept;

    // Offline render helpers.
    void renderBlock(
        std::vector<float>& leftOut,
        std::vector<float>& rightOut,
        const HatSynthSettings& settings,
        float velocity = 1.0f) noexcept;
    void renderMidiBlock(
        std::vector<float>& leftOut,
        std::vector<float>& rightOut,
        const HatSynthSettings& settings,
        float velocity,
        int midiNoteNumber) noexcept;
    int renderMidiBlock(
        std::span<float> leftOut,
        std::span<float> rightOut,
        const HatSynthSettings& settings,
        float velocity,
        int midiNoteNumber) noexcept;
    void copyRandomStateFrom(const HatSynth& source) noexcept { prngState_ = source.prngState_; }

private:
    void start(const HatSynthSettings& settings, float velocity) noexcept;
    [[nodiscard]] float fullAdsrEnvelopeAt(int sample, const HatSynthSettings& settings) const noexcept;
    [[nodiscard]] float generateNoiseSample(NoiseType type) noexcept;
    [[nodiscard]] float generateChickSample(ChickType type, float freqHz, int ageSamples, float level) noexcept;

    double sampleRate_ = 48000.0;
    bool isPlaying_ = false;
    float velocity_ = 1.0f;
    float midiPitchRatio_ = 1.0f; // the stick strike follows the played note; the air layer never does
    bool fullAdsrActive_ = false;
    int activeMidiNote_ = -1;
    int activeMidiChannel_ = 0;
    int adsrReleaseStartSample_ = -1;
    int adsrReleaseSamples_ = 0;
    float adsrReleaseStartLevel_ = 0.0f;

    int totalSamplesPlayed_ = 0;
    int totalDurationSamples_ = 0;
    int dryDurationSamples_ = 0;
    int chokeStartSample_ = -1;
    int chokeFadeSamples_ = 0;

    // Stick micro-bounce articulation
    int preChickCount_ = 1;
    std::array<int, 5> burstStartSamples_{};
    std::array<float, 5> burstAmplitudes_{};
    std::array<float, 5> burstPans_{};
    int mainHitSample_ = 0;

    // Filter states for air sizzle noise (4-pole 24dB/oct cascaded filters)
    BiquadState noiseHPF1_L_{};
    BiquadState noiseHPF1_R_{};
    BiquadState noiseHPF2_L_{};
    BiquadState noiseHPF2_R_{};
    BiquadState noiseLPF1_L_{};
    BiquadState noiseLPF1_R_{};
    BiquadState noiseLPF2_L_{};
    BiquadState noiseLPF2_R_{};

    // Filter cache for noise cutoff
    float lastNoiseHPF_ = -1.0f;
    float lastNoiseLPF_ = -1.0f;
    float b0_nh1_ = 0.0f, b1_nh1_ = 0.0f, b2_nh1_ = 0.0f, a1_nh1_ = 0.0f, a2_nh1_ = 0.0f;
    float b0_nh2_ = 0.0f, b1_nh2_ = 0.0f, b2_nh2_ = 0.0f, a1_nh2_ = 0.0f, a2_nh2_ = 0.0f;
    float b0_nl1_ = 0.0f, b1_nl1_ = 0.0f, b2_nl1_ = 0.0f, a1_nl1_ = 0.0f, a2_nl1_ = 0.0f;
    float b0_nl2_ = 0.0f, b1_nl2_ = 0.0f, b2_nl2_ = 0.0f, a1_nl2_ = 0.0f, a2_nl2_ = 0.0f;

    // PRNG and noise state (independent L/R filter/hold state for true stereo air)
    uint32_t prngState_ = 0x854378A1;
    float pinkB0_L_ = 0.0f, pinkB1_L_ = 0.0f, pinkB2_L_ = 0.0f;
    float pinkB0_R_ = 0.0f, pinkB1_R_ = 0.0f, pinkB2_R_ = 0.0f;
    float lofiHeld_L_ = 0.0f, lofiHeld_R_ = 0.0f;
    int lofiCounter_L_ = 0, lofiCounter_R_ = 0;

    // Stereo phase diffuser for silk air decorrelation
    AllpassDiffuser noiseDiffL_{};
    AllpassDiffuser noiseDiffR_{};

    // Gated Shimmer Reverb
    GatedStereoReverb reverb_;
};

} // namespace openhatter::core
