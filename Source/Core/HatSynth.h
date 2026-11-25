#pragma once

#include "Effects.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <span>
#include <string_view>
#include <vector>

namespace openhatter::core {

inline constexpr float kOneShotMaxReleaseMs = 800.0f;

enum class NoiseType {
    CrispWhite = 0,
    PinkSilk = 1,
    VelvetShimmer = 2,
    LoFiCrunch = 3,
    AnalogMetal = 4,
    BronzeCymbal = 5,
    SupersawSizzle = 6,
    CombWash = 7
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

    // Air Sizzle & Shimmer Hiss
    NoiseType noiseType = NoiseType::CrispWhite;
    float noiseAttackMs = 1.5f;   // 0..40 ms (allows off-beat pocket swell)
    float noiseHoldMs = 12.0f;    // 0..120 ms plateau hold before tail decay
    float noiseDecayMs = 420.0f;  // 20..2400 ms open hat tail decay
    float noiseCurve = -0.35f;    // -1..+1 shape curve (-1 exp, 0 lin, +1 sustained shoulder)
    float noiseHPF = 480.0f;      // 60..6000 Hz high-pass filter cutoff
    float noiseLPF = 16500.0f;    // 1500..20000 Hz low-pass filter cutoff
    float noiseStereoWidth = 1.2f;// 0 (mono) .. 2 (ultra-wide stereo shimmer)
    float noiseLevel = 0.88f;     // 0..1
    float velocityCurve = 1.0f;   // one exponent for air, stick, and reverb

    // Hiss Tone, Density & Analog Saturation Options
    float hissColor = 0.0f;       // -1 (warm bronze tilt) .. +1 (airy crystal sheen)
    float hissDensity = 0.35f;    // 0..1 metallic cluster/detune/comb density
    float hissDrive = 0.25f;      // 0..1 pre-envelope analog warmth & saturation

    // Spun Bronze Cymbal Body & Low-Mid Shell Punch
    float bodyLevel = 0.55f;      // 0..1 modal bronze shell + warm chest mix
    float bodyTone = 0.45f;       // 0..1 330 Hz warm chest .. 840 Hz bronze cut
    float bodyDecayMs = 150.0f;   // 25..900 ms bronze shell resonance tail

    // Stick Chick / Transient
    ChickType chickType = ChickType::CrispStick;
    float chickLevel = 0.45f;     // 0..1
    float chickFreqHz = 6800.0f;  // 1000..14000 Hz
    float keyTracking = 1.0f;     // 0..1 fractional MIDI pitch tracking across metallic & body modes

    // Shimmer Reverb & Choke Gate Tail
    float reverbSize = 0.55f;     // 0.1..1.0
    float reverbDecayMs = 620.0f; // 50..2500 ms
    float reverbDamp = 0.25f;     // 0..1 high damping
    float reverbGateMs = 320.0f;  // 0 (off) .. 1500 ms
    float reverbMix = 0.24f;      // 0..1

    bool operator==(const HatSynthSettings&) const noexcept = default;
};

[[nodiscard]] inline float midiNoteToHz(int noteNumber) noexcept {
    const int midiNote = std::clamp(noteNumber, 0, 127);
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

// Pre-warped trapezoidal ZDF state-variable filter state and coefficients.
struct SvfState {
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;

    void reset() noexcept {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }
};

struct SvfCoeffs {
    float warp = 0.0f;
    float damping = 1.0f;
    float a1 = 1.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
    float modalScale = 1.0f;
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

// Jot-style 4-line feedback delay network (FDN) shimmer reverb with S-curve choke gate.
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
    std::array<float, 4> baseDelaySamples_{};
    std::array<float, 4> hfStates_{};
    std::array<float, 4> lfStates_{};
    std::array<float, 4> lfoPhases_{{ 0.0f, 0.25f, 0.50f, 0.75f }};

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
    void copyRandomStateFrom(const HatSynth& source) noexcept {
        burstPrng_ = source.burstPrng_;
        channels_[0].prng = source.channels_[0].prng;
        channels_[1].prng = source.channels_[1].prng;
    }

private:
    static constexpr size_t kBronzeModeCount = 14;
    static constexpr size_t kShellModeCount = 4;

    struct ChannelState {
        uint64_t prng = 0x853C49E6748FEA9BULL;
        SvfState hpf1{};
        SvfState hpf2{};
        SvfState lpf1{};
        SvfState lpf2{};
        std::array<SvfState, kShellModeCount> bodyModes{};
        std::array<SvfState, kBronzeModeCount> bronzeModes{};
        Oversampler4x metalOversampler{};
        AdaaTanh sizzleAdaa{};
        AdaaTanh combAdaa{};
        SvfState sizzleSplit{};
        SvfState sizzlePostHp{};
        float tiltLp = 0.0f;
        float bodySubHp = 0.0f;
        float bodyWarmLp = 0.0f;
        float pinkB0 = 0.0f;
        float pinkB1 = 0.0f;
        float pinkB2 = 0.0f;
        float lofiHeld = 0.0f;
        int lofiCounter = 0;
        std::array<float, 6> metalPhases{};
        std::array<float, 8> sizzlePhases{};
        std::vector<float> combDelay;
        size_t combWrite = 0;
        float combFilter = 0.0f;
        std::array<AllpassDiffuser, 2> diffusers{};
        SvfState modelBp{};
        SvfState modelHp{};
        float shade = 1.0f;
        float shadeLp = 0.0f;
        float ringEnv = 1.0f;
        float stickHp = 0.0f;
        float stickLp = 0.0f;
        float dcX = 0.0f;
        float dcY = 0.0f;
    };

    void start(const HatSynthSettings& settings, float velocity, float pitchRatio = 1.0f) noexcept;
    void updateFilterCaches(const HatSynthSettings& settings) noexcept;
    [[nodiscard]] float trackedPitchRatio(const HatSynthSettings& settings) const noexcept;
    [[nodiscard]] float fullAdsrEnvelopeAt(int sample, const HatSynthSettings& settings) const noexcept;
    [[nodiscard]] float generateHissChannel(
        ChannelState& ch, int channelIdx, float strikeExcite, const HatSynthSettings& settings) noexcept;
    [[nodiscard]] float processBodyChannel(
        ChannelState& ch, int channelIdx, float shellImpulse, float transientBed, float thump) noexcept;
    [[nodiscard]] float generateNoiseSample(NoiseType type) noexcept;
    [[nodiscard]] float generateChickSample(ChickType type, float freqHz, int ageSamples, float level) noexcept;

    double sampleRate_ = 48000.0;
    bool isPlaying_ = false;
    float velocity_ = 1.0f;
    float midiPitchRatio_ = 1.0f; // the stick strike follows the played note; the air/body layer only with key tracking
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

    // Filter caches for air cutoffs, bronze cymbal modes, and shell resonators
    float lastNoiseHPF_ = -1.0f;
    float lastNoiseLPF_ = -1.0f;
    float lastBodyTone_ = -1.0f;
    float lastBodyDecayMs_ = -1.0f;
    float lastDecayScale_ = -1.0f;
    float lastTrackedPitch_ = -1.0f;
    int lastModelType_ = -1;
    float lastModelColor_ = -10.0f;
    float lastModelDensity_ = -10.0f;
    SvfCoeffs nh1Coeffs_{};
    SvfCoeffs nh2Coeffs_{};
    SvfCoeffs nl1Coeffs_{};
    SvfCoeffs nl2Coeffs_{};
    SvfCoeffs sizzleSplitCoeffs_{};
    SvfCoeffs sizzlePostCoeffs_{};
    SvfCoeffs modelBpCoeffs_{};
    SvfCoeffs modelHpCoeffs_{};
    std::array<std::array<SvfCoeffs, kShellModeCount>, 2> bodyModeCoeffs_{};
    std::array<std::array<SvfCoeffs, kBronzeModeCount>, 2> bronzeModeCoeffs_{};
    float stickHpAlpha_ = 0.3f;
    float stickLpAlpha_ = 0.6f;

    // Bronze shell strike thump phase and uncorrelated PRNG seeds
    float chestPhase_ = 0.0f;
    uint64_t burstPrng_ = 0x9E3779B97F4A7C15ULL;
    std::array<ChannelState, 2> channels_{
        ChannelState{ .prng = 0x853C49E6748FEA9BULL },
        ChannelState{ .prng = 0xDA3E39CB94B95BDBULL }
    };

    // Gated Shimmer Reverb
    GatedStereoReverb reverb_;
};

} // namespace openhatter::core
