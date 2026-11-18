#include "Presets.h"
#include "Parameters.h"

#include <algorithm>
#include <array>

namespace openhatter::presets {

using namespace openhatter::params;

namespace {

struct Character {
    float bounceSpacing, bounceDecay, bounceLevel;
    float airAttack, airHold, airCurve, airLevel;
    float drive, compressorMix, roomSize, roomMs, roomGate;
};

constexpr std::array<Character, 6> kStyles {{
    { 5, 4, .42f, 2.0f, 14, +.05f, .88f, .12f, .45f, .42f, 460, 320 }, // 0 Progressive
    { 6, 4, .44f, 1.2f, 16, +.12f, .92f, .16f, .52f, .58f, 780, 440 }, // 1 Uplifting
    { 5, 5, .48f, 0.6f, 12, -.05f, .90f, .18f, .48f, .35f, 380, 260 }, // 2 Classic
    { 3, 3, .42f, 0.4f,  8, -.28f, .90f, .16f, .55f, .22f, 260, 190 }, // 3 Tech
    { 4, 3, .44f, 0.5f,  9, -.22f, .92f, .14f, .50f, .32f, 340, 240 }, // 4 Psy
    { 6, 4, .45f, 0.9f, 13, -.02f, .91f, .14f, .50f, .40f, 420, 300 }  // 5 Rolling
}};

struct Hat {
    const char* name;
    size_t style;
    int texture;
    float highpass, lowpass, airMs, width;
    float hissColor, hissDensity, hissDrive;
    float bodyLevel, bodyTone, bodyMs;
    int chickType;
    float chickLevel, chickHz;
    int strikes;
    float roomMix, echoMix;
};

constexpr std::array<Hat, 56> kDefinitions {{
    // Uplifting Trance (24 presets)
    { "Uplifting / Euphoria Open",      1, 6, 420, 18800,  480, 1.38f, +.25f, .48f, .34f, .62f, .48f, 185, 0, .40f,  8800, 1, .12f, .00f },
    { "Uplifting / 138 Anthem Bronze",  1, 5, 360, 18200,  520, 1.32f, +.10f, .42f, .38f, .68f, .42f, 210, 0, .42f,  8400, 1, .10f, .00f },
    { "Uplifting / Pearl Horizon",      1, 1, 480, 18600,  560, 1.42f, +.30f, .45f, .28f, .58f, .52f, 195, 0, .38f,  9200, 1, .14f, .00f },
    { "Uplifting / Skyward Wash",       1, 7, 380, 17800,  780, 1.48f, +.18f, .55f, .32f, .64f, .46f, 260, 1, .38f,  7400, 1, .18f, .00f },
    { "Uplifting / White Flare Sizzle", 1, 6, 580, 19800,  440, 1.35f, +.45f, .52f, .30f, .55f, .58f, 165, 2, .38f,  9800, 1, .10f, .00f },
    { "Uplifting / Golden 909 Open",    1, 0, 390, 17900,  410, 1.22f, +.05f, .40f, .42f, .72f, .38f, 190, 0, .44f,  8200, 1, .08f, .00f },
    { "Uplifting / Starlight Ride",     1, 5, 440, 19200,  860, 1.44f, +.22f, .48f, .26f, .60f, .55f, 280, 0, .36f,  9500, 2, .15f, .00f },
    { "Uplifting / Analog 808 Air",     1, 4, 340, 17400,  465, 1.30f, +.00f, .44f, .38f, .70f, .40f, 205, 0, .40f,  8100, 1, .09f, .00f },
    { "Uplifting / Long Arc Sizzle",    1, 6, 350, 19400,  980, 1.52f, +.32f, .60f, .34f, .66f, .50f, 320, 0, .38f,  9000, 1, .20f, .00f },
    { "Uplifting / Sunlit Tight Closed",1, 5, 540, 18500,  215, 1.12f, +.20f, .35f, .32f, .58f, .60f, 105, 0, .42f, 10200, 1, .05f, .00f },
    { "Uplifting / Driving Offbeat 140",1, 4, 410, 18600,  380, 1.28f, +.15f, .46f, .36f, .68f, .48f, 165, 0, .42f,  8900, 1, .08f, .00f },
    { "Uplifting / Crystal Ribbon",     1, 2, 620, 19500,  510, 1.45f, +.40f, .50f, .28f, .52f, .62f, 175, 2, .36f, 10400, 1, .14f, .00f },
    { "Uplifting / Cloudburst Swell",   1, 7, 430, 18800,  680, 1.46f, +.12f, .62f, .30f, .62f, .44f, 240, 0, .36f,  8600, 2, .16f, .12f },
    { "Uplifting / Solar Pump",         1, 0, 320, 17200,  490, 1.34f, -.08f, .45f, .44f, .76f, .34f, 220, 1, .44f,  6800, 1, .10f, .00f },
    { "Uplifting / Afterglow Wide",     1, 1, 400, 18200,  640, 1.50f, +.18f, .48f, .30f, .64f, .42f, 230, 0, .35f,  8500, 1, .16f, .00f },
    { "Uplifting / Mainstage Splash",   1, 5, 310, 18900, 1120, 1.48f, +.20f, .56f, .36f, .70f, .49f, 360, 0, .40f,  9100, 1, .22f, .00f },
    { "Uplifting / Neon Sixteenth",     1, 4, 560, 18800,  245, 1.20f, +.25f, .38f, .34f, .56f, .58f, 115, 0, .38f,  9600, 2, .06f, .00f },
    { "Uplifting / Pure 138 Bronze",    1, 5, 440, 19100,  435, 1.32f, +.14f, .40f, .32f, .65f, .50f, 175, 0, .40f,  9000, 1, .10f, .00f },
    { "Uplifting / Silver Comb Lift",   1, 7, 500, 19000,  540, 1.44f, +.28f, .58f, .28f, .58f, .54f, 185, 2, .36f, 10200, 1, .12f, .00f },
    { "Uplifting / Arena Shaker Hat",   1, 2, 520, 18100,  340, 1.36f, +.22f, .55f, .30f, .54f, .56f, 140, 0, .36f,  8400, 3, .10f, .00f },
    { "Uplifting / Twilight Bronze",    1, 6, 330, 16800,  580, 1.38f, -.12f, .46f, .40f, .72f, .36f, 225, 1, .42f,  7200, 1, .14f, .00f },
    { "Uplifting / Peak Time Ride",     1, 6, 460, 19600,  740, 1.46f, +.34f, .54f, .32f, .60f, .54f, 250, 0, .38f,  9400, 1, .16f, .10f },
    { "Uplifting / Satin Closed Tick",  1, 1, 580, 17600,  195, 1.14f, +.08f, .32f, .30f, .58f, .52f,  95, 0, .40f,  9100, 1, .04f, .00f },
    { "Uplifting / Endless Horizon",    1, 5, 360, 18700,  920, 1.54f, +.22f, .52f, .34f, .68f, .46f, 300, 0, .36f,  8800, 1, .20f, .12f },

    // Rolling (8 presets)
    { "Rolling / Heavy Floor Open",     5, 4, 360, 18000,  420, 1.24f, +.05f, .44f, .38f, .72f, .42f, 175, 0, .42f,  8300, 1, .08f, .00f },
    { "Rolling / Warm Club Pocket",     5, 0, 320, 16600,  360, 1.16f, -.10f, .40f, .40f, .70f, .38f, 160, 1, .42f,  6600, 1, .08f, .00f },
    { "Rolling / Brushed Sixteen",      5, 1, 480, 17400,  235, 1.18f, +.12f, .36f, .30f, .60f, .50f, 115, 0, .38f,  8600, 2, .06f, .00f },
    { "Rolling / Open Shutter",         5, 6, 400, 18400,  480, 1.38f, +.20f, .46f, .34f, .62f, .48f, 185, 0, .38f,  8900, 1, .10f, .00f },
    { "Rolling / Punchy Pedal",         5, 5, 340, 16900,  330, 1.14f, -.05f, .38f, .36f, .74f, .40f, 155, 1, .44f,  6900, 1, .07f, .00f },
    { "Rolling / Groove Flam",          5, 2, 460, 17800,  285, 1.26f, +.14f, .45f, .30f, .60f, .52f, 130, 0, .38f,  8400, 3, .08f, .00f },
    { "Rolling / Midnight Mirrors",     5, 7, 420, 18600,  540, 1.36f, +.18f, .52f, .32f, .62f, .46f, 200, 2, .36f,  9600, 1, .12f, .14f },
    { "Rolling / Bronze Driver",        5, 5, 380, 17900,  390, 1.22f, +.04f, .42f, .36f, .70f, .44f, 170, 0, .40f,  8200, 1, .08f, .00f },

    // Progressive (8 presets)
    { "Progressive / Silk Offbeat",     0, 1, 420, 17400,  450, 1.30f, +.10f, .42f, .28f, .64f, .44f, 180, 0, .34f,  7900, 1, .10f, .00f },
    { "Progressive / Deep Alloy",       0, 5, 340, 16000,  380, 1.18f, -.18f, .40f, .34f, .72f, .34f, 170, 0, .38f,  7400, 1, .08f, .00f },
    { "Progressive / After Hours",      0, 7, 300, 15400,  580, 1.36f, -.22f, .48f, .30f, .66f, .32f, 220, 1, .34f,  6200, 1, .16f, .00f },
    { "Progressive / Satin Rim",        0, 1, 500, 16800,  290, 1.22f, +.08f, .36f, .28f, .62f, .48f, 130, 0, .36f,  7800, 2, .07f, .00f },
    { "Progressive / Smoked Bronze",    0, 4, 310, 15800,  410, 1.20f, -.14f, .42f, .40f, .74f, .36f, 185, 1, .38f,  6500, 1, .08f, .00f },
    { "Progressive / Horizon Drift",    0, 6, 440, 18200,  620, 1.46f, +.22f, .46f, .26f, .58f, .48f, 215, 0, .32f,  8500, 1, .14f, .00f },
    { "Progressive / Warm Shaker",      0, 2, 480, 16600,  320, 1.28f, +.05f, .44f, .28f, .60f, .46f, 145, 0, .34f,  7600, 3, .08f, .00f },
    { "Progressive / Closed Felt",      0, 1, 460, 15200,  200, 1.06f, -.16f, .32f, .32f, .66f, .38f, 100, 1, .38f,  6000, 1, .04f, .00f },

    // Psy (6 presets)
    { "Psy / Spectral Orbit",           4, 7, 520, 18800,  360, 1.32f, +.20f, .58f, .34f, .56f, .60f, 145, 2, .40f, 10800, 1, .08f, .00f },
    { "Psy / Twin Laser Metal",         4, 4, 480, 19200,  270, 1.24f, +.16f, .50f, .36f, .60f, .58f, 120, 2, .38f, 11400, 2, .06f, .00f },
    { "Psy / Fractal Shimmer",          4, 5, 450, 18600,  410, 1.34f, +.22f, .68f, .32f, .58f, .54f, 155, 0, .36f,  9200, 1, .09f, .00f },
    { "Psy / Crunch Cog",               4, 3, 440, 17400,  250, 1.18f, +.05f, .48f, .40f, .62f, .50f, 115, 0, .38f,  9400, 2, .05f, .00f },
    { "Psy / Liquid Combs",             4, 7, 410, 18400,  520, 1.42f, +.15f, .62f, .30f, .54f, .52f, 180, 2, .34f, 10400, 1, .12f, .16f },
    { "Psy / Acid Top",                 4, 6, 540, 19400,  310, 1.28f, +.30f, .52f, .34f, .60f, .56f, 135, 2, .38f, 11000, 1, .07f, .00f },

    // Tech (5 presets)
    { "Tech / Industrial Bronze",       3, 4, 380, 17200,  240, 1.06f, -.08f, .46f, .44f, .74f, .42f, 125, 0, .44f,  8800, 1, .04f, .00f },
    { "Tech / Dark Warehouse",          3, 5, 320, 15200,  310, 1.10f, -.24f, .42f, .40f, .76f, .34f, 150, 1, .42f,  6400, 1, .06f, .00f },
    { "Tech / Black Chrome",            3, 3, 420, 16200,  230, 1.04f, -.05f, .48f, .42f, .68f, .46f, 110, 0, .40f,  7900, 1, .04f, .00f },
    { "Tech / Raw Relay",               3, 0, 460, 17600,  210, 1.08f, +.02f, .36f, .38f, .66f, .50f, 105, 0, .42f,  9100, 2, .04f, .00f },
    { "Tech / Steel Pump",              3, 7, 390, 16600,  290, 1.14f, -.10f, .44f, .36f, .70f, .40f, 135, 1, .44f,  7100, 1, .06f, .00f },

    // Classic (5 presets)
    { "Classic / 909 Full Bronze",      2, 0, 320, 17000,  430, 1.18f, -.04f, .38f, .40f, .72f, .40f, 185, 0, .42f,  7800, 1, .08f, .00f },
    { "Classic / 808 Analog Open",      2, 4, 300, 16500,  490, 1.20f, -.08f, .42f, .36f, .74f, .36f, 200, 0, .40f,  7500, 1, .09f, .00f },
    { "Classic / Heavy Bronze Ride",    2, 5, 340, 17800,  720, 1.32f, +.05f, .46f, .34f, .70f, .44f, 250, 0, .38f,  8400, 1, .16f, .00f },
    { "Classic / Pedal Choke",          2, 1, 360, 15400,  240, 1.04f, -.12f, .34f, .32f, .68f, .38f, 120, 1, .44f,  6000, 1, .05f, .00f },
    { "Classic / Studio Brass",         2, 5, 380, 18000,  540, 1.24f, +.05f, .40f, .32f, .68f, .46f, 200, 0, .38f,  8200, 1, .12f, .00f },
}};

FactoryPreset makePreset(const Hat& hat, const Character& style) {
    return { hat.name, {
        { kDecayScale, 1 }, { kEchoMix, hat.echoMix },
        { kAmpAttack, .15f }, { kAmpDecay, 140 }, { kAmpSustain, .94f }, { kAmpRelease, 85 },
        { kNoiseType, static_cast<float>(hat.texture) }, { kNoiseAttack, style.airAttack },
        { kNoiseHold, style.airHold }, { kNoiseDecay, hat.airMs }, { kNoiseCurve, style.airCurve },
        { kNoiseHPF, hat.highpass }, { kNoiseLPF, hat.lowpass }, { kNoiseStereoWidth, hat.width },
        { kNoiseLevel, style.airLevel },
        { kHissColor, hat.hissColor }, { kHissDensity, hat.hissDensity }, { kHissDrive, hat.hissDrive },
        { kBodyLevel, hat.bodyLevel }, { kBodyTone, hat.bodyTone }, { kBodyDecay, hat.bodyMs },
        { kPreBursts, static_cast<float>(hat.strikes) }, { kPreSpread, style.bounceSpacing },
        { kPreDecay, style.bounceDecay }, { kPreJitter, .03f }, { kFlamSpread, .24f },
        { kPreLevel, style.bounceLevel },
        { kClickType, static_cast<float>(hat.chickType) }, { kClickLevel, hat.chickLevel },
        { kClickFreq, hat.chickHz },
        { kDistType, 0 }, { kDistDrive, style.drive }, { kDistTone, 0.1f }, { kDistMix, .14f },
        { kCompThresh, -14 }, { kCompRatio, 2.5f }, { kCompAttack, 5 }, { kCompRelease, 55 },
        { kCompMakeup, 1.5f }, { kCompMix, style.compressorMix }, { kLimiterEnabled, 1 },
        { kLimiterCeiling, -1.0f }, { kLimiterRelease, 22 }, { kReverbSize, style.roomSize },
        { kReverbDecay, style.roomMs }, { kReverbDamp, .34f }, { kReverbGate, style.roomGate },
        { kReverbMix, hat.roomMix }, { kReverseMode, 0 }, { kReverseMix, 0 },
        { kEqLowFreq, 80.0f }, { kEqLowDb, 0.0f }, { kEqMidFreq, 1000.0f }, { kEqMidDb, 0.0f },
        { kEqHighFreq, 7500.0f }, { kEqHighDb, 0.0f }, { kStereoWidth, 1.05f }, { kOutputGain, 0 }
    } };
}

[[nodiscard]] int sectionFor(const juce::String& id) {
    if (id == kPreBursts || id == kPreSpread || id == kPreDecay || id == kPreJitter
        || id == kFlamSpread || id == kPreLevel || id == kClickType || id == kClickLevel
        || id == kClickFreq || id == kBodyLevel || id == kBodyTone || id == kBodyDecay) return 0;
    if (id == kAmpAttack || id == kAmpDecay || id == kAmpSustain || id == kAmpRelease) return 1;
    if (id == kNoiseType || id == kNoiseAttack || id == kNoiseHold || id == kNoiseDecay || id == kNoiseCurve
        || id == kNoiseHPF || id == kNoiseLPF || id == kNoiseStereoWidth || id == kNoiseLevel
        || id == kDecayScale || id == kHissColor || id == kHissDensity || id == kHissDrive) return 2;
    if (id == kDistType || id == kDistDrive || id == kDistTone || id == kDistMix) return 3;
    if (id == kCompThresh || id == kCompRatio || id == kCompAttack || id == kCompRelease || id == kCompMakeup
        || id == kCompMix) return 4;
    if (id == kEqLowFreq || id == kEqLowDb || id == kEqMidFreq || id == kEqMidDb || id == kEqHighFreq
        || id == kEqHighDb || id == kStereoWidth || id == kOutputGain || id == kLimiterCeiling
        || id == kLimiterRelease || id == kLimiterEnabled) return 6;
    return 5;
}

} // namespace

const std::vector<FactoryPreset>& factoryPresets() {
    static const auto presets = [] {
        std::vector<FactoryPreset> result;
        result.reserve(kDefinitions.size());
        std::ranges::transform(kDefinitions, std::back_inserter(result),
                               [](const Hat& hat) { return makePreset(hat, kStyles[hat.style]); });
        return result;
    }();
    return presets;
}

void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state,
           const std::array<bool, 7>& lockedSections) {
    std::ranges::for_each(preset.values, [&](const auto& entry) {
        const auto& [paramId, value] = entry;
        if (lockedSections[static_cast<size_t>(sectionFor(paramId))]) return;
        if (auto* param = state.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    });
}

} // namespace openhatter::presets
