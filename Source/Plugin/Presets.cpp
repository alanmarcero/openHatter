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
    { "Progressive | Glassline", { 1, 7.0f, 4.5f, 0.04f, 0.20f, 0.62f, 0, 0.35f, 7200 }, { 1, 6, 6, 0, 2600, 2.8f, 172, 2.0f, 12, 0.70f, 0.25f, 72, 0.64f, 150 }, { 1, 1.0f, 2.0f, 235, -0.35f, 1300, 17500, 1.20f, 0.72f }, { 0, 0.04f, 0.05f, 0.10f }, { -15.0f, 3.0f, 4.0f, 70, 1.8f, 0.85f, -1.0f, 30, 1 }, { 0.38f, 380, 0.30f, 150, 0.18f, 0, 0.25f, 0, 1.10f, 0.0f } },
    { "Progressive | Night Current", { 2, 12.0f, 7.0f, 0.10f, 0.48f, 0.55f, 1, 0.32f, 6100 }, { 0, 6, 6, 0, 3150, 3.6f, 245, 4.0f, 18, 0.74f, 0.40f, 110, 0.70f, 205 }, { 2, 3.0f, 4.0f, 330, -0.10f, 1500, 16400, 1.45f, 0.68f }, { 1, 0.08f, -0.12f, 0.16f }, { -16.0f, 3.4f, 5.0f, 92, 2.0f, 0.82f, -1.5f, 36, 1 }, { 0.52f, 550, 0.24f, 220, 0.25f, 0, 0.32f, 0, 1.35f, -0.5f } },
    { "Progressive | Long Exposure", { 2, 18.0f, 12.0f, 0.16f, 0.68f, 0.48f, 0, 0.30f, 5600 }, { 1, 6, 6, 0, 3700, 2.4f, 310, 1.0f, 24, 0.60f, 0.80f, 145, 0.68f, 285 }, { 1, 5.0f, 5.0f, 420, 0.16f, 1150, 15000, 1.62f, 0.64f }, { 0, 0.03f, -0.25f, 0.12f }, { -13.0f, 2.5f, 6.5f, 120, 1.3f, 0.76f, -2.0f, 48, 1 }, { 0.76f, 780, 0.18f, 330, 0.31f, 0, 0.38f, 0, 1.52f, -1.0f } },
    { "Progressive | Paper Lantern", { 1, 9.0f, 6.0f, 0.05f, 0.32f, 0.50f, 1, 0.24f, 5000 }, { 1, 6, 6, 0, 2250, 2.1f, 205, -1.0f, 20, 0.58f, 0.55f, 105, 0.58f, 195 }, { 2, 4.5f, 3.5f, 295, 0.12f, 850, 11800, 1.10f, 0.57f }, { 0, 0.02f, -0.30f, 0.08f }, { -12.0f, 2.2f, 5.0f, 105, 1.0f, 0.70f, -2.0f, 42, 1 }, { 0.48f, 610, 0.32f, 280, 0.24f, 0, 0.20f, 0, 1.06f, -1.5f } },
    { "Progressive | Meridian", { 2, 14.0f, 9.0f, 0.10f, 0.52f, 0.58f, 2, 0.38f, 9400 }, { 2, 6, 6, 0, 4100, 4.2f, 230, 6.0f, 15, 0.80f, 0.25f, 100, 0.72f, 180 }, { 0, 1.0f, 1.5f, 260, -0.32f, 1800, 18300, 1.30f, 0.76f }, { 2, 0.13f, 0.18f, 0.19f }, { -18.0f, 3.6f, 3.0f, 68, 2.5f, 0.90f, -0.5f, 28, 1 }, { 0.45f, 460, 0.26f, 185, 0.20f, 0, 0.28f, 0, 1.24f, 0.0f } },
    { "Progressive | Soft Focus", { 3, 22.0f, 15.0f, 0.22f, 0.82f, 0.44f, 1, 0.28f, 4800 }, { 1, 6, 6, 0, 2850, 1.8f, 360, -2.0f, 30, 0.52f, 1.40f, 190, 0.60f, 340 }, { 2, 7.0f, 7.0f, 520, 0.28f, 1000, 13800, 1.78f, 0.58f }, { 0, 0.01f, -0.38f, 0.07f }, { -10.0f, 1.8f, 7.0f, 145, 0.8f, 0.68f, -3.0f, 55, 1 }, { 0.90f, 1050, 0.14f, 480, 0.36f, 0, 0.46f, 0, 1.78f, -2.0f } },
    { "Progressive | Copper Thread", { 1, 5.0f, 3.2f, 0.02f, 0.14f, 0.73f, 0, 0.40f, 7700 }, { 0, 6, 6, 0, 4450, 5.0f, 155, 7.0f, 9, 0.86f, 0.10f, 82, 0.76f, 120 }, { 0, 0.3f, 1.0f, 190, -0.55f, 2180, 19200, 1.18f, 0.85f }, { 2, 0.18f, 0.26f, 0.21f }, { -20.0f, 4.5f, 1.5f, 52, 3.2f, 0.95f, -0.5f, 22, 1 }, { 0.30f, 300, 0.40f, 130, 0.17f, 0, 0.26f, 0, 1.12f, 0.0f } },
    { "Progressive | Wide Stillness", { 2, 24.0f, 17.0f, 0.25f, 0.94f, 0.40f, 1, 0.24f, 5200 }, { 1, 6, 6, 0, 2400, 2.0f, 420, 0.0f, 35, 0.48f, 1.80f, 220, 0.58f, 420 }, { 2, 9.0f, 8.0f, 610, 0.40f, 900, 13200, 1.92f, 0.54f }, { 0, 0.02f, -0.42f, 0.06f }, { -9.0f, 1.6f, 8.0f, 170, 0.5f, 0.64f, -3.0f, 65, 1 }, { 0.98f, 1280, 0.10f, 570, 0.40f, 0, 0.50f, 0, 1.90f, -2.5f } },

    { "Uplifting | Skywall", { 1, 7.5f, 5.0f, 0.04f, 0.22f, 0.70f, 2, 0.46f, 10800 }, { 2, 6, 6, 0, 5000, 4.4f, 225, 8.0f, 13, 0.91f, 0.15f, 106, 0.78f, 165 }, { 0, 0.4f, 1.8f, 245, -0.32f, 2050, 19600, 1.42f, 0.92f }, { 3, 0.22f, 0.52f, 0.27f }, { -19.0f, 4.4f, 2.2f, 72, 3.0f, 0.92f, -0.5f, 20, 1 }, { 0.58f, 520, 0.24f, 195, 0.28f, 0, 0.38f, 0, 1.50f, 0.0f } },
    { "Uplifting | Radiant Spray", { 2, 13.0f, 8.0f, 0.09f, 0.48f, 0.64f, 2, 0.52f, 12000 }, { 2, 6, 6, 0, 5700, 6.0f, 270, 11.0f, 16, 0.95f, 0.12f, 120, 0.82f, 185 }, { 0, 0.6f, 1.5f, 290, -0.26f, 2350, 20000, 1.62f, 0.98f }, { 3, 0.29f, 0.62f, 0.32f }, { -21.0f, 5.0f, 1.8f, 64, 3.7f, 0.95f, -0.5f, 18, 1 }, { 0.64f, 570, 0.20f, 205, 0.30f, 0, 0.42f, 0, 1.62f, -0.5f } },
    { "Uplifting | Climb Marker", { 2, 10.0f, 7.0f, 0.07f, 0.42f, 0.68f, 0, 0.38f, 9100 }, { 2, 6, 7, 0, 4350, 3.8f, 245, 5.0f, 12, 0.84f, 0.20f, 112, 0.75f, 174 }, { 1, 1.2f, 2.0f, 270, -0.18f, 1720, 18600, 1.34f, 0.84f }, { 1, 0.11f, 0.24f, 0.18f }, { -17.0f, 3.4f, 2.8f, 72, 2.5f, 0.90f, -1.0f, 24, 1 }, { 0.50f, 480, 0.28f, 180, 0.25f, 0, 0.35f, 0, 1.36f, 0.0f } },
    { "Uplifting | Last Chorus", { 3, 17.0f, 12.0f, 0.18f, 0.72f, 0.55f, 1, 0.40f, 6500 }, { 2, 6, 6, 0, 3650, 3.2f, 345, 4.0f, 25, 0.76f, 0.75f, 170, 0.72f, 300 }, { 1, 3.5f, 4.0f, 410, 0.08f, 1380, 17700, 1.60f, 0.78f }, { 1, 0.08f, 0.10f, 0.15f }, { -15.0f, 2.8f, 4.6f, 110, 1.7f, 0.83f, -1.5f, 40, 1 }, { 0.78f, 860, 0.18f, 370, 0.34f, 0, 0.42f, 0, 1.70f, -1.0f } },
    { "Uplifting | High Window", { 1, 5.5f, 4.0f, 0.03f, 0.16f, 0.75f, 2, 0.48f, 13200 }, { 2, 6, 6, 0, 6250, 7.2f, 158, 14.0f, 8, 0.88f, 0.08f, 85, 0.74f, 130 }, { 0, 0.0f, 0.8f, 185, -0.58f, 2850, 20000, 1.12f, 0.96f }, { 3, 0.34f, 0.58f, 0.30f }, { -22.0f, 5.4f, 1.0f, 52, 4.2f, 0.96f, -0.5f, 18, 1 }, { 0.35f, 320, 0.38f, 140, 0.18f, 0, 0.30f, 0, 1.18f, 0.0f } },
    { "Uplifting | Blue Arpeggio", { 2, 11.0f, 7.0f, 0.08f, 0.46f, 0.62f, 0, 0.32f, 8200 }, { 1, 6, 6, 0, 3850, 4.8f, 282, 6.0f, 19, 0.72f, 0.36f, 140, 0.74f, 245 }, { 2, 2.4f, 3.5f, 355, 0.06f, 1550, 17400, 1.52f, 0.78f }, { 0, 0.05f, -0.18f, 0.13f }, { -15.5f, 3.0f, 4.2f, 95, 1.8f, 0.84f, -1.0f, 32, 1 }, { 0.68f, 700, 0.21f, 295, 0.31f, 0, 0.46f, 0, 1.56f, -0.5f } },
    { "Uplifting | Summit Bloom", { 4, 25.0f, 18.0f, 0.28f, 0.90f, 0.48f, 1, 0.34f, 5600 }, { 2, 6, 6, 0, 3300, 2.8f, 395, 3.0f, 30, 0.68f, 1.20f, 210, 0.70f, 390 }, { 2, 6.5f, 6.0f, 500, 0.24f, 1250, 16500, 1.88f, 0.68f }, { 1, 0.07f, -0.10f, 0.14f }, { -12.0f, 2.2f, 6.4f, 138, 1.2f, 0.73f, -2.0f, 55, 1 }, { 0.88f, 1080, 0.14f, 480, 0.39f, 0, 0.52f, 0, 1.86f, -1.5f } },
    { "Uplifting | Open Sky", { 1, 8.0f, 5.4f, 0.05f, 0.28f, 0.69f, 2, 0.44f, 9900 }, { 2, 6, 6, 0, 4700, 4.5f, 260, 7.0f, 14, 0.90f, 0.18f, 118, 0.80f, 185 }, { 0, 0.9f, 2.0f, 280, -0.24f, 1920, 19200, 1.46f, 0.90f }, { 3, 0.25f, 0.46f, 0.26f }, { -18.5f, 4.1f, 2.5f, 82, 2.8f, 0.91f, -0.5f, 24, 1 }, { 0.60f, 590, 0.22f, 220, 0.28f, 0, 0.40f, 0, 1.50f, -0.5f } },

    { "Classic | Tape Bronze", { 1, 6.0f, 4.5f, 0.02f, 0.18f, 0.70f, 0, 0.40f, 6800 }, { 0, 6, 6, 0, 2800, 3.0f, 190, 3.0f, 14, 0.82f, 0.22f, 105, 0.70f, 165 }, { 0, 0.3f, 1.2f, 220, -0.45f, 1450, 16500, 1.05f, 0.82f }, { 0, 0.06f, -0.10f, 0.15f }, { -14.0f, 3.0f, 3.0f, 70, 2.2f, 0.90f, -1.0f, 24, 1 }, { 0.42f, 420, 0.30f, 185, 0.22f, 0, 0.35f, 0, 1.08f, 0.0f } },
    { "Classic | Warm Press", { 1, 8.5f, 5.5f, 0.04f, 0.30f, 0.62f, 1, 0.32f, 5400 }, { 0, 6, 6, 0, 2150, 2.2f, 245, -1.0f, 20, 0.64f, 0.65f, 128, 0.66f, 215 }, { 1, 2.0f, 2.5f, 305, -0.12f, 1000, 14500, 1.10f, 0.64f }, { 0, 0.03f, -0.24f, 0.12f }, { -12.5f, 2.5f, 4.0f, 88, 1.5f, 0.82f, -1.5f, 34, 1 }, { 0.55f, 585, 0.30f, 245, 0.26f, 0, 0.28f, 0, 1.16f, -1.0f } },
    { "Classic | 909 Chamber", { 2, 11.0f, 7.0f, 0.08f, 0.42f, 0.66f, 0, 0.46f, 8200 }, { 0, 6, 6, 0, 3350, 4.0f, 225, 4.0f, 16, 0.88f, 0.18f, 112, 0.72f, 180 }, { 0, 0.2f, 1.0f, 260, -0.38f, 1850, 17800, 1.22f, 0.86f }, { 2, 0.14f, 0.08f, 0.20f }, { -17.0f, 3.8f, 2.3f, 74, 2.8f, 0.91f, -0.5f, 26, 1 }, { 0.48f, 490, 0.28f, 205, 0.25f, 0, 0.30f, 0, 1.28f, 0.0f } },
    { "Classic | Soft Machine", { 1, 14.0f, 10.0f, 0.10f, 0.55f, 0.52f, 1, 0.24f, 4700 }, { 1, 6, 6, 0, 2450, 1.9f, 315, -2.0f, 26, 0.55f, 1.10f, 155, 0.62f, 270 }, { 2, 4.0f, 3.5f, 390, 0.10f, 920, 12800, 1.30f, 0.55f }, { 0, 0.02f, -0.35f, 0.09f }, { -11.0f, 2.0f, 5.5f, 105, 1.0f, 0.70f, -2.0f, 44, 1 }, { 0.70f, 730, 0.22f, 320, 0.30f, 0, 0.35f, 0, 1.36f, -1.5f } },
    { "Classic | Polished Rim", { 1, 5.0f, 3.5f, 0.02f, 0.16f, 0.74f, 0, 0.36f, 9200 }, { 0, 6, 6, 0, 3950, 5.4f, 160, 8.0f, 10, 0.84f, 0.10f, 88, 0.73f, 130 }, { 0, 0.0f, 0.6f, 195, -0.60f, 2360, 18800, 1.08f, 0.90f }, { 2, 0.21f, 0.22f, 0.22f }, { -20.0f, 4.5f, 1.3f, 52, 3.2f, 0.95f, -0.5f, 18, 1 }, { 0.30f, 310, 0.38f, 135, 0.18f, 0, 0.28f, 0, 1.14f, 0.0f } },
    { "Classic | Backroom Velvet", { 2, 17.0f, 12.0f, 0.14f, 0.72f, 0.48f, 1, 0.28f, 5000 }, { 1, 6, 6, 0, 2050, 2.0f, 340, -3.0f, 28, 0.52f, 1.30f, 175, 0.60f, 315 }, { 1, 6.0f, 5.0f, 455, 0.18f, 850, 13200, 1.54f, 0.58f }, { 0, 0.02f, -0.40f, 0.07f }, { -10.0f, 1.9f, 6.0f, 125, 0.9f, 0.68f, -2.5f, 48, 1 }, { 0.84f, 920, 0.17f, 405, 0.34f, 0, 0.42f, 0, 1.70f, -2.0f } },
    { "Classic | Edge of Vinyl", { 1, 9.0f, 6.0f, 0.06f, 0.35f, 0.64f, 2, 0.34f, 7600 }, { 3, 6, 6, 0, 3600, 3.4f, 205, 2.0f, 17, 0.75f, 0.30f, 120, 0.68f, 190 }, { 3, 0.5f, 1.5f, 245, -0.42f, 2100, 16400, 1.16f, 0.76f }, { 2, 0.17f, 0.14f, 0.23f }, { -16.0f, 3.4f, 2.8f, 76, 2.7f, 0.90f, -1.0f, 28, 1 }, { 0.46f, 470, 0.29f, 200, 0.24f, 0, 0.32f, 0, 1.22f, -0.5f } },
    { "Classic | Warm Room", { 2, 12.0f, 8.0f, 0.08f, 0.46f, 0.58f, 0, 0.30f, 6100 }, { 0, 6, 6, 0, 2980, 3.2f, 275, 3.0f, 21, 0.70f, 0.50f, 138, 0.66f, 235 }, { 1, 2.8f, 3.0f, 335, 0.00f, 1250, 15200, 1.24f, 0.66f }, { 0, 0.03f, -0.22f, 0.11f }, { -13.0f, 2.6f, 4.5f, 98, 1.4f, 0.78f, -1.5f, 36, 1 }, { 0.62f, 680, 0.25f, 290, 0.28f, 0, 0.30f, 0, 1.32f, -1.0f } },

    { "Tech | Hard Relay", { 1, 3.5f, 2.6f, 0.00f, 0.08f, 0.82f, 2, 0.60f, 11800 }, { 3, 6, 6, 0, 5850, 7.0f, 118, 12.0f, 6, 0.78f, 0.06f, 68, 0.62f, 84 }, { 3, 0.0f, 0.4f, 145, -0.82f, 3500, 20000, 1.04f, 0.98f }, { 2, 0.46f, 0.42f, 0.30f }, { -23.0f, 6.0f, 0.8f, 42, 4.6f, 0.96f, -0.5f, 16, 1 }, { 0.25f, 245, 0.46f, 95, 0.12f, 0, 0.24f, 0, 1.05f, 0.0f } },
    { "Tech | Grid Slicer", { 1, 5.0f, 3.0f, 0.01f, 0.10f, 0.78f, 2, 0.54f, 13200 }, { 3, 6, 6, 0, 6350, 8.0f, 148, 18.0f, 5, 0.84f, 0.05f, 74, 0.66f, 92 }, { 0, 0.0f, 0.2f, 170, -0.76f, 3900, 20000, 1.10f, 1.0f }, { 2, 0.58f, 0.54f, 0.34f }, { -25.0f, 7.0f, 0.5f, 36, 5.0f, 0.98f, -0.5f, 14, 1 }, { 0.24f, 220, 0.44f, 82, 0.10f, 0, 0.26f, 0, 1.10f, 0.0f } },
    { "Tech | Steel Staccato", { 2, 8.0f, 4.5f, 0.03f, 0.28f, 0.74f, 2, 0.52f, 10600 }, { 3, 6, 6, 0, 5200, 6.0f, 178, 14.0f, 8, 0.86f, 0.08f, 88, 0.68f, 112 }, { 3, 0.3f, 0.5f, 192, -0.68f, 3220, 19600, 1.18f, 0.94f }, { 2, 0.40f, 0.28f, 0.27f }, { -22.0f, 5.2f, 1.2f, 50, 3.8f, 0.95f, -0.5f, 18, 1 }, { 0.34f, 295, 0.40f, 118, 0.15f, 0, 0.28f, 0, 1.16f, -0.5f } },
    { "Tech | Notch Driver", { 1, 2.8f, 2.1f, 0.00f, 0.04f, 0.88f, 0, 0.42f, 8700 }, { 0, 6, 6, 0, 4300, 4.0f, 128, 9.0f, 7, 0.74f, 0.04f, 62, 0.60f, 78 }, { 0, 0.0f, 0.0f, 138, -0.92f, 2750, 18800, 0.88f, 0.88f }, { 2, 0.32f, 0.20f, 0.22f }, { -20.0f, 4.4f, 0.8f, 40, 3.2f, 0.96f, -0.5f, 13, 1 }, { 0.20f, 220, 0.50f, 88, 0.12f, 0, 0.20f, 0, 0.98f, 0.0f } },
    { "Tech | Dry Circuit", { 1, 6.0f, 3.8f, 0.01f, 0.18f, 0.68f, 0, 0.30f, 7400 }, { 3, 6, 6, 0, 4700, 5.5f, 155, 10.0f, 9, 0.80f, 0.08f, 86, 0.65f, 110 }, { 3, 0.1f, 0.3f, 165, -0.70f, 3100, 18200, 0.96f, 0.82f }, { 2, 0.30f, 0.36f, 0.20f }, { -18.0f, 3.8f, 1.3f, 58, 3.0f, 0.94f, -0.5f, 16, 1 }, { 0.22f, 260, 0.48f, 100, 0.13f, 0, 0.22f, 0, 1.02f, 0.0f } },
    { "Tech | Torque Stack", { 3, 14.0f, 9.0f, 0.11f, 0.54f, 0.63f, 1, 0.48f, 9600 }, { 3, 6, 6, 0, 3900, 4.6f, 212, 6.0f, 13, 0.88f, 0.16f, 116, 0.70f, 168 }, { 0, 0.8f, 1.0f, 235, -0.38f, 2050, 19400, 1.36f, 0.90f }, { 2, 0.36f, 0.34f, 0.27f }, { -21.0f, 5.0f, 1.5f, 64, 3.5f, 0.93f, -0.5f, 20, 1 }, { 0.42f, 380, 0.35f, 150, 0.19f, 0, 0.31f, 0, 1.32f, -0.5f } },
    { "Tech | Voltage Bridge", { 2, 10.0f, 6.0f, 0.06f, 0.40f, 0.76f, 2, 0.50f, 12400 }, { 3, 6, 6, 0, 5550, 6.8f, 185, 16.0f, 7, 0.92f, 0.07f, 90, 0.72f, 120 }, { 3, 0.2f, 0.4f, 188, -0.66f, 3450, 20000, 1.30f, 0.96f }, { 2, 0.48f, 0.56f, 0.32f }, { -23.0f, 5.8f, 1.0f, 48, 4.4f, 0.96f, -0.5f, 16, 1 }, { 0.36f, 305, 0.42f, 125, 0.18f, 0, 0.35f, 0, 1.40f, 0.0f } },
    { "Tech | Small Machine", { 1, 7.0f, 4.2f, 0.02f, 0.22f, 0.70f, 1, 0.34f, 6500 }, { 0, 6, 6, 0, 3200, 3.5f, 188, 4.0f, 11, 0.76f, 0.12f, 100, 0.66f, 145 }, { 1, 1.0f, 1.2f, 210, -0.42f, 1650, 16000, 1.08f, 0.76f }, { 0, 0.05f, -0.16f, 0.12f }, { -16.0f, 3.1f, 2.0f, 65, 2.0f, 0.86f, -0.5f, 18, 1 }, { 0.30f, 350, 0.36f, 140, 0.18f, 0, 0.22f, 0, 1.10f, 0.0f } },

    { "Psy | Prism Orbit", { 3, 18.0f, 12.0f, 0.16f, 0.68f, 0.55f, 2, 0.52f, 11600 }, { 2, 6, 6, 0, 4300, 5.8f, 220, 15.0f, 10, 0.92f, 0.16f, 122, 0.78f, 178 }, { 2, 2.0f, 3.0f, 315, -0.20f, 1940, 19600, 1.60f, 0.91f }, { 3, 0.26f, 0.68f, 0.31f }, { -20.0f, 4.6f, 1.5f, 60, 3.8f, 0.92f, -0.5f, 18, 1 }, { 0.50f, 430, 0.28f, 175, 0.24f, 3, 0.54f, 0, 1.62f, -0.5f } },
    { "Psy | Fractal Dust", { 4, 27.0f, 19.0f, 0.32f, 0.98f, 0.48f, 1, 0.42f, 7600 }, { 1, 6, 6, 0, 2850, 3.0f, 360, 5.0f, 25, 0.66f, 1.70f, 205, 0.67f, 380 }, { 2, 6.8f, 7.0f, 515, 0.38f, 1250, 18200, 1.94f, 0.75f }, { 1, 0.10f, -0.24f, 0.17f }, { -12.0f, 2.4f, 7.5f, 148, 1.1f, 0.72f, -2.5f, 52, 1 }, { 0.92f, 1180, 0.12f, 510, 0.42f, 0, 0.58f, 0, 1.94f, -2.0f } },
    { "Psy | Jungle Glass", { 2, 13.0f, 8.0f, 0.09f, 0.46f, 0.62f, 0, 0.40f, 8900 }, { 2, 6, 6, 0, 5200, 7.0f, 194, 12.0f, 9, 0.90f, 0.12f, 100, 0.76f, 155 }, { 3, 0.2f, 1.0f, 215, -0.62f, 2850, 20000, 1.34f, 0.92f }, { 3, 0.30f, 0.58f, 0.29f }, { -21.0f, 5.0f, 1.4f, 55, 3.8f, 0.94f, -0.5f, 17, 1 }, { 0.44f, 380, 0.32f, 155, 0.22f, 0, 0.38f, 0, 1.48f, -0.5f } },
    { "Psy | Phase Bloom", { 3, 21.0f, 15.0f, 0.24f, 0.86f, 0.50f, 1, 0.36f, 6000 }, { 1, 6, 6, 0, 3450, 3.8f, 330, 3.0f, 22, 0.68f, 1.0f, 176, 0.70f, 320 }, { 2, 5.0f, 5.0f, 460, 0.28f, 1150, 15800, 1.82f, 0.72f }, { 0, 0.04f, -0.30f, 0.09f }, { -11.0f, 2.0f, 6.4f, 130, 0.9f, 0.68f, -2.5f, 48, 1 }, { 0.88f, 1050, 0.14f, 460, 0.39f, 0, 0.50f, 0, 1.84f, -1.5f } },
    { "Psy | Molecular", { 1, 6.0f, 3.8f, 0.02f, 0.18f, 0.72f, 2, 0.48f, 12500 }, { 3, 6, 6, 0, 6100, 7.5f, 152, 17.0f, 6, 0.84f, 0.06f, 76, 0.66f, 96 }, { 3, 0.0f, 0.4f, 170, -0.82f, 3800, 20000, 1.18f, 0.98f }, { 2, 0.52f, 0.44f, 0.32f }, { -24.0f, 6.5f, 0.7f, 40, 4.8f, 0.97f, -0.5f, 14, 1 }, { 0.30f, 255, 0.46f, 100, 0.12f, 0, 0.28f, 0, 1.20f, 0.0f } },
    { "Psy | Starfield", { 2, 15.0f, 10.0f, 0.13f, 0.60f, 0.56f, 0, 0.32f, 7500 }, { 2, 6, 6, 0, 3650, 4.2f, 275, 9.0f, 16, 0.82f, 0.28f, 135, 0.78f, 220 }, { 1, 2.5f, 3.2f, 370, 0.02f, 1480, 17200, 1.58f, 0.80f }, { 1, 0.10f, 0.16f, 0.16f }, { -16.0f, 3.1f, 3.8f, 88, 1.9f, 0.85f, -1.5f, 31, 1 }, { 0.70f, 720, 0.22f, 310, 0.32f, 2, 0.42f, 0, 1.68f, -1.0f } },
    { "Psy | Spiral Ratchet", { 5, 11.0f, 7.0f, 0.18f, 0.92f, 0.60f, 2, 0.55f, 13800 }, { 3, 6, 6, 0, 4550, 6.2f, 198, 15.0f, 8, 0.90f, 0.10f, 105, 0.72f, 160 }, { 3, 0.4f, 1.4f, 235, -0.66f, 3000, 19800, 1.76f, 0.95f }, { 2, 0.42f, 0.38f, 0.30f }, { -22.0f, 5.6f, 1.0f, 52, 4.0f, 0.94f, -0.5f, 17, 1 }, { 0.48f, 410, 0.30f, 165, 0.24f, 3, 0.48f, 0, 1.76f, -0.5f } },
    { "Psy | Deep Sun", { 2, 19.0f, 14.0f, 0.20f, 0.74f, 0.51f, 1, 0.34f, 5300 }, { 1, 6, 6, 0, 2350, 2.4f, 385, 2.0f, 31, 0.60f, 1.35f, 198, 0.64f, 360 }, { 2, 7.5f, 7.5f, 565, 0.42f, 1050, 14600, 1.68f, 0.62f }, { 0, 0.02f, -0.34f, 0.08f }, { -10.0f, 1.9f, 7.2f, 155, 0.8f, 0.66f, -3.0f, 54, 1 }, { 0.96f, 1240, 0.10f, 560, 0.43f, 0, 0.52f, 0, 1.88f, -2.0f } },

    { "Club | Neutral Open", { 1, 7.0f, 4.5f, 0.03f, 0.18f, 0.64f, 0, 0.36f, 7000 }, { 0, 6, 6, 0, 3000, 3.0f, 205, 3.0f, 14, 0.78f, 0.35f, 135, 0.72f, 190 }, { 0, 1.0f, 2.0f, 255, -0.35f, 1400, 18000, 1.15f, 0.82f }, { 0, 0.06f, 0.10f, 0.12f }, { -14.0f, 3.5f, 3.5f, 65, 2.0f, 0.92f, -0.5f, 20, 1 }, { 0.50f, 420, 0.25f, 180, 0.24f, 0, 0.40f, 0, 1.15f, 0.0f } },
    { "Club | Tight Eighths", { 1, 4.0f, 2.8f, 0.01f, 0.08f, 0.75f, 0, 0.28f, 8400 }, { 3, 6, 6, 0, 3850, 3.6f, 145, 4.0f, 8, 0.72f, 0.08f, 82, 0.66f, 105 }, { 0, 0.0f, 0.5f, 170, -0.62f, 2400, 17800, 0.92f, 0.80f }, { 2, 0.16f, 0.18f, 0.15f }, { -16.0f, 3.1f, 1.6f, 54, 2.6f, 0.95f, -0.5f, 16, 1 }, { 0.22f, 250, 0.46f, 100, 0.14f, 0, 0.24f, 0, 0.96f, 0.0f } },
    { "Club | Wide Hall", { 3, 22.0f, 15.0f, 0.23f, 0.90f, 0.46f, 1, 0.30f, 5000 }, { 1, 6, 6, 0, 2500, 2.1f, 330, -1.0f, 28, 0.58f, 1.4f, 178, 0.62f, 330 }, { 2, 6.0f, 6.0f, 470, 0.30f, 1000, 14200, 1.92f, 0.66f }, { 0, 0.03f, -0.30f, 0.08f }, { -10.0f, 1.9f, 6.5f, 135, 0.9f, 0.68f, -2.5f, 48, 1 }, { 0.92f, 1180, 0.12f, 520, 0.40f, 0, 0.52f, 0, 1.90f, -2.0f } },
    { "Club | Small Room", { 1, 8.0f, 5.0f, 0.04f, 0.28f, 0.58f, 1, 0.25f, 5200 }, { 0, 6, 6, 0, 2750, 2.7f, 210, 2.0f, 16, 0.67f, 0.35f, 115, 0.65f, 190 }, { 1, 2.0f, 2.2f, 250, -0.25f, 1280, 15400, 1.12f, 0.65f }, { 0, 0.04f, -0.20f, 0.11f }, { -13.0f, 2.6f, 3.5f, 82, 1.5f, 0.86f, -1.0f, 28, 1 }, { 0.34f, 460, 0.34f, 170, 0.20f, 0, 0.28f, 0, 1.12f, -0.5f } },
    { "Club | Felt Brush", { 2, 16.0f, 12.0f, 0.18f, 0.68f, 0.42f, 1, 0.18f, 4200 }, { 1, 6, 6, 0, 1800, 1.6f, 360, -3.0f, 32, 0.46f, 2.2f, 195, 0.58f, 380 }, { 2, 8.0f, 8.0f, 540, 0.46f, 780, 11200, 1.38f, 0.50f }, { 0, 0.01f, -0.45f, 0.06f }, { -8.0f, 1.6f, 8.0f, 180, 0.4f, 0.62f, -3.0f, 62, 1 }, { 0.94f, 1350, 0.08f, 580, 0.44f, 0, 0.46f, 0, 1.48f, -2.5f } },
    { "Club | Bright Cut", { 1, 5.0f, 3.0f, 0.01f, 0.12f, 0.80f, 2, 0.52f, 12500 }, { 3, 6, 6, 0, 5200, 6.0f, 155, 12.0f, 7, 0.82f, 0.05f, 80, 0.65f, 105 }, { 0, 0.0f, 0.5f, 160, -0.72f, 3200, 20000, 1.02f, 0.96f }, { 2, 0.38f, 0.32f, 0.24f }, { -22.0f, 5.4f, 0.8f, 44, 4.0f, 0.96f, -0.5f, 14, 1 }, { 0.28f, 235, 0.44f, 90, 0.14f, 0, 0.26f, 0, 1.08f, 0.0f } },
    { "Club | Subtle Air", { 1, 10.0f, 7.0f, 0.06f, 0.38f, 0.48f, 0, 0.24f, 5800 }, { 1, 6, 6, 0, 3300, 2.8f, 265, 1.0f, 22, 0.60f, 0.75f, 145, 0.68f, 240 }, { 1, 4.5f, 4.0f, 345, 0.10f, 1050, 14000, 1.18f, 0.48f }, { 0, 0.02f, -0.28f, 0.07f }, { -11.0f, 2.1f, 4.4f, 112, 0.9f, 0.70f, -1.5f, 38, 1 }, { 0.58f, 640, 0.26f, 250, 0.27f, 0, 0.34f, 0, 1.20f, -1.0f } },
    { "Club | Reference Plate", { 1, 7.0f, 4.8f, 0.03f, 0.20f, 0.62f, 0, 0.34f, 7300 }, { 0, 6, 7, 0, 3400, 3.4f, 215, 3.0f, 16, 0.80f, 0.28f, 125, 0.70f, 190 }, { 0, 0.8f, 1.8f, 240, -0.30f, 1550, 17400, 1.20f, 0.80f }, { 0, 0.06f, 0.08f, 0.12f }, { -14.0f, 3.2f, 3.2f, 70, 1.8f, 0.90f, -0.5f, 22, 1 }, { 0.44f, 430, 0.30f, 185, 0.23f, 0, 0.38f, 0, 1.18f, 0.0f } }
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
