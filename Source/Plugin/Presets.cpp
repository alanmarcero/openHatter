#include "Presets.h"
#include "Parameters.h"

#include <array>

namespace openhatter::presets {

using namespace openhatter::params;

namespace {

struct Articulation {
    float bounceCount, spreadMs, decayMs, humanize, pan, bounceLevel;
    float chickType, chickLevel, chickFrequency;
};

struct Cymbal {
    float mode, note, octave, fineCents, bellFrequency, bellQ, metalDecay;
    float pitchChick, pitchDecay, metalLevel;
    float attack, decay, sustain, release;
};

struct Air {
    float texture, attack, hold, decay, curve, highpass, lowpass, width, level;
};

struct Exciter {
    float type, drive, tilt, mix;
};

struct Dynamics {
    float threshold, ratio, attack, release, makeup, mix;
    float limiterCeiling, limiterRelease, limiterEnabled;
};

struct Space {
    float size, decay, damping, choke, mix, reverseMode, reverseMix;
    float midiMode, masterWidth, outputGain;
};

struct PresetDefinition {
    const char* name;
    Articulation articulation;
    Cymbal cymbal;
    Air air;
    Exciter exciter;
    Dynamics dynamics;
    Space space;
};

// Values are authored per preset in these field groups:
// articulation = bounces, spread, bounce decay, humanize, pan, level, chick type/level/frequency
// cymbal = mode, root, octave, cents, bell frequency/Q, metal decay, pitch chick/decay, level, ADSR
// air = texture, attack, hold, decay, curve, HPF, LPF, width, level
// space = room size, decay, damping, choke, mix, reverse mode/mix, MIDI mode, master width, output gain
constexpr std::array<PresetDefinition, 48> kDefinitions {{
    { "01 Dawnline Offbeat", { 1, 8.0f, 6.0f, 0.04f, 0.26f, 0.58f, 0, 0.36f, 6400 }, { 1, 6, 6, 0, 2860, 3.1f, 218, 2.4f, 17, 0.74f, 0.28f, 118, 0.72f, 142 }, { 1, 1.3f, 2.5f, 278, -0.40f, 1440, 17600, 1.12f, 0.84f }, { 0, 0.06f, 0.12f, 0.13f }, { -15.0f, 3.0f, 4.5f, 72, 1.8f, 0.91f, -0.35f, 22, 1 }, { 0.48f, 430, 0.28f, 205, 0.25f, 0, 0.42f, 0, 1.18f, -0.5f } },
    { "02 Blueglass Horizon", { 2, 12.5f, 8.0f, 0.11f, 0.54f, 0.55f, 0, 0.32f, 7100 }, { 1, 2, 6, -4.5f, 3180, 3.8f, 286, 1.8f, 21, 0.69f, 1.2f, 155, 0.66f, 228 }, { 2, 4.2f, 5.0f, 365, 0.14f, 1720, 19200, 1.58f, 0.79f }, { 1, 0.11f, 0.25f, 0.18f }, { -17.5f, 3.8f, 5.8f, 94, 2.0f, 0.82f, -0.45f, 28, 1 }, { 0.70f, 685, 0.19f, 290, 0.31f, 0, 0.50f, 0, 1.55f, -1.0f } },
    { "03 Silver Transit", { 1, 5.5f, 4.0f, 0.02f, 0.12f, 0.72f, 2, 0.28f, 8800 }, { 0, 9, 5, 2.0f, 3650, 2.5f, 176, 4.2f, 11, 0.84f, 0.12f, 96, 0.79f, 116 }, { 0, 0.0f, 1.0f, 214, -0.58f, 2260, 16800, 0.88f, 0.91f }, { 3, 0.08f, 0.42f, 0.10f }, { -12.0f, 2.4f, 2.2f, 55, 2.4f, 0.94f, -0.25f, 16, 1 }, { 0.32f, 345, 0.37f, 145, 0.18f, 0, 0.30f, 0, 1.02f, 0.0f } },
    { "04 Euphoric Afterglow", { 2, 15.0f, 10.5f, 0.16f, 0.62f, 0.53f, 1, 0.40f, 5200 }, { 2, 4, 7, 0, 3920, 4.3f, 318, 5.6f, 25, 0.88f, 0.45f, 188, 0.81f, 265 }, { 1, 2.8f, 4.0f, 430, -0.16f, 1350, 19800, 1.68f, 0.88f }, { 3, 0.12f, 0.36f, 0.20f }, { -18.5f, 4.2f, 3.4f, 108, 2.6f, 0.88f, -0.40f, 26, 1 }, { 0.74f, 820, 0.18f, 310, 0.34f, 0, 0.56f, 0, 1.72f, -1.0f } },
    { "05 Quiet Beacon", { 1, 7.0f, 5.0f, 0.03f, 0.18f, 0.40f, 1, 0.24f, 5900 }, { 1, 0, 5, 0, 2450, 2.7f, 245, 1.2f, 19, 0.58f, 1.1f, 142, 0.61f, 192 }, { 1, 5.6f, 6.0f, 304, 0.20f, 1880, 15400, 1.04f, 0.60f }, { 0, 0.03f, -0.18f, 0.10f }, { -13.5f, 2.7f, 6.2f, 82, 1.2f, 0.73f, -0.30f, 24, 1 }, { 0.42f, 510, 0.35f, 225, 0.20f, 0, 0.36f, 0, 1.08f, -1.5f } },
    { "06 Cloud Atlas Swing", { 3, 18.0f, 13.0f, 0.28f, 0.86f, 0.50f, 0, 0.34f, 7600 }, { 1, 7, 6, 5.5f, 3320, 3.2f, 302, 3.2f, 18, 0.65f, 0.75f, 178, 0.76f, 248 }, { 2, 6.0f, 7.0f, 420, 0.30f, 1580, 20000, 1.82f, 0.82f }, { 1, 0.10f, 0.12f, 0.16f }, { -16.0f, 3.2f, 6.5f, 118, 1.8f, 0.84f, -0.50f, 30, 1 }, { 0.82f, 740, 0.16f, 340, 0.35f, 0, 0.48f, 1, 1.86f, -1.5f } },
    { "07 Sunrise Pulse", { 2, 10.0f, 7.0f, 0.08f, 0.42f, 0.62f, 2, 0.45f, 9600 }, { 0, 2, 6, -2.0f, 4280, 4.0f, 206, 6.8f, 13, 0.83f, 0.22f, 124, 0.73f, 174 }, { 0, 0.5f, 2.0f, 250, -0.24f, 1980, 18500, 1.32f, 0.91f }, { 3, 0.16f, 0.44f, 0.22f }, { -17.0f, 3.4f, 3.0f, 78, 2.7f, 0.92f, -0.40f, 20, 1 }, { 0.58f, 530, 0.26f, 215, 0.28f, 0, 0.43f, 0, 1.46f, -0.5f } },
    { "08 Velvet Skyline", { 1, 22.0f, 16.0f, 0.21f, 0.76f, 0.46f, 1, 0.31f, 4800 }, { 1, 10, 5, -7.0f, 2260, 2.1f, 350, 0.8f, 28, 0.55f, 2.2f, 228, 0.59f, 325 }, { 2, 8.0f, 8.0f, 530, 0.42f, 1140, 14200, 1.64f, 0.72f }, { 0, 0.07f, -0.30f, 0.14f }, { -11.0f, 2.1f, 8.0f, 145, 1.0f, 0.68f, -0.55f, 38, 1 }, { 0.95f, 980, 0.12f, 430, 0.39f, 0, 0.52f, 0, 1.91f, -2.0f } },

    { "09 Crystal Lift 138", { 1, 8.5f, 6.2f, 0.05f, 0.30f, 0.61f, 2, 0.48f, 10400 }, { 2, 4, 7, 0, 4520, 4.8f, 262, 7.8f, 12, 0.93f, 0.18f, 138, 0.78f, 178 }, { 3, 1.0f, 2.2f, 288, -0.30f, 2080, 19800, 1.48f, 0.92f }, { 3, 0.18f, 0.46f, 0.24f }, { -19.0f, 4.4f, 2.4f, 74, 3.2f, 0.92f, -0.32f, 18, 1 }, { 0.62f, 610, 0.22f, 180, 0.29f, 0, 0.40f, 0, 1.58f, -0.5f } },
    { "10 Sapphire Bellline", { 2, 20.0f, 12.0f, 0.12f, 0.66f, 0.48f, 0, 0.30f, 7600 }, { 2, 11, 6, 4.0f, 5250, 6.2f, 335, 9.5f, 22, 0.97f, 0.65f, 204, 0.82f, 292 }, { 1, 2.4f, 4.4f, 386, -0.04f, 1680, 19400, 1.56f, 0.83f }, { 1, 0.14f, 0.24f, 0.20f }, { -18.0f, 4.0f, 3.1f, 102, 2.4f, 0.86f, -0.38f, 24, 1 }, { 0.78f, 890, 0.14f, 365, 0.36f, 0, 0.48f, 1, 1.72f, -1.0f } },
    { "11 Aurora Step", { 2, 12.0f, 7.5f, 0.09f, 0.44f, 0.64f, 2, 0.42f, 11200 }, { 2, 1, 7, -1.5f, 3980, 3.7f, 228, 5.8f, 10, 0.89f, 0.10f, 112, 0.75f, 155 }, { 0, 0.4f, 1.8f, 226, -0.46f, 2460, 20000, 1.26f, 0.93f }, { 3, 0.21f, 0.52f, 0.25f }, { -16.5f, 3.2f, 2.1f, 68, 2.8f, 0.95f, -0.28f, 16, 1 }, { 0.57f, 520, 0.24f, 175, 0.26f, 0, 0.42f, 0, 1.40f, 0.0f } },
    { "12 Open Gate Euphoria", { 3, 21.0f, 17.0f, 0.24f, 0.82f, 0.45f, 1, 0.35f, 6100 }, { 2, 7, 7, 0, 4800, 5.6f, 412, 8.2f, 31, 0.86f, 1.0f, 260, 0.72f, 372 }, { 2, 5.5f, 7.8f, 590, 0.34f, 1440, 18900, 1.80f, 0.85f }, { 3, 0.11f, 0.30f, 0.19f }, { -17.5f, 3.7f, 4.5f, 136, 2.0f, 0.80f, -0.44f, 32, 1 }, { 0.92f, 1250, 0.12f, 560, 0.42f, 0, 0.56f, 0, 1.95f, -1.0f } },
    { "13 Lucent Rise", { 2, 9.5f, 5.8f, 0.06f, 0.35f, 0.68f, 0, 0.50f, 9200 }, { 2, 6, 7, 2.5f, 4300, 4.6f, 246, 6.4f, 15, 0.95f, 0.24f, 128, 0.86f, 194 }, { 3, 0.8f, 2.4f, 302, -0.26f, 2160, 20000, 1.62f, 0.95f }, { 3, 0.19f, 0.40f, 0.26f }, { -20.0f, 4.8f, 2.4f, 78, 3.0f, 0.93f, -0.30f, 18, 1 }, { 0.66f, 760, 0.20f, 210, 0.31f, 0, 0.46f, 0, 1.70f, -0.5f } },
    { "14 Starlight Division", { 2, 16.0f, 11.0f, 0.15f, 0.72f, 0.56f, 1, 0.38f, 6900 }, { 1, 9, 6, -3.0f, 3540, 3.9f, 298, 3.4f, 24, 0.72f, 0.90f, 172, 0.75f, 250 }, { 2, 3.6f, 4.8f, 412, 0.18f, 1620, 19500, 1.72f, 0.84f }, { 1, 0.09f, 0.16f, 0.17f }, { -16.5f, 3.5f, 5.0f, 112, 1.8f, 0.82f, -0.48f, 29, 1 }, { 0.81f, 930, 0.15f, 390, 0.37f, 0, 0.50f, 0, 1.82f, -1.5f } },
    { "15 Halo Spread", { 4, 28.0f, 21.0f, 0.32f, 0.96f, 0.43f, 0, 0.34f, 8400 }, { 1, 3, 6, 6.0f, 3100, 2.8f, 372, 2.2f, 34, 0.63f, 1.6f, 218, 0.70f, 335 }, { 2, 7.2f, 9.0f, 508, 0.48f, 1380, 20000, 1.98f, 0.77f }, { 0, 0.04f, -0.22f, 0.12f }, { -13.0f, 2.6f, 7.0f, 152, 1.2f, 0.70f, -0.54f, 42, 1 }, { 0.98f, 1180, 0.10f, 510, 0.44f, 0, 0.58f, 0, 2.0f, -2.0f } },
    { "16 Mainstage Daybreak", { 2, 10.5f, 7.1f, 0.08f, 0.48f, 0.72f, 2, 0.52f, 12100 }, { 2, 5, 7, 0.5f, 4680, 5.1f, 282, 8.8f, 16, 0.98f, 0.15f, 146, 0.84f, 215 }, { 0, 0.5f, 1.5f, 246, -0.34f, 2210, 19900, 1.52f, 0.96f }, { 3, 0.24f, 0.54f, 0.28f }, { -19.5f, 4.7f, 2.0f, 76, 3.1f, 0.94f, -0.26f, 17, 1 }, { 0.68f, 720, 0.18f, 195, 0.32f, 0, 0.48f, 0, 1.78f, -0.5f } },

    { "17 Dutch Signal 1998", { 1, 8.0f, 5.0f, 0.03f, 0.22f, 0.70f, 0, 0.46f, 6900 }, { 0, 6, 6, 0, 3120, 2.6f, 226, 3.0f, 13, 0.86f, 0.15f, 122, 0.76f, 158 }, { 3, 0.4f, 2.0f, 255, -0.50f, 1850, 17800, 1.08f, 0.91f }, { 2, 0.15f, 0.12f, 0.19f }, { -15.0f, 3.0f, 3.0f, 65, 2.4f, 0.94f, -0.34f, 18, 1 }, { 0.48f, 470, 0.30f, 195, 0.23f, 0, 0.38f, 0, 1.22f, 0.0f } },
    { "18 Four on the Horizon", { 2, 13.0f, 8.0f, 0.10f, 0.42f, 0.66f, 1, 0.39f, 5700 }, { 0, 0, 6, -1.0f, 3440, 3.3f, 248, 4.0f, 17, 0.81f, 0.30f, 146, 0.78f, 204 }, { 1, 1.8f, 3.5f, 318, -0.22f, 1570, 18300, 1.24f, 0.86f }, { 0, 0.08f, 0.08f, 0.15f }, { -15.5f, 3.1f, 4.0f, 76, 2.2f, 0.89f, -0.36f, 20, 1 }, { 0.55f, 565, 0.27f, 220, 0.25f, 0, 0.42f, 0, 1.35f, -0.5f } },
    { "19 Silver Kicker Hat", { 1, 4.5f, 3.0f, 0.01f, 0.12f, 0.78f, 0, 0.32f, 8100 }, { 3, 6, 5, 0, 4100, 3.0f, 152, 2.5f, 9, 0.76f, 0.08f, 82, 0.66f, 94 }, { 0, 0.0f, 0.8f, 186, -0.62f, 2380, 16000, 0.86f, 0.88f }, { 2, 0.20f, 0.25f, 0.16f }, { -11.0f, 2.3f, 1.4f, 46, 2.8f, 0.98f, -0.18f, 12, 1 }, { 0.28f, 270, 0.42f, 125, 0.16f, 0, 0.26f, 0, 0.96f, 0.0f } },
    { "20 Intercity Resonance", { 1, 10.0f, 6.0f, 0.05f, 0.33f, 0.60f, 2, 0.35f, 10100 }, { 0, 1, 6, 3.5f, 2760, 5.2f, 198, 7.0f, 8, 0.90f, 0.36f, 132, 0.82f, 166 }, { 3, 0.6f, 1.8f, 238, -0.38f, 1840, 18800, 1.22f, 0.89f }, { 3, 0.13f, 0.40f, 0.20f }, { -18.0f, 4.0f, 2.6f, 62, 3.0f, 0.93f, -0.32f, 16, 1 }, { 0.46f, 405, 0.33f, 165, 0.24f, 0, 0.41f, 0, 1.31f, -0.5f } },
    { "21 Clubland Memory", { 2, 17.0f, 12.0f, 0.17f, 0.60f, 0.58f, 1, 0.42f, 6200 }, { 0, 9, 5, -4.0f, 3550, 2.4f, 292, 1.0f, 23, 0.78f, 0.70f, 185, 0.68f, 255 }, { 1, 3.0f, 4.3f, 365, -0.05f, 1430, 17000, 1.38f, 0.80f }, { 0, 0.10f, -0.12f, 0.17f }, { -14.5f, 2.9f, 4.7f, 100, 2.0f, 0.84f, -0.42f, 25, 1 }, { 0.64f, 690, 0.21f, 280, 0.32f, 0, 0.48f, 0, 1.50f, -1.0f } },
    { "22 Amber Overdrive", { 3, 19.0f, 14.0f, 0.20f, 0.70f, 0.55f, 1, 0.50f, 7600 }, { 3, 4, 6, 1.0f, 2380, 4.6f, 224, 6.0f, 14, 0.83f, 0.20f, 138, 0.80f, 188 }, { 3, 1.2f, 3.2f, 298, -0.24f, 1250, 15300, 1.40f, 0.88f }, { 2, 0.33f, 0.18f, 0.27f }, { -20.0f, 4.8f, 1.8f, 72, 3.6f, 0.91f, -0.32f, 18, 1 }, { 0.50f, 520, 0.28f, 200, 0.29f, 0, 0.44f, 0, 1.48f, -0.5f } },
    { "23 Festival Dialtone", { 2, 11.0f, 7.5f, 0.10f, 0.50f, 0.74f, 2, 0.56f, 11800 }, { 2, 8, 6, 0, 4920, 5.0f, 214, 8.5f, 12, 0.92f, 0.14f, 118, 0.88f, 164 }, { 0, 0.2f, 1.4f, 220, -0.42f, 2370, 20000, 1.54f, 0.94f }, { 3, 0.27f, 0.58f, 0.31f }, { -20.5f, 5.0f, 1.8f, 68, 3.4f, 0.94f, -0.24f, 16, 1 }, { 0.62f, 495, 0.25f, 170, 0.30f, 0, 0.46f, 0, 1.68f, 0.0f } },
    { "24 Midnight Warehouse", { 1, 2.0f, 3.5f, 0.0f, 0.08f, 0.48f, 1, 0.30f, 4700 }, { 0, 3, 5, -5.5f, 1940, 3.8f, 210, -2.0f, 18, 0.72f, 0.15f, 115, 0.62f, 145 }, { 3, 0.0f, 0.0f, 208, -0.68f, 2860, 10800, 0.72f, 0.66f }, { 2, 0.24f, -0.38f, 0.22f }, { -16.0f, 3.4f, 2.0f, 52, 2.6f, 0.88f, -0.45f, 14, 1 }, { 0.38f, 300, 0.46f, 115, 0.18f, 0, 0.32f, 0, 0.82f, -1.5f } },

    { "25 Gridrunner Snap", { 1, 5.0f, 3.5f, 0.01f, 0.14f, 0.80f, 2, 0.62f, 11800 }, { 3, 2, 6, 0, 3550, 5.2f, 164, 11.0f, 7, 0.78f, 0.08f, 83, 0.70f, 105 }, { 0, 0.0f, 0.7f, 195, -0.66f, 2840, 18500, 0.92f, 0.96f }, { 2, 0.36f, 0.34f, 0.25f }, { -21.0f, 5.4f, 1.2f, 48, 4.0f, 0.96f, -0.22f, 12, 1 }, { 0.30f, 280, 0.44f, 90, 0.14f, 0, 0.28f, 0, 1.04f, 0.0f } },
    { "26 Voltage Triplet", { 3, 24.0f, 18.0f, 0.30f, 0.90f, 0.50f, 2, 0.52f, 13400 }, { 3, 9, 6, 2.0f, 4560, 6.4f, 244, 14.0f, 11, 0.90f, 0.32f, 132, 0.70f, 186 }, { 3, 0.5f, 2.0f, 234, -0.72f, 3380, 19600, 1.78f, 0.92f }, { 2, 0.42f, 0.48f, 0.31f }, { -22.0f, 6.0f, 0.9f, 44, 4.8f, 0.93f, -0.20f, 10, 1 }, { 0.54f, 405, 0.34f, 150, 0.22f, 0, 0.38f, 0, 1.68f, -0.5f } },
    { "27 Circuit Halo", { 2, 9.0f, 5.5f, 0.04f, 0.30f, 0.70f, 0, 0.42f, 9200 }, { 1, 5, 6, 0, 3180, 4.7f, 196, 3.5f, 10, 0.82f, 0.12f, 103, 0.72f, 125 }, { 2, 1.2f, 2.4f, 257, -0.42f, 2720, 18400, 1.36f, 0.90f }, { 3, 0.19f, 0.62f, 0.24f }, { -18.0f, 4.0f, 2.4f, 58, 3.2f, 0.91f, -0.30f, 16, 1 }, { 0.48f, 435, 0.34f, 170, 0.25f, 0, 0.40f, 0, 1.42f, -0.5f } },
    { "28 Pulse Climber", { 2, 16.0f, 10.0f, 0.14f, 0.64f, 0.66f, 1, 0.46f, 8200 }, { 2, 7, 7, 0, 5200, 5.7f, 280, 10.0f, 15, 0.96f, 0.22f, 155, 0.76f, 208 }, { 0, 0.6f, 1.8f, 314, -0.20f, 2100, 19800, 1.48f, 0.93f }, { 3, 0.28f, 0.54f, 0.30f }, { -20.0f, 4.8f, 1.7f, 70, 3.5f, 0.92f, -0.26f, 14, 1 }, { 0.60f, 535, 0.26f, 195, 0.30f, 0, 0.48f, 0, 1.60f, -0.5f } },
    { "29 Hardline Sizzle", { 1, 3.0f, 2.5f, 0.0f, 0.06f, 0.88f, 2, 0.70f, 13900 }, { 3, 1, 6, -2.0f, 5800, 7.8f, 128, 17.0f, 6, 0.72f, 0.05f, 72, 0.64f, 88 }, { 3, 0.0f, 0.5f, 168, -0.82f, 3560, 20000, 1.10f, 0.99f }, { 2, 0.52f, 0.38f, 0.29f }, { -23.0f, 6.5f, 0.6f, 38, 5.2f, 0.96f, -0.18f, 9, 1 }, { 0.33f, 240, 0.48f, 85, 0.12f, 0, 0.28f, 0, 1.10f, 0.0f } },
    { "30 Chrome Sawtooth", { 2, 11.0f, 6.0f, 0.05f, 0.36f, 0.77f, 2, 0.58f, 10200 }, { 3, 8, 6, 3.0f, 6120, 6.0f, 188, 13.0f, 8, 0.86f, 0.10f, 108, 0.71f, 132 }, { 0, 0.2f, 0.8f, 205, -0.60f, 3150, 20000, 1.38f, 0.98f }, { 2, 0.47f, 0.30f, 0.32f }, { -22.5f, 6.1f, 0.8f, 42, 4.8f, 0.94f, -0.20f, 10, 1 }, { 0.43f, 360, 0.39f, 132, 0.19f, 0, 0.34f, 0, 1.52f, -0.5f } },
    { "31 Nocturne 140", { 2, 20.0f, 15.0f, 0.26f, 0.82f, 0.52f, 1, 0.38f, 6000 }, { 1, 10, 6, -6.0f, 2280, 2.4f, 316, 2.2f, 25, 0.67f, 0.55f, 188, 0.65f, 282 }, { 2, 4.8f, 6.0f, 452, 0.22f, 1660, 17200, 1.52f, 0.80f }, { 1, 0.12f, -0.20f, 0.19f }, { -15.0f, 3.0f, 5.0f, 105, 1.7f, 0.82f, -0.46f, 26, 1 }, { 0.74f, 760, 0.17f, 330, 0.35f, 0, 0.50f, 0, 1.64f, -1.0f } },
    { "32 Strobe Relay", { 1, 6.5f, 4.0f, 0.02f, 0.20f, 0.84f, 2, 0.65f, 12600 }, { 3, 6, 6, 1.0f, 4980, 6.8f, 150, 16.0f, 5, 0.82f, 0.06f, 75, 0.69f, 94 }, { 0, 0.0f, 0.4f, 174, -0.78f, 3280, 19800, 1.06f, 0.98f }, { 3, 0.34f, 0.68f, 0.28f }, { -22.0f, 6.2f, 0.7f, 39, 4.9f, 0.97f, -0.18f, 8, 1 }, { 0.30f, 255, 0.47f, 95, 0.15f, 0, 0.31f, 0, 1.22f, 0.0f } },

    { "33 Psychedelic Comet", { 3, 15.0f, 10.0f, 0.12f, 0.50f, 0.61f, 2, 0.64f, 13000 }, { 2, 8, 7, 0, 3820, 5.2f, 214, 12.0f, 9, 0.93f, 0.16f, 112, 0.82f, 160 }, { 2, 1.4f, 2.8f, 280, -0.38f, 2600, 19700, 1.46f, 0.94f }, { 3, 0.30f, 0.72f, 0.32f }, { -20.0f, 5.0f, 1.3f, 54, 3.5f, 0.91f, -0.24f, 12, 1 }, { 0.55f, 480, 0.29f, 178, 0.25f, 3, 0.60f, 0, 1.60f, -0.5f } },
    { "34 Fractal Shimmer", { 2, 26.0f, 19.0f, 0.35f, 0.98f, 0.46f, 0, 0.35f, 7800 }, { 1, 3, 6, 7.0f, 2940, 3.5f, 388, 3.0f, 37, 0.62f, 1.8f, 244, 0.67f, 365 }, { 2, 7.0f, 8.5f, 580, 0.58f, 1280, 18600, 1.92f, 0.78f }, { 0, 0.05f, -0.32f, 0.14f }, { -13.0f, 2.6f, 7.0f, 165, 1.2f, 0.72f, -0.52f, 38, 1 }, { 0.98f, 1390, 0.09f, 520, 0.44f, 0, 0.54f, 0, 2.0f, -1.5f } },
    { "35 Forest Gate", { 1, 7.0f, 4.5f, 0.03f, 0.15f, 0.45f, 1, 0.30f, 4900 }, { 3, 2, 5, -3.0f, 1820, 4.0f, 236, 5.0f, 16, 0.70f, 0.20f, 142, 0.55f, 182 }, { 3, 0.0f, 1.0f, 232, -0.75f, 2940, 11200, 0.84f, 0.70f }, { 2, 0.28f, -0.42f, 0.21f }, { -17.0f, 3.6f, 2.3f, 61, 2.8f, 0.86f, -0.42f, 12, 1 }, { 0.36f, 330, 0.45f, 125, 0.20f, 0, 0.34f, 0, 0.94f, -1.0f } },
    { "36 Spiral Bloom", { 4, 32.0f, 24.0f, 0.42f, 1.0f, 0.49f, 0, 0.38f, 6900 }, { 1, 11, 6, -5.0f, 3260, 3.6f, 440, 2.2f, 42, 0.60f, 2.6f, 296, 0.64f, 420 }, { 2, 9.5f, 10.0f, 660, 0.64f, 1320, 18000, 2.0f, 0.75f }, { 1, 0.13f, -0.28f, 0.20f }, { -12.5f, 2.5f, 8.5f, 190, 1.1f, 0.70f, -0.58f, 45, 1 }, { 1.0f, 1560, 0.08f, 590, 0.47f, 0, 0.58f, 0, 2.0f, -2.0f } },
    { "37 Orbital Rattle", { 5, 13.0f, 9.0f, 0.22f, 0.94f, 0.57f, 2, 0.56f, 12300 }, { 3, 7, 6, 0, 4250, 6.7f, 205, 15.0f, 8, 0.88f, 0.14f, 122, 0.65f, 178 }, { 3, 0.3f, 1.5f, 242, -0.68f, 3240, 19400, 1.74f, 0.94f }, { 2, 0.39f, 0.36f, 0.30f }, { -21.0f, 5.8f, 1.0f, 49, 4.4f, 0.93f, -0.22f, 11, 1 }, { 0.48f, 390, 0.36f, 155, 0.22f, 0, 0.42f, 0, 1.77f, -0.5f } },
    { "38 Deep Space Chirp", { 2, 18.0f, 11.5f, 0.18f, 0.72f, 0.53f, 2, 0.58f, 13700 }, { 2, 0, 7, 5.5f, 5680, 7.2f, 324, 18.0f, 19, 0.91f, 0.32f, 196, 0.73f, 286 }, { 1, 3.8f, 5.5f, 376, 0.12f, 1980, 18800, 1.54f, 0.85f }, { 3, 0.32f, 0.66f, 0.31f }, { -19.0f, 4.7f, 1.9f, 85, 3.4f, 0.88f, -0.36f, 21, 1 }, { 0.84f, 940, 0.15f, 370, 0.36f, 0, 0.49f, 1, 1.70f, -1.0f } },
    { "39 Acid Moonlight", { 1, 4.0f, 2.5f, 0.0f, 0.04f, 0.62f, 2, 0.68f, 14400 }, { 3, 5, 6, -1.0f, 6450, 8.0f, 142, 20.0f, 4, 0.80f, 0.05f, 65, 0.62f, 80 }, { 3, 0.0f, 0.4f, 158, -0.90f, 3820, 20000, 1.16f, 1.0f }, { 2, 0.50f, 0.48f, 0.30f }, { -24.0f, 7.0f, 0.5f, 34, 5.8f, 0.96f, -0.17f, 8, 1 }, { 0.30f, 245, 0.50f, 82, 0.12f, 0, 0.27f, 0, 1.15f, 0.0f } },
    { "40 Solar Flare Roll", { 3, 23.0f, 17.0f, 0.30f, 0.90f, 0.55f, 0, 0.44f, 11000 }, { 2, 4, 7, 2.0f, 4420, 4.4f, 294, 9.0f, 20, 0.92f, 0.24f, 166, 0.83f, 235 }, { 0, 1.0f, 2.3f, 342, -0.18f, 2020, 20000, 1.74f, 0.93f }, { 3, 0.22f, 0.61f, 0.27f }, { -18.5f, 4.4f, 2.1f, 72, 3.1f, 0.90f, -0.28f, 15, 1 }, { 0.72f, 670, 0.21f, 260, 0.33f, 0, 0.51f, 0, 1.86f, -0.5f } },

    { "41 Studio Air 01", { 1, 8.0f, 5.0f, 0.03f, 0.18f, 0.62f, 0, 0.32f, 6800 }, { 0, 6, 6, 0, 2800, 3.0f, 190, 3.5f, 14, 0.80f, 0.35f, 125, 0.70f, 180 }, { 0, 1.0f, 2.0f, 260, -0.35f, 1200, 16500, 1.20f, 0.88f }, { 0, 0.08f, 0.15f, 0.15f }, { -14.0f, 3.5f, 4.0f, 65, 2.0f, 1.0f, -0.3f, 20, 1 }, { 0.55f, 380, 0.25f, 180, 0.24f, 0, 0.5f, 0, 1.15f, 0.0f } },
    { "42 Dry Club Utility", { 1, 3.0f, 2.2f, 0.0f, 0.05f, 0.76f, 0, 0.30f, 7800 }, { 3, 6, 6, 0, 3600, 3.5f, 132, 2.0f, 8, 0.74f, 0.08f, 75, 0.64f, 92 }, { 0, 0.0f, 0.6f, 188, -0.64f, 2450, 17400, 0.92f, 0.84f }, { 2, 0.12f, 0.20f, 0.13f }, { -12.0f, 2.6f, 1.5f, 48, 2.5f, 0.96f, -0.15f, 12, 1 }, { 0.22f, 220, 0.50f, 95, 0.12f, 0, 0.22f, 0, 0.95f, 0.0f } },
    { "43 Wide Room Silver", { 3, 26.0f, 18.0f, 0.28f, 0.95f, 0.47f, 1, 0.33f, 5600 }, { 1, 2, 6, 4.0f, 3100, 3.8f, 335, 2.0f, 32, 0.62f, 1.6f, 228, 0.69f, 338 }, { 2, 4.0f, 6.0f, 480, 0.40f, 1450, 17600, 1.90f, 0.76f }, { 0, 0.06f, -0.35f, 0.13f }, { -12.5f, 2.5f, 7.5f, 158, 1.0f, 0.70f, -0.56f, 42, 1 }, { 1.0f, 1360, 0.10f, 545, 0.45f, 0, 0.57f, 0, 2.0f, -1.5f } },
    { "44 Tight Offbeat", { 1, 5.2f, 3.2f, 0.01f, 0.10f, 0.71f, 2, 0.46f, 10800 }, { 0, 9, 5, -1.5f, 3890, 3.2f, 158, 8.0f, 7, 0.82f, 0.12f, 85, 0.72f, 102 }, { 0, 0.0f, 0.4f, 200, -0.54f, 2280, 19000, 0.98f, 0.88f }, { 3, 0.14f, 0.58f, 0.18f }, { -17.0f, 3.8f, 1.2f, 46, 3.0f, 0.95f, -0.22f, 11, 1 }, { 0.30f, 295, 0.41f, 118, 0.17f, 0, 0.35f, 0, 1.13f, 0.0f } },
    { "45 Balearic Motion", { 2, 20.0f, 15.0f, 0.25f, 0.82f, 0.44f, 1, 0.26f, 5200 }, { 1, 9, 5, 2.5f, 2440, 2.0f, 330, 1.5f, 25, 0.56f, 2.0f, 210, 0.64f, 305 }, { 1, 6.5f, 7.0f, 510, 0.36f, 1220, 14800, 1.42f, 0.68f }, { 0, 0.04f, -0.28f, 0.10f }, { -10.0f, 2.0f, 8.0f, 174, 0.8f, 0.65f, -0.50f, 40, 1 }, { 0.88f, 1090, 0.12f, 470, 0.40f, 0, 0.48f, 0, 1.58f, -2.0f } },
    { "46 Satin Cymbal", { 1, 11.0f, 7.0f, 0.08f, 0.36f, 0.52f, 1, 0.27f, 5300 }, { 1, 4, 6, -2.0f, 2670, 2.3f, 280, 0.8f, 22, 0.60f, 0.85f, 172, 0.60f, 242 }, { 1, 3.2f, 4.5f, 350, 0.18f, 1520, 15800, 1.12f, 0.71f }, { 1, 0.07f, -0.22f, 0.14f }, { -12.0f, 2.3f, 5.0f, 110, 1.3f, 0.74f, -0.42f, 27, 1 }, { 0.62f, 710, 0.20f, 275, 0.30f, 0, 0.44f, 0, 1.24f, -1.0f } },
    { "47 Afterhours Brush", { 2, 16.0f, 12.0f, 0.18f, 0.71f, 0.38f, 1, 0.19f, 4500 }, { 1, 0, 5, -8.0f, 2160, 1.8f, 380, 0.4f, 32, 0.48f, 2.8f, 264, 0.52f, 390 }, { 2, 8.0f, 9.0f, 610, 0.52f, 980, 12800, 1.32f, 0.58f }, { 0, 0.02f, -0.50f, 0.08f }, { -9.0f, 1.8f, 9.0f, 205, 0.5f, 0.62f, -0.62f, 50, 1 }, { 0.96f, 1500, 0.08f, 600, 0.48f, 0, 0.46f, 0, 1.48f, -2.5f } },
    { "48 Neutral Silver Reference", { 1, 8.0f, 6.0f, 0.05f, 0.28f, 0.60f, 0, 0.40f, 7000 }, { 0, 6, 6, 0, 3000, 3.0f, 205, 3.0f, 14, 0.78f, 0.35f, 135, 0.72f, 190 }, { 0, 1.0f, 2.0f, 280, -0.35f, 1400, 18000, 1.15f, 0.86f }, { 0, 0.06f, 0.10f, 0.12f }, { -14.0f, 3.5f, 4.0f, 65, 2.0f, 0.92f, -0.3f, 20, 1 }, { 0.55f, 420, 0.25f, 180, 0.25f, 0, 0.45f, 0, 1.15f, 0.0f } }
}};

[[nodiscard]] FactoryPreset makePreset(const PresetDefinition& definition) {
    FactoryPreset preset { definition.name, {} };
    preset.values = {
        { kPreBursts, definition.articulation.bounceCount },
        { kPreSpread, definition.articulation.spreadMs },
        { kPreDecay, definition.articulation.decayMs },
        { kPreJitter, definition.articulation.humanize },
        { kFlamSpread, definition.articulation.pan },
        { kPreLevel, definition.articulation.bounceLevel },
        { kToneMode, definition.cymbal.mode },
        { kToneNote, definition.cymbal.note },
        { kToneOctave, definition.cymbal.octave },
        { kToneFine, definition.cymbal.fineCents },
        { kToneFreq, definition.cymbal.bellFrequency },
        { kToneQ, definition.cymbal.bellQ },
        { kToneDecay, definition.cymbal.metalDecay },
        { kTonePitchEnv, definition.cymbal.pitchChick },
        { kTonePitchDecay, definition.cymbal.pitchDecay },
        { kToneLevel, definition.cymbal.metalLevel },
        { kAmpAttack, definition.cymbal.attack },
        { kAmpDecay, definition.cymbal.decay },
        { kAmpSustain, definition.cymbal.sustain },
        { kAmpRelease, definition.cymbal.release },
        { kNoiseType, definition.air.texture },
        { kNoiseAttack, definition.air.attack },
        { kNoiseHold, definition.air.hold },
        { kNoiseDecay, definition.air.decay },
        { kNoiseCurve, definition.air.curve },
        { kNoiseHPF, definition.air.highpass },
        { kNoiseLPF, definition.air.lowpass },
        { kNoiseStereoWidth, definition.air.width },
        { kNoiseLevel, definition.air.level },
        { kClickType, definition.articulation.chickType },
        { kClickLevel, definition.articulation.chickLevel },
        { kClickFreq, definition.articulation.chickFrequency },
        { kDistType, definition.exciter.type },
        { kDistDrive, definition.exciter.drive },
        { kDistTone, definition.exciter.tilt },
        { kDistMix, definition.exciter.mix },
        { kCompThresh, definition.dynamics.threshold },
        { kCompRatio, definition.dynamics.ratio },
        { kCompAttack, definition.dynamics.attack },
        { kCompRelease, definition.dynamics.release },
        { kCompMakeup, definition.dynamics.makeup },
        { kCompMix, definition.dynamics.mix },
        { kLimiterCeiling, definition.dynamics.limiterCeiling },
        { kLimiterRelease, definition.dynamics.limiterRelease },
        { kLimiterEnabled, definition.dynamics.limiterEnabled },
        { kReverbSize, definition.space.size },
        { kReverbDecay, definition.space.decay },
        { kReverbDamp, definition.space.damping },
        { kReverbGate, definition.space.choke },
        { kReverbMix, definition.space.mix },
        { kReverseMode, definition.space.reverseMode },
        { kReverseMix, definition.space.reverseMix },
        { kMidiMode, definition.space.midiMode },
        { kStereoWidth, definition.space.masterWidth },
        { kOutputGain, definition.space.outputGain }
    };
    return preset;
}

[[nodiscard]] int sectionFor(const juce::String& id) {
    if (id == kPreBursts || id == kPreSpread || id == kPreDecay || id == kPreJitter
        || id == kFlamSpread || id == kPreLevel || id == kClickType || id == kClickLevel || id == kClickFreq) return 0;
    if (id == kToneMode || id == kToneNote || id == kToneOctave || id == kToneFine || id == kToneFreq
        || id == kToneQ || id == kToneDecay || id == kTonePitchEnv || id == kTonePitchDecay || id == kToneLevel
        || id == kAmpAttack || id == kAmpDecay || id == kAmpSustain || id == kAmpRelease || id == kLockPitch) return 1;
    if (id == kNoiseType || id == kNoiseAttack || id == kNoiseHold || id == kNoiseDecay || id == kNoiseCurve
        || id == kNoiseHPF || id == kNoiseLPF || id == kNoiseStereoWidth || id == kNoiseLevel) return 2;
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
        for (const auto& definition : kDefinitions)
            result.push_back(makePreset(definition));
        return result;
    }();
    return presets;
}

void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state,
           const std::array<bool, 6>& lockedSections, bool preservePitch) {
    for (const auto& [paramId, value] : preset.values) {
        if (lockedSections[static_cast<size_t>(sectionFor(paramId))]) continue;
        if (preservePitch && (paramId == kToneOctave || paramId == kToneFreq || paramId == kToneNote || paramId == kToneFine)) continue;
        if (auto* param = state.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

} // namespace openhatter::presets
