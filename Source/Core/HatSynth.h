#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string_view>
#include <vector>

namespace openhatter::core {

inline constexpr int kNumNotes = 12;

inline constexpr std::array<std::string_view, kNumNotes> kNoteNames = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

enum class NoiseType {
    CrispWhite = 0,
    PinkSilk = 1,
    VelvetShimmer = 2,
    LoFiCrunch = 3
};

enum class MetalMode {
    Classic909 = 0,
    AnjunaSilk = 1,
    UpliftingFMBell = 2,
    Analog808Crisp = 3
};

enum class ChickType {
    CrispStick = 0,
    PedalFoot = 1,
    LaserTick = 2
};

struct HatSynthSettings {
    // Pitch & Chromatic Cymbal Tuning
    int note = 6;                 // 0..11 (F#), default F# (classic trance root)
    int octave = 6;               // 1..8, default 6 (hi-hat fundamental register)
    float fineCents = 0.0f;       // -50..+50 cents

    // Pre-Chick / Stick Bounce Articulation
    int preChickCount = 1;        // 1..5 micro-bounces before open ring
    float preChickSpreadMs = 8.0f;// 2..45 ms spacing between bounces
    float preChickDecayMs = 6.0f; // 2..35 ms decay of each bounce
    float preChickJitter = 0.08f; // 0..1 timing humanization
    float preChickFlamSpread = 0.35f; // 0..1 stereo micro-panning spread
    float preChickLevel = 0.65f;  // 0..1 volume of stick bounces

    // Tunable Metallic Cymbal Matrix & Body
    MetalMode metalMode = MetalMode::Classic909;
    float toneFreqHz = 2800.0f;   // 300..8000 Hz metallic body center frequency
    float toneQ = 3.0f;           // 0.5..12.0 resonance / bell focus
    float toneDecayMs = 190.0f;   // 15..800 ms metallic body decay
    float tonePitchEnv = 3.5f;    // -24..+24 semitones stick impact pitch modulation
    float tonePitchDecayMs = 14.0f;// 3..80 ms pitch envelope decay
    float toneLevel = 0.80f;      // 0..1 level of tuned metallic matrix

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

    // Stick Chick / Transient
    ChickType chickType = ChickType::CrispStick;
    float chickLevel = 0.45f;     // 0..1
    float chickFreqHz = 6800.0f;  // 1000..14000 Hz

    // Shimmer Reverb & Choke Gate Tail
    float reverbSize = 0.55f;     // 0.1..1.0
    float reverbDecayMs = 380.0f; // 50..2500 ms
    float reverbDamp = 0.25f;     // 0..1 high damping
    float reverbGateMs = 180.0f;  // 0 (off) .. 600 ms
    float reverbMix = 0.24f;      // 0..1

    bool operator==(const HatSynthSettings&) const noexcept = default;
};

// Computes base tuned frequency from note index, octave, and fine cents.
[[nodiscard]] inline float noteToHz(int noteIndex, int octave, float fineCents = 0.0f) noexcept {
    const int note = (noteIndex % kNumNotes + kNumNotes) % kNumNotes;
    const int oct = std::clamp(octave, 1, 8);
    // MIDI note: Octave 4 C is 60, Octave 4 A is 69 (440 Hz)
    const int midiNote = 12 * (oct + 1) + note;
    const float totalNotes = static_cast<float>(midiNote - 69) + (fineCents / 100.0f);
    return 440.0f * std::pow(2.0f, totalNotes / 12.0f);
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
    void trigger(const HatSynthSettings& settings, float velocity = 1.0f, int semitoneOffset = 0) noexcept;
    [[nodiscard]] StereoFrame processSample(const HatSynthSettings& settings) noexcept;
    [[nodiscard]] bool isPlaying() const noexcept { return isPlaying_; }
    [[nodiscard]] float getEnvelopeProgress() const noexcept;

    // Direct render block for offline preview, tests, or WAV lock-in
    void renderBlock(
        std::vector<float>& leftOut,
        std::vector<float>& rightOut,
        const HatSynthSettings& settings,
        float velocity = 1.0f,
        int semitoneOffset = 0) noexcept;

private:
    [[nodiscard]] float generateNoiseSample(NoiseType type) noexcept;
    [[nodiscard]] float generateChickSample(ChickType type, float freqHz, int ageSamples, float level) noexcept;

    double sampleRate_ = 48000.0;
    bool isPlaying_ = false;
    float velocity_ = 1.0f;
    int semitoneOffset_ = 0;

    int totalSamplesPlayed_ = 0;
    int totalDurationSamples_ = 0;
    int dryDurationSamples_ = 0;

    // Stick micro-bounce articulation
    int preChickCount_ = 1;
    std::array<int, 5> burstStartSamples_{};
    std::array<float, 5> burstAmplitudes_{};
    std::array<float, 5> burstPans_{};
    int mainHitSample_ = 0;

    // PolyBLEP anti-aliased bronze metallic oscillators
    std::array<float, 6> metalPhases_{};
    std::array<float, 6> metalDt_{};

    // Filter states for metallic body
    BiquadState metalBPF_L_{};
    BiquadState metalBPF_R_{};
    BiquadState metalHPF_L_{};
    BiquadState metalHPF_R_{};

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

    // Filter cache for metal body
    float lastMetalFreq_ = -1.0f;
    float lastMetalQ_ = -1.0f;
    float b0_mb_ = 0.0f, b1_mb_ = 0.0f, b2_mb_ = 0.0f, a1_mb_ = 0.0f, a2_mb_ = 0.0f;
    float b0_mh_ = 0.0f, b1_mh_ = 0.0f, b2_mh_ = 0.0f, a1_mh_ = 0.0f, a2_mh_ = 0.0f;

    // PRNG and noise state
    uint32_t prngState_ = 0x854378A1;
    float pinkB0_ = 0.0f, pinkB1_ = 0.0f, pinkB2_ = 0.0f;
    float lofiHeld_ = 0.0f;
    int lofiCounter_ = 0;

    // Stereo phase diffuser for silk air decorrelation
    AllpassDiffuser noiseDiffL_{};
    AllpassDiffuser noiseDiffR_{};

    // Gated Shimmer Reverb
    GatedStereoReverb reverb_;
};

} // namespace openhatter::core
