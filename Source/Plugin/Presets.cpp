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

// spacing, bounce decay, fallback bounce, fallback attack, fallback hold, fallback curve,
// air level, exciter drive, compressor mix, room size, room ms, room gate.
constexpr std::array<Character, 6> kStyles {{
    { 10, 5, .16f, .4f, 6, -.55f, .86f, .06f, .16f, .36f, 320, 200 }, // 0 Progressive
    {  9, 5, .14f, .4f, 8, -.45f, .86f, .08f, .20f, .42f, 380, 240 }, // 1 Uplifting
    {  8, 4, .14f, .3f, 4, -.60f, .86f, .07f, .14f, .32f, 280, 180 }, // 2 Classic
    {  6, 3, .12f, .2f, 2, -.80f, .84f, .14f, .26f, .24f, 180, 120 }, // 3 Tech
    {  7, 4, .14f, .3f, 3, -.55f, .86f, .10f, .18f, .30f, 240, 160 }, // 4 Psy
    {  8, 4, .14f, .4f, 5, -.55f, .86f, .07f, .18f, .34f, 300, 190 }  // 5 Rolling
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
    float curve, attack, hold, bounce, outGain;
};

// texture: 0 909, 1 silk, 2 velvet, 3 lofi, 4 808, 5 bronze, 6 sizzle, 7 comb
// chick: 0 stick, 1 pedal, 2 laser
// trailing fields: curve, attack ms, hold ms, bounce level, output gain dB
constexpr std::array<Hat, 56> kDefinitions {{
    // Uplifting Trance (24)
    { "Uplifting / Euphoria Open",      1, 0,  520, 17000,  580, 1.26f, -.08f, .36f, .16f, .38f, .40f, 180, 0, .32f,  7400, 1, .10f, .00f, -.40f, 0.6f, 12, .12f, -2.12f },
    { "Uplifting / 138 Anthem Bronze",  1, 5,  420, 16800,  520, 1.18f, +.04f, .48f, .22f, .62f, .42f, 210, 0, .34f,  7600, 1, .09f, .00f, -.48f, 0.6f, 10, .12f, -1.34f },
    { "Uplifting / Pearl Horizon",      1, 1, 1100, 17800,  380, 1.18f, +.34f, .30f, .08f, .16f, .62f, 120, 0, .28f,  9800, 1, .08f, .00f, -.58f, 0.6f,  6, .08f, -0.41f },
    { "Uplifting / Skyward Wash",       1, 7,  520, 17000,  760, 1.32f, +.08f, .62f, .16f, .34f, .40f, 240, 0, .28f,  7000, 1, .14f, .00f, -.32f, 2.4f, 18, .10f, -1.84f },
    { "Uplifting / White Flare Sizzle", 1, 6, 1400, 19000,  360, 1.20f, +.36f, .40f, .14f, .16f, .62f, 110, 2, .34f, 10400, 1, .07f, .00f, -.62f, 0.3f,  4, .10f, -2.98f },
    { "Uplifting / Golden 909 Open",    1, 0, 1800, 16800,  340, 1.06f, +.22f, .62f, .06f, .10f, .58f,  90, 0, .46f, 10000, 1, .04f, .00f, -.70f, 0.2f,  2, .08f, -1.82f },
    { "Uplifting / Starlight Ride",     1, 5,  360, 17500,  980, 1.30f, +.16f, .92f, .18f, .55f, .46f, 280, 0, .26f,  8000, 2, .12f, .00f, -.22f, 1.0f, 16, .16f, -2.37f },
    { "Uplifting / Analog 808 Air",     1, 4,  700, 11200,  460, 1.10f, -.06f, .46f, .28f, .40f, .32f, 180, 0, .38f,  6800, 1, .07f, .00f, -.52f, 0.4f,  6, .12f, -1.09f },
    { "Uplifting / Long Arc Sizzle",    1, 6,  800, 18800, 1100, 1.34f, +.22f, .48f, .16f, .24f, .58f, 260, 0, .24f,  9000, 1, .13f, .08f, -.24f, 1.6f, 20, .10f, -2.62f },
    { "Uplifting / Sunlit Tight Closed",1, 5, 1600, 17500,   78, 1.04f, +.32f, .40f, .10f, .36f, .64f,  55, 0, .58f, 10400, 1, .03f, .00f, -.88f, 0.2f,  0, .08f, -2.30f },
    { "Uplifting / Driving Offbeat 140",1, 4,  860, 12400,  300, 1.08f, +.02f, .50f, .24f, .36f, .38f, 140, 0, .42f,  8400, 1, .05f, .00f, -.64f, 0.3f,  4, .12f, -1.60f },
    { "Uplifting / Crystal Ribbon",     1, 2, 1200, 18600,  480, 1.26f, +.28f, .58f, .12f, .20f, .60f, 150, 2, .32f, 10800, 1, .09f, .00f, -.46f, 0.8f, 10, .10f, -2.01f },
    { "Uplifting / Cloudburst Swell",   1, 7,  480, 16600,  640, 1.30f, +.02f, .66f, .18f, .30f, .36f, 220, 0, .26f,  6400, 2, .12f, .10f, -.30f, 6.0f, 12, .12f, -1.55f },
    { "Uplifting / Solar Pump",         1, 0,  760, 16000,  420, 1.14f, -.10f, .48f, .20f, .32f, .34f, 160, 1, .40f,  5600, 1, .07f, .00f, -.55f, 0.5f,  8, .14f, -3.15f },
    { "Uplifting / Afterglow Wide",     1, 1,  340, 14000,  820, 1.40f, -.18f, .34f, .14f, .34f, .30f, 260, 0, .22f,  5600, 1, .14f, .00f, -.28f, 2.2f, 24, .10f, -1.61f },
    { "Uplifting / Mainstage Splash",   1, 5,  300, 17200, 1080, 1.28f, +.08f, .44f, .20f, .68f, .40f, 320, 0, .30f,  7800, 1, .16f, .00f, -.20f, 0.8f, 14, .14f, -2.35f },
    { "Uplifting / Neon Sixteenth",     1, 0, 2800, 18400,   74, 1.00f, +.26f, .30f, .08f, .12f, .66f,  52, 0, .58f, 11800, 1, .03f, .00f, -.92f, 0.2f,  0, .06f, -2.04f },
    { "Uplifting / Pure 138 Bronze",    1, 5,  260, 14800,  480, 1.12f, -.20f, .44f, .24f, .68f, .32f, 220, 0, .32f,  6400, 1, .08f, .00f, -.42f, 0.5f,  8, .14f, -1.31f },
    { "Uplifting / Silver Comb Lift",   1, 7,  880, 17800,  500, 1.24f, +.24f, .60f, .14f, .22f, .52f, 160, 2, .30f, 10000, 1, .08f, .00f, -.44f, 0.5f,  6, .10f, -1.74f },
    { "Uplifting / Arena Shaker Hat",   1, 2,  980, 17000,  260, 1.18f, +.14f, .64f, .16f, .24f, .56f, 110, 0, .34f,  7600, 2, .08f, .00f, -.70f, 0.3f,  2, .55f, -1.35f },
    { "Uplifting / Twilight Bronze",    1, 5,  280, 14000,  540, 1.14f, -.22f, .42f, .26f, .70f, .30f, 230, 1, .42f,  4800, 1, .10f, .00f, -.40f, 0.6f,  8, .16f, -1.79f },
    { "Uplifting / Peak Time Ride",     1, 6,  640, 18600,  860, 1.32f, +.30f, .46f, .14f, .20f, .64f, 220, 0, .28f,  9600, 1, .12f, .08f, -.26f, 0.8f, 12, .10f, -2.31f },
    { "Uplifting / Satin Closed Tick",  1, 2, 1800, 16800,   70, 1.08f, +.20f, .48f, .10f, .12f, .60f,  48, 0, .42f,  9800, 1, .02f, .00f, -.86f, 0.5f,  2, .10f, -1.4f },
    { "Uplifting / Endless Horizon",    1, 0, 1400, 18000,  920, 1.34f, +.28f, .40f, .10f, .20f, .55f, 160, 0, .22f,  9000, 1, .14f, .08f, -.24f, 1.4f, 16, .10f, -2.4f },

    // Rolling (8)
    { "Rolling / Heavy Floor Open",     5, 4,  280,  7600,  340, 0.98f, -.42f, .58f, .42f, .66f, .22f, 200, 0, .46f,  5200, 1, .05f, .00f, -.58f, 0.3f,  4, .16f, -1.49f },
    { "Rolling / Warm Club Pocket",     5, 1,  380, 12000,  280, 1.02f, -.28f, .32f, .16f, .40f, .28f, 160, 0, .34f,  5200, 1, .05f, .00f, -.55f, 0.8f,  8, .12f, -1.6f },
    { "Rolling / Brushed Sixteen",      5, 2, 1400, 16500,  140, 1.08f, +.22f, .55f, .10f, .14f, .58f,  70, 0, .30f,  8600, 2, .04f, .00f, -.74f, 0.4f,  2, .42f, -0.35f },
    { "Rolling / Open Shutter",         5, 6,  720, 17600,  450, 1.16f, +.18f, .44f, .14f, .22f, .54f, 150, 0, .32f,  8800, 1, .07f, .00f, -.48f, 0.4f,  6, .10f, -1.99f },
    { "Rolling / Punchy Pedal",         5, 5,  260,  9800,  130, 0.96f, -.18f, .36f, .22f, .64f, .26f, 110, 1, .56f,  4200, 1, .04f, .00f, -.74f, 0.3f,  2, .14f, -3.81f },
    { "Rolling / Groove Flam",          5, 2,  500, 13000,  200, 1.04f, -.16f, .48f, .18f, .30f, .34f, 110, 0, .36f,  5600, 2, .05f, .00f, -.66f, 0.3f,  2, .60f, -1.6f },
    { "Rolling / Midnight Mirrors",     5, 7,  460, 16800,  560, 1.22f, +.12f, .58f, .16f, .28f, .40f, 180, 2, .30f,  9200, 1, .10f, .12f, -.34f, 0.8f,  8, .12f, -1.24f },
    { "Rolling / Bronze Driver",        5, 5,  900, 17200,  260, 1.14f, +.26f, .34f, .12f, .40f, .62f, 100, 0, .40f,  9800, 1, .05f, .00f, -.68f, 0.3f,  2, .10f, -1.58f },

    // Progressive (8)
    { "Progressive / Silk Offbeat",     0, 1,  560, 15400,  460, 1.20f, +.08f, .34f, .08f, .24f, .40f, 160, 0, .28f,  7200, 1, .08f, .00f, -.46f, 1.0f, 12, .10f, -0.83f },
    { "Progressive / Deep Alloy",       0, 5,  240, 13600,  430, 1.08f, -.28f, .46f, .24f, .66f, .28f, 200, 0, .34f,  6200, 1, .07f, .00f, -.44f, 0.5f,  8, .12f, -1.31f },
    { "Progressive / After Hours",      0, 7,  300, 13200,  620, 1.24f, -.16f, .52f, .18f, .36f, .32f, 240, 1, .30f,  4600, 1, .12f, .00f, -.30f, 1.8f, 14, .14f, -0.96f },
    { "Progressive / Satin Rim",        0, 1, 1100, 14600,  170, 1.06f, +.02f, .30f, .06f, .20f, .50f,  90, 0, .32f,  6800, 2, .05f, .00f, -.72f, 0.5f,  3, .36f, -0.93f },
    { "Progressive / Smoked Bronze",    0, 4,  240,  8200,  560, 1.02f, -.48f, .36f, .38f, .70f, .18f, 280, 1, .34f,  3400, 1, .08f, .00f, -.36f, 0.6f, 10, .16f, -1.37f },
    { "Progressive / Horizon Drift",    0, 6,  600, 17400,  640, 1.30f, +.16f, .42f, .12f, .18f, .56f, 200, 0, .26f,  8400, 1, .11f, .00f, -.34f, 1.2f, 14, .08f, -2.33f },
    { "Progressive / Warm Shaker",      0, 2,  760, 15000,  280, 1.16f, -.02f, .60f, .12f, .22f, .46f, 120, 0, .30f,  6600, 2, .06f, .00f, -.64f, 0.4f,  3, .52f, -1.93f },
    { "Progressive / Closed Felt",      0, 1, 1500, 15000,   80, 1.00f, +.12f, .24f, .06f, .16f, .55f,  50, 0, .40f,  6400, 1, .03f, .00f, -.86f, 0.8f,  2, .08f, 1.55f },

    // Psy (6)
    { "Psy / Spectral Orbit",           4, 7,  980, 18400,  340, 1.18f, +.18f, .70f, .20f, .18f, .58f, 120, 2, .36f, 11200, 1, .06f, .00f, -.58f, 0.3f,  3, .12f, -1.20f },
    { "Psy / Twin Laser Metal",         4, 4, 1100, 16800,  280, 1.16f, +.28f, .46f, .16f, .18f, .62f,  80, 2, .34f, 12800, 2, .05f, .00f, -.62f, 0.2f,  2, .46f, -0.92f },
    { "Psy / Fractal Shimmer",          4, 5,  640, 18000,  420, 1.22f, +.20f, .36f, .16f, .34f, .55f, 150, 0, .32f,  9800, 1, .07f, .00f, -.50f, 0.4f,  5, .10f, -1.99f },
    { "Psy / Crunch Cog",               4, 3, 1400, 15500,  210, 1.12f, +.24f, .50f, .36f, .16f, .64f,  70, 2, .36f, 11000, 1, .04f, .00f, -.64f, 0.2f,  2, .10f, -2.2f },
    { "Psy / Liquid Combs",             4, 7,  540, 17600,  540, 1.28f, +.06f, .74f, .18f, .20f, .44f, 170, 2, .28f, 10200, 1, .10f, .10f, -.36f, 0.6f,  8, .12f, -1.90f },
    { "Psy / Acid Top",                 4, 6, 1600, 19200,  220, 1.10f, +.40f, .50f, .22f, .14f, .68f,  90, 2, .42f, 12400, 1, .05f, .00f, -.70f, 0.2f,  2, .08f, -2.06f },

    // Tech (5)
    { "Tech / Industrial Bronze",       3, 4,  360,  7200,  130, 0.86f, -.28f, .64f, .62f, .42f, .30f,  90, 0, .54f,  6400, 1, .02f, .00f, -.86f, 0.15f, 0, .08f, -3.04f },
    { "Tech / Dark Warehouse",          3, 5,  220, 11000,  280, 0.92f, -.34f, .40f, .30f, .72f, .22f, 160, 1, .46f,  3600, 1, .05f, .00f, -.60f, 0.3f,  3, .12f, -3.75f },
    { "Tech / Black Chrome",            3, 3,  600,  6400,   90, 0.84f, -.30f, .70f, .58f, .28f, .36f,  70, 0, .52f,  5400, 1, .02f, .00f, -.88f, 0.15f, 0, .06f, -2.0f },
    { "Tech / Raw Relay",               3, 0,  480,  9000,   96, 0.90f, -.24f, .34f, .22f, .18f, .40f,  60, 0, .50f,  7200, 1, .02f, .00f, -.90f, 0.15f, 0, .08f, 0.14f },
    { "Tech / Steel Pump",              3, 7,  420, 14000,  240, 0.98f, -.08f, .48f, .24f, .38f, .34f, 120, 1, .44f,  5200, 1, .04f, .00f, -.68f, 0.3f,  2, .12f, 0.03f },

    // Classic (5)
    { "Classic / 909 Full Bronze",      2, 0,  800, 16000,  480, 1.08f, +.00f, .50f, .12f, .34f, .42f, 170, 0, .42f,  8400, 1, .06f, .00f, -.55f, 0.3f,  6, .12f, -1.68f },
    { "Classic / 808 Analog Open",      2, 4,  900, 14500,  680, 1.14f, +.16f, .40f, .18f, .26f, .48f, 160, 0, .30f,  8200, 1, .08f, .00f, -.38f, 0.5f, 10, .10f, -1.05f },
    { "Classic / Heavy Bronze Ride",    2, 5,  260, 16200,  880, 1.16f, -.04f, .92f, .20f, .74f, .36f, 340, 0, .28f,  7000, 1, .12f, .00f, -.24f, 0.6f, 12, .14f, -1.21f },
    { "Classic / Pedal Choke",          2, 1,  180,  7200,  110, 0.88f, -.36f, .28f, .20f, .70f, .24f,  90, 1, .60f,  2800, 1, .03f, .00f, -.80f, 0.2f,  1, .12f, -0.64f },
    { "Classic / Studio Brass",         2, 5,  900, 13500,  360, 1.08f, +.30f, .28f, .10f, .30f, .66f, 120, 0, .36f, 10200, 1, .06f, .00f, -.62f, 0.3f,  4, .08f, -1.8f },
}};

FactoryPreset makePreset(const Hat& hat, const Character& style) {
    return { hat.name, {
        { kDecayScale, 1 }, { kEchoMix, hat.echoMix },
        { kAmpAttack, .20f }, { kAmpDecay, 30 }, { kAmpSustain, 1.f }, { kAmpRelease, 36 },
        { kNoiseType, static_cast<float>(hat.texture) }, { kNoiseAttack, hat.attack },
        { kNoiseHold, hat.hold }, { kNoiseDecay, hat.airMs }, { kNoiseCurve, hat.curve },
        { kNoiseHPF, hat.highpass }, { kNoiseLPF, hat.lowpass }, { kNoiseStereoWidth, hat.width },
        { kNoiseLevel, style.airLevel },
        { kHissColor, hat.hissColor }, { kHissDensity, hat.hissDensity }, { kHissDrive, hat.hissDrive },
        { kBodyLevel, hat.bodyLevel }, { kBodyTone, hat.bodyTone }, { kBodyDecay, hat.bodyMs },
        { kPreBursts, static_cast<float>(hat.strikes) }, { kPreSpread, style.bounceSpacing },
        { kPreDecay, style.bounceDecay }, { kPreJitter, .03f }, { kFlamSpread, .22f },
        { kPreLevel, hat.bounce },
        { kClickType, static_cast<float>(hat.chickType) }, { kClickLevel, hat.chickLevel },
        { kClickFreq, hat.chickHz }, { kKeyTracking, 0 }, { kKeyTracking, 0 },
        { kDistType, 0 }, { kDistDrive, style.drive }, { kDistTone, 0.05f }, { kDistMix, .06f },
        { kCompThresh, -18 }, { kCompRatio, 2.0f }, { kCompAttack, 6 }, { kCompRelease, 80 },
        { kCompMakeup, 0.0f }, { kCompMix, style.compressorMix }, { kLimiterEnabled, 1 },
        { kLimiterCeiling, -1.0f }, { kLimiterRelease, 18 }, { kReverbSize, style.roomSize },
        { kReverbDecay, style.roomMs }, { kReverbDamp, .42f }, { kReverbGate, style.roomGate },
        { kReverbMix, hat.roomMix }, { kReverseMode, 0 }, { kReverseMix, 0 },
        { kEqLowFreq, 90.0f }, { kEqLowDb, 0.0f }, { kEqMidFreq, 3200.0f }, { kEqMidDb, 0.0f },
        { kEqHighFreq, 9000.0f }, { kEqHighDb, 0.0f }, { kStereoWidth, 1.0f }, { kOutputGain, hat.outGain }
    } };
}

[[nodiscard]] int sectionFor(const juce::String& id) {
    if (id == kPreBursts || id == kPreSpread || id == kPreDecay || id == kPreJitter
        || id == kFlamSpread || id == kPreLevel || id == kClickType || id == kClickLevel
        || id == kClickFreq || id == kBodyLevel || id == kBodyTone || id == kBodyDecay
        || id == kKeyTracking) return 0;
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

void applyValues(const std::vector<std::pair<juce::String, float>>& values,
                 juce::AudioProcessorValueTreeState& state,
                 const std::array<bool, 7>& lockedSections) {
    std::ranges::for_each(values, [&](const auto& entry) {
        const auto& [paramId, value] = entry;
        if (lockedSections[static_cast<size_t>(sectionFor(paramId))]) return;
        if (auto* param = state.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    });
}

void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state,
           const std::array<bool, 7>& lockedSections) {
    applyValues(preset.values, state, lockedSections);
}

} // namespace openhatter::presets
