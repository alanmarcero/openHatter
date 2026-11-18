#include "Presets.h"
#include "Parameters.h"

#include <algorithm>
#include <array>

namespace openhatter::presets {

using namespace openhatter::params;

namespace {

struct Character {
    float bounceSpacing, bounceDecay, bounceLevel;
    float bellQ, sweep, sweepMs;
    float airAttack, airHold, airCurve, airLevel;
    float drive, compressorMix, roomSize, roomMs;
};

constexpr std::array<Character, 6> kStyles {{
    { 5, 4, .38f, 2.8f,  1, 14, .8f,  4, -.15f, .88f, .02f, .38f, .26f, 170 },
    { 7, 3, .34f, 4.2f,  4, 10, .45f, 5, -.28f, .92f, .03f, .42f, .38f, 220 },
    { 6, 5, .46f, 3.1f,  2, 12, .65f, 6, -.10f, .87f, .07f, .35f, .22f, 150 },
    { 3, 3, .35f, 3.8f,  3,  8, .2f,  3, -.34f, .94f, .04f, .48f, .16f, 110 },
    { 4, 3, .40f, 4.4f,  8,  6, .35f, 3, -.30f, .92f, .04f, .40f, .23f, 140 },
    { 6, 4, .44f, 3.2f,  2,  9, .5f,  5, -.18f, .90f, .03f, .45f, .28f, 160 }
}};

struct Hat {
    const char* name;
    float mode, bellHz, metalMs, metalLevel;
    float texture, highpass, lowpass, airMs, width;
    float chickType, chickLevel, chickHz, strikes, roomMix;
};

constexpr std::array<Hat, 48> kDefinitions {{
    { "Progressive | Silk Offbeat", 1, 3200, 116, .20f, 1, 1650, 17100, 184, 1.12f, 0, .28f, 7600, 1, .02f },
    { "Progressive | Narrow Beam", 0, 2850, 94, .17f, 0, 2300, 15100, 151, .82f, 0, .31f, 8400, 1, 0 },
    { "Progressive | After Hours", 1, 2100, 129, .16f, 2, 1200, 13700, 204, 1.18f, 1, .30f, 5800, 1, .03f },
    { "Progressive | Lifted Edge", 2, 3950, 106, .18f, 0, 2400, 18500, 178, 1.22f, 0, .38f, 8900, 1, .02f },
    { "Progressive | Two Steps", 1, 2950, 102, .17f, 1, 1800, 16200, 165, 1.04f, 0, .29f, 7200, 2, .01f },
    { "Progressive | Amber Rim", 0, 2500, 136, .21f, 1, 1450, 15300, 196, 1.08f, 1, .32f, 6500, 1, .02f },
    { "Progressive | Side Drift", 1, 3700, 123, .15f, 2, 2050, 17700, 211, 1.35f, 0, .25f, 8200, 1, .04f },
    { "Progressive | Downbeat Gap", 3, 4100, 88, .17f, 0, 2600, 16600, 138, .96f, 0, .35f, 9200, 1, 0 },

    { "Uplifting | Pearl Lift", 2, 4400, 139, .21f, 0, 2100, 19000, 208, 1.26f, 0, .34f, 9000, 1, .03f },
    { "Uplifting | Sunlit Tip", 2, 5300, 119, .19f, 0, 2950, 19800, 178, 1.15f, 2, .29f, 11800, 1, .02f },
    { "Uplifting | Wide Ribbon", 1, 3650, 146, .16f, 2, 1900, 18200, 228, 1.42f, 0, .28f, 7900, 1, .05f },
    { "Uplifting | Flyover", 2, 4850, 132, .20f, 1, 2050, 19400, 198, 1.30f, 0, .36f, 9700, 1, .03f },
    { "Uplifting | Glint Pair", 2, 4700, 115, .18f, 0, 2500, 18800, 174, 1.18f, 0, .30f, 9300, 2, .02f },
    { "Uplifting | High Balcony", 1, 3950, 152, .19f, 1, 1550, 17600, 232, 1.33f, 1, .33f, 6900, 1, .04f },
    { "Uplifting | White Flare", 3, 5450, 104, .17f, 0, 2950, 20000, 163, 1.13f, 2, .33f, 12300, 1, .01f },
    { "Uplifting | Long Arc", 2, 4120, 158, .20f, 2, 1800, 19100, 242, 1.36f, 0, .30f, 8650, 1, .04f },

    { "Classic | Pressed Bronze", 0, 3000, 136, .22f, 0, 1550, 16000, 192, 1.02f, 0, .34f, 7300, 1, .02f },
    { "Classic | Pedal Board", 0, 2450, 101, .20f, 1, 1400, 14500, 153, .85f, 1, .38f, 5600, 1, .01f },
    { "Classic | Two Machines", 3, 3550, 109, .19f, 0, 2000, 17300, 170, 1.06f, 0, .32f, 8150, 2, .01f },
    { "Classic | Rounded Lip", 1, 2200, 141, .17f, 2, 1250, 13800, 198, 1.13f, 1, .29f, 6100, 1, .03f },
    { "Classic | Silver Stamp", 0, 4100, 113, .21f, 0, 2250, 18200, 175, 1.08f, 0, .39f, 8850, 1, .01f },
    { "Classic | Room Mic", 0, 2750, 149, .19f, 1, 1500, 15300, 213, 1.24f, 1, .32f, 6550, 1, .05f },
    { "Classic | Short Throw", 3, 3300, 89, .18f, 0, 2400, 16500, 138, .90f, 0, .34f, 8000, 1, 0 },
    { "Classic | Spun Alloy", 0, 3850, 132, .23f, 1, 2050, 17800, 194, 1.14f, 0, .34f, 8250, 1, .02f },

    { "Tech | Dry Contact", 3, 3900, 81, .18f, 0, 2750, 17700, 126, .87f, 0, .36f, 9000, 1, 0 },
    { "Tech | Pin Switch", 3, 4700, 72, .16f, 0, 3300, 19000, 113, .92f, 2, .30f, 11100, 1, 0 },
    { "Tech | Brake Dust", 0, 2850, 104, .20f, 1, 2100, 14300, 149, 1.00f, 1, .36f, 6100, 1, .01f },
    { "Tech | Twin Relay", 3, 4320, 89, .19f, 0, 2900, 17900, 137, 1.04f, 0, .32f, 9200, 2, .01f },
    { "Tech | Black Chrome", 3, 3290, 98, .21f, 3, 2450, 15500, 151, .96f, 0, .35f, 7800, 1, 0 },
    { "Tech | Cut Steel", 0, 4650, 86, .20f, 0, 3000, 18700, 134, 1.01f, 0, .38f, 9800, 1, .01f },
    { "Tech | Quiet Valve", 1, 2620, 105, .16f, 2, 1950, 14600, 156, 1.10f, 1, .31f, 6750, 1, .02f },
    { "Tech | Footwork", 3, 4060, 80, .17f, 0, 2850, 16800, 121, .92f, 1, .39f, 8250, 1, 0 },

    { "Psy | Orbit Tick", 2, 4650, 97, .21f, 0, 2650, 18400, 149, 1.09f, 2, .32f, 10500, 1, .01f },
    { "Psy | Triplet Dust", 3, 3780, 76, .16f, 2, 2500, 17100, 122, .98f, 0, .34f, 8600, 1, 0 },
    { "Psy | Twin Pulses", 2, 5250, 93, .20f, 0, 2950, 19400, 142, 1.17f, 2, .29f, 11700, 2, .01f },
    { "Psy | Glass Seed", 2, 4250, 117, .23f, 1, 2050, 17900, 166, 1.16f, 0, .31f, 8200, 1, .02f },
    { "Psy | Quicksilver", 3, 4850, 85, .18f, 0, 3050, 19300, 134, 1.13f, 2, .34f, 11400, 1, 0 },
    { "Psy | Shadow Trail", 1, 2950, 119, .17f, 2, 1750, 15400, 173, 1.26f, 1, .31f, 6750, 2, .03f },
    { "Psy | Dotted Wheel", 0, 3860, 90, .18f, 3, 2600, 17000, 141, 1.06f, 0, .31f, 9450, 2, .01f },
    { "Psy | Prism Edge", 2, 5100, 108, .22f, 0, 2850, 19500, 160, 1.21f, 2, .30f, 10900, 1, .02f },

    { "Club | Main Floor", 0, 3510, 112, .21f, 0, 2050, 17700, 174, 1.10f, 0, .36f, 8300, 1, .01f },
    { "Club | Low Ceiling", 0, 2560, 123, .18f, 1, 1550, 14500, 181, .98f, 1, .36f, 6200, 1, .03f },
    { "Club | Compact Disc", 3, 3910, 86, .18f, 0, 2650, 16900, 136, .90f, 0, .32f, 9000, 1, 0 },
    { "Club | Open Door", 1, 3280, 141, .20f, 2, 1750, 17900, 217, 1.28f, 0, .31f, 7750, 1, .04f },
    { "Club | Kick Return", 3, 4250, 90, .17f, 0, 2800, 18500, 146, 1.02f, 0, .40f, 9500, 1, 0 },
    { "Club | Shuffle Plate", 0, 3050, 114, .20f, 1, 1800, 16400, 170, 1.14f, 0, .35f, 7700, 2, .02f },
    { "Club | Mirror Ball", 2, 4620, 127, .21f, 0, 2450, 19200, 189, 1.23f, 2, .32f, 10300, 1, .03f },
    { "Club | Last Train", 1, 2820, 133, .17f, 1, 1350, 15100, 202, 1.18f, 1, .33f, 6400, 1, .03f },

}};

FactoryPreset makePreset(const Hat& hat, const Character& style) {
    return { hat.name, {
        { kToneMode, hat.mode }, { kToneNote, 6 }, { kToneOctave, 6 }, { kToneFine, 0 },
        { kToneFreq, hat.bellHz }, { kToneQ, style.bellQ }, { kToneDecay, hat.metalMs },
        { kTonePitchEnv, style.sweep }, { kTonePitchDecay, style.sweepMs }, { kToneLevel, hat.metalLevel },
        { kMetalSpread, 0 }, { kMetalDampingSlope, 0 }, { kMetalStereoSpread, 1 },
        { kRingMod, 0 }, { kDecayScale, 1 }, { kEchoMix, 0 },
        { kAmpAttack, .15f }, { kAmpDecay, 48 }, { kAmpSustain, .80f }, { kAmpRelease, 40 },
        { kNoiseType, hat.texture }, { kNoiseAttack, style.airAttack }, { kNoiseHold, style.airHold },
        { kNoiseDecay, hat.airMs }, { kNoiseCurve, style.airCurve }, { kNoiseHPF, hat.highpass },
        { kNoiseLPF, hat.lowpass }, { kNoiseStereoWidth, hat.width }, { kNoiseLevel, style.airLevel },
        { kPreBursts, hat.strikes }, { kPreSpread, style.bounceSpacing }, { kPreDecay, style.bounceDecay },
        { kPreJitter, .02f }, { kFlamSpread, .20f }, { kPreLevel, style.bounceLevel },
        { kClickType, hat.chickType }, { kClickLevel, hat.chickLevel }, { kClickFreq, hat.chickHz },
        { kDistType, 0 }, { kDistDrive, style.drive }, { kDistTone, 0 }, { kDistMix, .08f },
        { kCompThresh, -14 }, { kCompRatio, 2 }, { kCompAttack, 5 }, { kCompRelease, 48 },
        { kCompMakeup, 1 }, { kCompMix, style.compressorMix }, { kLimiterEnabled, 1 },
        { kLimiterCeiling, -1 }, { kLimiterRelease, 18 }, { kReverbSize, style.roomSize },
        { kReverbDecay, style.roomMs }, { kReverbDamp, .42f }, { kReverbGate, 95 },
        { kReverbMix, hat.roomMix }, { kReverseMode, 0 }, { kReverseMix, 0 },
        { kMidiMode, 0 }, { kStereoWidth, 1 }, { kOutputGain, 0 }
    } };
}

[[nodiscard]] int sectionFor(const juce::String& id) {
    if (id == kPreBursts || id == kPreSpread || id == kPreDecay || id == kPreJitter
        || id == kFlamSpread || id == kPreLevel || id == kClickType || id == kClickLevel || id == kClickFreq) return 0;
    if (id == kToneMode || id == kToneNote || id == kToneOctave || id == kToneFine || id == kToneFreq
        || id == kToneQ || id == kToneDecay || id == kTonePitchEnv || id == kTonePitchDecay
        || id == kMetalSpread || id == kMetalDampingSlope || id == kMetalStereoSpread || id == kRingMod || id == kToneLevel
        || id == kAmpAttack || id == kAmpDecay || id == kAmpSustain || id == kAmpRelease || id == kLockPitch) return 1;
    if (id == kNoiseType || id == kNoiseAttack || id == kNoiseHold || id == kNoiseDecay || id == kNoiseCurve
        || id == kNoiseHPF || id == kNoiseLPF || id == kNoiseStereoWidth || id == kNoiseLevel || id == kDecayScale) return 2;
    if (id == kDistType || id == kDistDrive || id == kDistTone || id == kDistMix) return 3;
    if (id == kCompThresh || id == kCompRatio || id == kCompAttack || id == kCompRelease || id == kCompMakeup
        || id == kCompMix || id == kLimiterCeiling || id == kLimiterRelease || id == kLimiterEnabled) return 4;
    return 5;
}

} // namespace

const std::vector<FactoryPreset>& factoryPresets() {
    static const auto presets = [] {
        std::vector<FactoryPreset> result;
        result.reserve(kDefinitions.size());
        constexpr size_t hatsPerStyle = 8;
        static_assert(kStyles.size() * hatsPerStyle == kDefinitions.size());
        for (size_t i = 0; i < kDefinitions.size(); ++i)
            result.push_back(makePreset(kDefinitions[i], kStyles[i / hatsPerStyle]));
        return result;
    }();
    return presets;
}

void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state,
           const std::array<bool, 6>& lockedSections) {
    for (const auto& [paramId, value] : preset.values) {
        if (lockedSections[static_cast<size_t>(sectionFor(paramId))]) continue;
        if (auto* param = state.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

} // namespace openhatter::presets
