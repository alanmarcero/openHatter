#include "Presets.h"
#include "Parameters.h"

namespace openhatter::presets {

using namespace openhatter::params;

const std::vector<FactoryPreset>& factoryPresets() {
    static const std::vector<FactoryPreset> kPresets = {
        // ====================================================================
        // 01 - 12: Progressive & Melodic Trance
        // ====================================================================
        {
            "01 Init Velvet Off-Beat Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 8.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.65f },
                { kToneMode, 1.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2900.0f }, { kToneQ, 3.2f }, { kToneDecay, 210.0f }, { kTonePitchEnv, 2.0f }, { kTonePitchDecay, 15.0f }, { kToneLevel, 0.76f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 2.5f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 270.0f }, { kNoiseCurve, -0.28f }, { kNoiseHPF, 1350.0f }, { kNoiseLPF, 18200.0f }, { kNoiseStereoWidth, 1.35f }, { kNoiseLevel, 0.88f },
                { kClickType, 0.0f }, { kClickLevel, 0.42f }, { kClickFreq, 7200.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.07f }, { kDistTone, 0.15f }, { kDistMix, 0.12f },
                { kCompThresh, -13.0f }, { kCompRatio, 3.2f }, { kCompAttack, 5.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 1.8f }, { kCompMix, 0.88f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.60f }, { kReverbDecay, 420.0f }, { kReverbDamp, 0.22f }, { kReverbGate, 190.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.25f }, { kOutputGain, 0.0f }
            }
        },
        {
            "02 Sun & Moon Silk 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 7.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.60f },
                { kToneMode, 1.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3200.0f }, { kToneQ, 3.0f }, { kToneDecay, 240.0f }, { kTonePitchEnv, 1.8f }, { kTonePitchDecay, 18.0f }, { kToneLevel, 0.78f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 3.5f }, { kNoiseHold, 4.0f }, { kNoiseDecay, 320.0f }, { kNoiseCurve, -0.24f }, { kNoiseHPF, 1450.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.45f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.36f }, { kClickFreq, 7500.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.05f }, { kDistTone, 0.20f }, { kDistMix, 0.10f },
                { kCompThresh, -12.0f }, { kCompRatio, 3.0f }, { kCompAttack, 6.0f }, { kCompRelease, 75.0f }, { kCompMakeup, 1.5f }, { kCompMix, 0.85f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.70f }, { kReverbDecay, 540.0f }, { kReverbDamp, 0.20f }, { kReverbGate, 220.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.32f }, { kOutputGain, 0.0f }
            }
        },
        {
            "03 Melodic Silk 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.65f },
                { kToneMode, 0.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3000.0f }, { kToneQ, 3.4f }, { kToneDecay, 195.0f }, { kTonePitchEnv, 3.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.82f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.2f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 260.0f }, { kNoiseCurve, -0.35f }, { kNoiseHPF, 1400.0f }, { kNoiseLPF, 18000.0f }, { kNoiseStereoWidth, 1.25f }, { kNoiseLevel, 0.88f },
                { kClickType, 0.0f }, { kClickLevel, 0.46f }, { kClickFreq, 7000.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.12f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.6f }, { kCompAttack, 4.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.54f }, { kReverbDecay, 380.0f }, { kReverbDamp, 0.26f }, { kReverbGate, 180.0f }, { kReverbMix, 0.22f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.20f }, { kOutputGain, 0.0f }
            }
        },
        {
            "04 Progressive Volume Classic",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 8.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.06f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.68f },
                { kToneMode, 1.0f }, { kToneNote, 5.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2850.0f }, { kToneQ, 3.2f }, { kToneDecay, 215.0f }, { kTonePitchEnv, 2.5f }, { kTonePitchDecay, 16.0f }, { kToneLevel, 0.80f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 1.8f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 290.0f }, { kNoiseCurve, -0.28f }, { kNoiseHPF, 1250.0f }, { kNoiseLPF, 18500.0f }, { kNoiseStereoWidth, 1.40f }, { kNoiseLevel, 0.88f },
                { kClickType, 0.0f }, { kClickLevel, 0.40f }, { kClickFreq, 6800.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.07f }, { kDistTone, 0.14f }, { kDistMix, 0.13f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.5f }, { kCompAttack, 5.0f }, { kCompRelease, 68.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.62f }, { kReverbDecay, 460.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 195.0f }, { kReverbMix, 0.26f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.26f }, { kOutputGain, 0.0f }
            }
        },
        {
            "05 Silky Shimmer Velvet Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.52f },
                { kToneMode, 1.0f }, { kToneNote, 11.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3450.0f }, { kToneQ, 2.8f }, { kToneDecay, 260.0f }, { kTonePitchEnv, 1.5f }, { kTonePitchDecay, 20.0f }, { kToneLevel, 0.72f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 4.5f }, { kNoiseHold, 4.0f }, { kNoiseDecay, 360.0f }, { kNoiseCurve, -0.20f }, { kNoiseHPF, 1650.0f }, { kNoiseLPF, 19500.0f }, { kNoiseStereoWidth, 1.55f }, { kNoiseLevel, 0.92f },
                { kClickType, 1.0f }, { kClickLevel, 0.30f }, { kClickFreq, 7600.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.05f }, { kDistTone, 0.22f }, { kDistMix, 0.10f },
                { kCompThresh, -12.0f }, { kCompRatio, 2.8f }, { kCompAttack, 7.5f }, { kCompRelease, 80.0f }, { kCompMakeup, 1.6f }, { kCompMix, 0.85f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.75f }, { kReverbDecay, 620.0f }, { kReverbDamp, 0.18f }, { kReverbGate, 240.0f }, { kReverbMix, 0.32f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.36f }, { kOutputGain, 0.0f }
            }
        },
        {
            "06 Tuned F# Open Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 7.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.65f },
                { kToneMode, 1.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3050.0f }, { kToneQ, 4.0f }, { kToneDecay, 220.0f }, { kTonePitchEnv, 2.8f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.85f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.5f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 275.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1350.0f }, { kNoiseLPF, 18200.0f }, { kNoiseStereoWidth, 1.30f }, { kNoiseLevel, 0.88f },
                { kClickType, 0.0f }, { kClickLevel, 0.45f }, { kClickFreq, 6900.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.15f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.6f }, { kCompAttack, 4.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.58f }, { kReverbDecay, 420.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 190.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.24f }, { kOutputGain, 0.0f }
            }
        },
        {
            "07 Deep Progressive Air",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 10.0f }, { kPreDecay, 6.5f }, { kPreJitter, 0.08f }, { kFlamSpread, 0.40f }, { kPreLevel, 0.60f },
                { kToneMode, 1.0f }, { kToneNote, 2.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2500.0f }, { kToneQ, 2.6f }, { kToneDecay, 200.0f }, { kTonePitchEnv, 1.8f }, { kTonePitchDecay, 16.0f }, { kToneLevel, 0.72f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 3.5f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 265.0f }, { kNoiseCurve, -0.26f }, { kNoiseHPF, 1150.0f }, { kNoiseLPF, 16800.0f }, { kNoiseStereoWidth, 1.35f }, { kNoiseLevel, 0.86f },
                { kClickType, 1.0f }, { kClickLevel, 0.32f }, { kClickFreq, 6000.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.06f }, { kDistTone, 0.08f }, { kDistMix, 0.12f },
                { kCompThresh, -13.0f }, { kCompRatio, 3.2f }, { kCompAttack, 6.0f }, { kCompRelease, 70.0f }, { kCompMakeup, 1.8f }, { kCompMix, 0.86f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.64f }, { kReverbDecay, 480.0f }, { kReverbDamp, 0.28f }, { kReverbGate, 210.0f }, { kReverbMix, 0.26f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.26f }, { kOutputGain, 0.0f }
            }
        },
        {
            "08 Stadium Big Trance Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 8.5f }, { kPreDecay, 5.5f }, { kPreJitter, 0.06f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.78f },
                { kToneMode, 0.0f }, { kToneNote, 7.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3300.0f }, { kToneQ, 3.8f }, { kToneDecay, 230.0f }, { kTonePitchEnv, 4.5f }, { kTonePitchDecay, 13.0f }, { kToneLevel, 0.88f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.6f }, { kNoiseHold, 4.0f }, { kNoiseDecay, 310.0f }, { kNoiseCurve, -0.28f }, { kNoiseHPF, 1300.0f }, { kNoiseLPF, 19000.0f }, { kNoiseStereoWidth, 1.45f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.54f }, { kClickFreq, 7600.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.14f }, { kDistTone, 0.20f }, { kDistMix, 0.18f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.5f }, { kCompAttack, 3.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 3.0f }, { kCompMix, 0.95f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.65f }, { kReverbDecay, 480.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 200.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.32f }, { kOutputGain, 0.5f }
            }
        },
        {
            "09 Uplifting Sheen Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 7.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.65f },
                { kToneMode, 2.0f }, { kToneNote, 4.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3600.0f }, { kToneQ, 3.8f }, { kToneDecay, 220.0f }, { kTonePitchEnv, 3.5f }, { kTonePitchDecay, 15.0f }, { kToneLevel, 0.84f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 1.8f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 300.0f }, { kNoiseCurve, -0.28f }, { kNoiseHPF, 1500.0f }, { kNoiseLPF, 19200.0f }, { kNoiseStereoWidth, 1.42f }, { kNoiseLevel, 0.92f },
                { kClickType, 0.0f }, { kClickLevel, 0.44f }, { kClickFreq, 7800.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.10f }, { kDistTone, 0.25f }, { kDistMix, 0.15f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.8f }, { kCompAttack, 4.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.2f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.60f }, { kReverbDecay, 450.0f }, { kReverbDamp, 0.22f }, { kReverbGate, 195.0f }, { kReverbMix, 0.25f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.28f }, { kOutputGain, 0.0f }
            }
        },
        {
            "10 Crisp Off-Beat Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.68f },
                { kToneMode, 0.0f }, { kToneNote, 0.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2900.0f }, { kToneQ, 3.6f }, { kToneDecay, 175.0f }, { kTonePitchEnv, 3.2f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.84f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 2.5f }, { kNoiseDecay, 235.0f }, { kNoiseCurve, -0.40f }, { kNoiseHPF, 1450.0f }, { kNoiseLPF, 18500.0f }, { kNoiseStereoWidth, 1.22f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.52f }, { kClickFreq, 7200.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.15f }, { kDistMix, 0.14f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 3.5f }, { kCompRelease, 55.0f }, { kCompMakeup, 2.4f }, { kCompMix, 0.92f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.50f }, { kReverbDecay, 340.0f }, { kReverbDamp, 0.28f }, { kReverbGate, 160.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.18f }, { kOutputGain, 0.0f }
            }
        },
        {
            "11 Club Pocket Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 9.5f }, { kPreDecay, 6.0f }, { kPreJitter, 0.08f }, { kFlamSpread, 0.40f }, { kPreLevel, 0.72f },
                { kToneMode, 3.0f }, { kToneNote, 10.0f }, { kToneOctave, 5.0f }, { kToneFine, 0.0f }, { kToneFreq, 2500.0f }, { kToneQ, 3.8f }, { kToneDecay, 190.0f }, { kTonePitchEnv, 5.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.85f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 4.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 260.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1100.0f }, { kNoiseLPF, 17200.0f }, { kNoiseStereoWidth, 1.30f }, { kNoiseLevel, 0.90f },
                { kClickType, 1.0f }, { kClickLevel, 0.46f }, { kClickFreq, 6200.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.12f }, { kDistTone, 0.08f }, { kDistMix, 0.18f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.2f }, { kCompAttack, 4.5f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.6f }, { kCompMix, 0.94f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.54f }, { kReverbDecay, 370.0f }, { kReverbDamp, 0.30f }, { kReverbGate, 165.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.22f }, { kOutputGain, 0.2f }
            }
        },
        {
            "12 Deep Warm Velvet Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 11.0f }, { kPreDecay, 7.0f }, { kPreJitter, 0.12f }, { kFlamSpread, 0.45f }, { kPreLevel, 0.58f },
                { kToneMode, 1.0f }, { kToneNote, 9.0f }, { kToneOctave, 5.0f }, { kToneFine, 0.0f }, { kToneFreq, 2200.0f }, { kToneQ, 2.4f }, { kToneDecay, 210.0f }, { kTonePitchEnv, 1.5f }, { kTonePitchDecay, 18.0f }, { kToneLevel, 0.70f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 3.8f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 285.0f }, { kNoiseCurve, -0.25f }, { kNoiseHPF, 1000.0f }, { kNoiseLPF, 15000.0f }, { kNoiseStereoWidth, 1.38f }, { kNoiseLevel, 0.84f },
                { kClickType, 1.0f }, { kClickLevel, 0.28f }, { kClickFreq, 5400.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.05f }, { kDistTone, -0.05f }, { kDistMix, 0.10f },
                { kCompThresh, -12.0f }, { kCompRatio, 2.8f }, { kCompAttack, 7.0f }, { kCompRelease, 80.0f }, { kCompMakeup, 1.5f }, { kCompMix, 0.82f },
                { kLimiterCeiling, -0.4f }, { kLimiterRelease, 25.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.66f }, { kReverbDecay, 510.0f }, { kReverbDamp, 0.35f }, { kReverbGate, 210.0f }, { kReverbMix, 0.27f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.28f }, { kOutputGain, -0.2f }
            }
        },

        // ====================================================================
        // 13 - 18: Emotional 138 Uplifting
        // ====================================================================
        {
            "13 Euphoria 138 Open Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.5f }, { kPreDecay, 4.5f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.70f },
                { kToneMode, 2.0f }, { kToneNote, 6.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4300.0f }, { kToneQ, 4.2f }, { kToneDecay, 220.0f }, { kTonePitchEnv, 4.5f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.88f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.0f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 290.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1700.0f }, { kNoiseLPF, 19600.0f }, { kNoiseStereoWidth, 1.45f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.52f }, { kClickFreq, 8400.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.11f }, { kDistTone, 0.28f }, { kDistMix, 0.16f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.2f }, { kCompAttack, 3.5f }, { kCompRelease, 60.0f }, { kCompMakeup, 2.6f }, { kCompMix, 0.95f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.65f }, { kReverbDecay, 490.0f }, { kReverbDamp, 0.18f }, { kReverbGate, 200.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.34f }, { kOutputGain, 0.3f }
            }
        },
        {
            "14 Always Alive Sparkle",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 7.0f }, { kPreDecay, 4.5f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.72f },
                { kToneMode, 2.0f }, { kToneNote, 9.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4600.0f }, { kToneQ, 4.6f }, { kToneDecay, 235.0f }, { kTonePitchEnv, 5.0f }, { kTonePitchDecay, 13.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 1.5f }, { kNoiseHold, 4.0f }, { kNoiseDecay, 315.0f }, { kNoiseCurve, -0.28f }, { kNoiseHPF, 1850.0f }, { kNoiseLPF, 19800.0f }, { kNoiseStereoWidth, 1.52f }, { kNoiseLevel, 0.95f },
                { kClickType, 2.0f }, { kClickLevel, 0.48f }, { kClickFreq, 8900.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.12f }, { kDistTone, 0.32f }, { kDistMix, 0.17f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 4.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.5f }, { kCompMix, 0.94f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.70f }, { kReverbDecay, 540.0f }, { kReverbDamp, 0.16f }, { kReverbGate, 210.0f }, { kReverbMix, 0.30f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.38f }, { kOutputGain, 0.3f }
            }
        },
        {
            "15 Emotional Breakdown Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 8.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.58f },
                { kToneMode, 1.0f }, { kToneNote, 1.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 3900.0f }, { kToneQ, 3.4f }, { kToneDecay, 270.0f }, { kTonePitchEnv, 2.5f }, { kTonePitchDecay, 18.0f }, { kToneLevel, 0.78f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 4.0f }, { kNoiseHold, 5.0f }, { kNoiseDecay, 390.0f }, { kNoiseCurve, -0.20f }, { kNoiseHPF, 1550.0f }, { kNoiseLPF, 19400.0f }, { kNoiseStereoWidth, 1.58f }, { kNoiseLevel, 0.92f },
                { kClickType, 0.0f }, { kClickLevel, 0.35f }, { kClickFreq, 7800.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.06f }, { kDistTone, 0.22f }, { kDistMix, 0.12f },
                { kCompThresh, -13.0f }, { kCompRatio, 3.0f }, { kCompAttack, 7.0f }, { kCompRelease, 85.0f }, { kCompMakeup, 1.8f }, { kCompMix, 0.86f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 24.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.80f }, { kReverbDecay, 700.0f }, { kReverbDamp, 0.18f }, { kReverbGate, 260.0f }, { kReverbMix, 0.35f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.5f }, { kStereoWidth, 1.42f }, { kOutputGain, 0.0f }
            }
        },
        {
            "16 Peak Time Dual Drive",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 7.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.78f },
                { kToneMode, 0.0f }, { kToneNote, 4.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3350.0f }, { kToneQ, 3.8f }, { kToneDecay, 205.0f }, { kTonePitchEnv, 4.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.88f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 275.0f }, { kNoiseCurve, -0.34f }, { kNoiseHPF, 1500.0f }, { kNoiseLPF, 19000.0f }, { kNoiseStereoWidth, 1.38f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.56f }, { kClickFreq, 8000.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.13f }, { kDistTone, 0.22f }, { kDistMix, 0.18f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.5f }, { kCompAttack, 3.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 2.8f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.64f }, { kReverbDecay, 460.0f }, { kReverbDamp, 0.22f }, { kReverbGate, 190.0f }, { kReverbMix, 0.26f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.30f }, { kOutputGain, 0.4f }
            }
        },
        {
            "17 Euphoric Tuned C#7",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.68f },
                { kToneMode, 2.0f }, { kToneNote, 1.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4500.0f }, { kToneQ, 5.2f }, { kToneDecay, 200.0f }, { kTonePitchEnv, 3.5f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.92f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 265.0f }, { kNoiseCurve, -0.36f }, { kNoiseHPF, 1800.0f }, { kNoiseLPF, 19700.0f }, { kNoiseStereoWidth, 1.40f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.50f }, { kClickFreq, 8600.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.09f }, { kDistTone, 0.26f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.8f }, { kCompAttack, 3.5f }, { kCompRelease, 60.0f }, { kCompMakeup, 2.2f }, { kCompMix, 0.92f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.60f }, { kReverbDecay, 430.0f }, { kReverbDamp, 0.20f }, { kReverbGate, 185.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.32f }, { kOutputGain, 0.2f }
            }
        },
        {
            "18 Nordic Uplifting Silver Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 7.5f }, { kPreDecay, 5.0f }, { kPreJitter, 0.06f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.72f },
                { kToneMode, 1.0f }, { kToneNote, 11.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 4000.0f }, { kToneQ, 3.8f }, { kToneDecay, 230.0f }, { kTonePitchEnv, 3.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.86f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 2.0f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 300.0f }, { kNoiseCurve, -0.30f }, { kNoiseHPF, 1650.0f }, { kNoiseLPF, 19600.0f }, { kNoiseStereoWidth, 1.48f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.46f }, { kClickFreq, 8200.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.24f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.6f }, { kCompAttack, 4.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.68f }, { kReverbDecay, 510.0f }, { kReverbDamp, 0.20f }, { kReverbGate, 210.0f }, { kReverbMix, 0.27f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.34f }, { kOutputGain, 0.2f }
            }
        },

        // ====================================================================
        // 19 - 24: Driving Tech / Uplifting 140
        // ====================================================================
        {
            "19 Uplifting 140 BPM Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 5.5f }, { kPreDecay, 4.0f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.30f }, { kPreLevel, 0.78f },
                { kToneMode, 2.0f }, { kToneNote, 7.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4700.0f }, { kToneQ, 4.8f }, { kToneDecay, 180.0f }, { kTonePitchEnv, 6.0f }, { kTonePitchDecay, 10.0f }, { kToneLevel, 0.92f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 2.5f }, { kNoiseDecay, 240.0f }, { kNoiseCurve, -0.42f }, { kNoiseHPF, 1900.0f }, { kNoiseLPF, 19800.0f }, { kNoiseStereoWidth, 1.32f }, { kNoiseLevel, 0.95f },
                { kClickType, 2.0f }, { kClickLevel, 0.64f }, { kClickFreq, 9400.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.16f }, { kDistTone, 0.25f }, { kDistMix, 0.22f },
                { kCompThresh, -17.0f }, { kCompRatio, 5.5f }, { kCompAttack, 2.0f }, { kCompRelease, 50.0f }, { kCompMakeup, 3.2f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 15.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.50f }, { kReverbDecay, 330.0f }, { kReverbDamp, 0.25f }, { kReverbGate, 155.0f }, { kReverbMix, 0.22f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.25f }, { kOutputGain, 0.4f }
            }
        },
        {
            "20 Titanium Laser Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.30f }, { kPreLevel, 0.80f },
                { kToneMode, 2.0f }, { kToneNote, 4.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4900.0f }, { kToneQ, 6.2f }, { kToneDecay, 170.0f }, { kTonePitchEnv, 9.0f }, { kTonePitchDecay, 10.0f }, { kToneLevel, 0.94f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 2.0f }, { kNoiseDecay, 220.0f }, { kNoiseCurve, -0.45f }, { kNoiseHPF, 2050.0f }, { kNoiseLPF, 20000.0f }, { kNoiseStereoWidth, 1.28f }, { kNoiseLevel, 0.94f },
                { kClickType, 2.0f }, { kClickLevel, 0.70f }, { kClickFreq, 9900.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.18f }, { kDistTone, 0.30f }, { kDistMix, 0.24f },
                { kCompThresh, -18.0f }, { kCompRatio, 6.0f }, { kCompAttack, 1.5f }, { kCompRelease, 45.0f }, { kCompMakeup, 3.5f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.1f }, { kLimiterRelease, 14.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.46f }, { kReverbDecay, 290.0f }, { kReverbDamp, 0.28f }, { kReverbGate, 140.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.22f }, { kOutputGain, 0.5f }
            }
        },
        {
            "21 Degenerate Acid Sizzle",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 6.5f }, { kPreDecay, 4.5f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.82f },
                { kToneMode, 3.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3700.0f }, { kToneQ, 5.5f }, { kToneDecay, 175.0f }, { kTonePitchEnv, 7.0f }, { kTonePitchDecay, 11.0f }, { kToneLevel, 0.92f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 2.0f }, { kNoiseDecay, 215.0f }, { kNoiseCurve, -0.42f }, { kNoiseHPF, 1750.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.24f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.66f }, { kClickFreq, 8700.0f },
                { kDistType, 2.0f }, { kDistDrive, 0.20f }, { kDistTone, 0.20f }, { kDistMix, 0.25f },
                { kCompThresh, -18.0f }, { kCompRatio, 6.5f }, { kCompAttack, 1.8f }, { kCompRelease, 48.0f }, { kCompMakeup, 3.8f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.1f }, { kLimiterRelease, 14.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.44f }, { kReverbDecay, 270.0f }, { kReverbDamp, 0.35f }, { kReverbGate, 130.0f }, { kReverbMix, 0.18f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.20f }, { kOutputGain, 0.4f }
            }
        },
        {
            "22 Rolling Off-Beat Tech Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 5.0f }, { kPreDecay, 3.5f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.30f }, { kPreLevel, 0.74f },
                { kToneMode, 0.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3300.0f }, { kToneQ, 4.0f }, { kToneDecay, 160.0f }, { kTonePitchEnv, 4.0f }, { kTonePitchDecay, 10.0f }, { kToneLevel, 0.86f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 2.0f }, { kNoiseDecay, 200.0f }, { kNoiseCurve, -0.42f }, { kNoiseHPF, 1700.0f }, { kNoiseLPF, 19200.0f }, { kNoiseStereoWidth, 1.28f }, { kNoiseLevel, 0.92f },
                { kClickType, 0.0f }, { kClickLevel, 0.60f }, { kClickFreq, 8200.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.12f }, { kDistTone, 0.18f }, { kDistMix, 0.18f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.8f }, { kCompAttack, 2.5f }, { kCompRelease, 50.0f }, { kCompMakeup, 2.8f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 16.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.45f }, { kReverbDecay, 280.0f }, { kReverbDamp, 0.30f }, { kReverbGate, 135.0f }, { kReverbMix, 0.18f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.22f }, { kOutputGain, 0.2f }
            }
        },
        {
            "23 Tech-Trance Snap Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 5.0f }, { kPreDecay, 3.5f }, { kPreJitter, 0.02f }, { kFlamSpread, 0.30f }, { kPreLevel, 0.76f },
                { kToneMode, 3.0f }, { kToneNote, 2.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4200.0f }, { kToneQ, 5.0f }, { kToneDecay, 145.0f }, { kTonePitchEnv, 8.0f }, { kTonePitchDecay, 9.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 1.5f }, { kNoiseDecay, 185.0f }, { kNoiseCurve, -0.50f }, { kNoiseHPF, 2000.0f }, { kNoiseLPF, 19600.0f }, { kNoiseStereoWidth, 1.18f }, { kNoiseLevel, 0.92f },
                { kClickType, 2.0f }, { kClickLevel, 0.72f }, { kClickFreq, 9600.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.17f }, { kDistTone, 0.22f }, { kDistMix, 0.22f },
                { kCompThresh, -17.0f }, { kCompRatio, 5.5f }, { kCompAttack, 1.5f }, { kCompRelease, 42.0f }, { kCompMakeup, 3.2f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 14.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.38f }, { kReverbDecay, 220.0f }, { kReverbDamp, 0.35f }, { kReverbGate, 115.0f }, { kReverbMix, 0.15f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.3f }, { kStereoWidth, 1.14f }, { kOutputGain, 0.3f }
            }
        },
        {
            "24 Underground Warehouse Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 8.0f }, { kPreDecay, 5.5f }, { kPreJitter, 0.07f }, { kFlamSpread, 0.38f }, { kPreLevel, 0.82f },
                { kToneMode, 0.0f }, { kToneNote, 5.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2950.0f }, { kToneQ, 4.2f }, { kToneDecay, 195.0f }, { kTonePitchEnv, 5.5f }, { kTonePitchDecay, 13.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 250.0f }, { kNoiseCurve, -0.38f }, { kNoiseHPF, 1400.0f }, { kNoiseLPF, 17800.0f }, { kNoiseStereoWidth, 1.30f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.62f }, { kClickFreq, 7400.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.19f }, { kDistTone, 0.10f }, { kDistMix, 0.24f },
                { kCompThresh, -17.0f }, { kCompRatio, 5.2f }, { kCompAttack, 2.5f }, { kCompRelease, 55.0f }, { kCompMakeup, 3.4f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 16.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.58f }, { kReverbDecay, 390.0f }, { kReverbDamp, 0.38f }, { kReverbGate, 165.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.24f }, { kOutputGain, 0.4f }
            }
        },

        // ====================================================================
        // 25 - 30: High-Energy Melodic Classics
        // ====================================================================
        {
            "25 Celestial Trance Air",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 9.0f }, { kPreDecay, 6.0f }, { kPreJitter, 0.08f }, { kFlamSpread, 0.40f }, { kPreLevel, 0.74f },
                { kToneMode, 0.0f }, { kToneNote, 8.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3200.0f }, { kToneQ, 3.4f }, { kToneDecay, 245.0f }, { kTonePitchEnv, 3.2f }, { kTonePitchDecay, 16.0f }, { kToneLevel, 0.86f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.0f }, { kNoiseHold, 4.5f }, { kNoiseDecay, 345.0f }, { kNoiseCurve, -0.26f }, { kNoiseHPF, 1300.0f }, { kNoiseLPF, 19000.0f }, { kNoiseStereoWidth, 1.50f }, { kNoiseLevel, 0.95f },
                { kClickType, 0.0f }, { kClickLevel, 0.48f }, { kClickFreq, 7100.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.10f }, { kDistTone, 0.18f }, { kDistMix, 0.16f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 4.0f }, { kCompRelease, 75.0f }, { kCompMakeup, 2.5f }, { kCompMix, 0.95f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.75f }, { kReverbDecay, 640.0f }, { kReverbDamp, 0.22f }, { kReverbGate, 230.0f }, { kReverbMix, 0.34f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.36f }, { kOutputGain, 0.3f }
            }
        },
        {
            "26 Out Of The Blue 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 7.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 0.0f }, { kToneNote, 5.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2900.0f }, { kToneQ, 3.4f }, { kToneDecay, 220.0f }, { kTonePitchEnv, 3.5f }, { kTonePitchDecay, 15.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 295.0f }, { kNoiseCurve, -0.30f }, { kNoiseHPF, 1200.0f }, { kNoiseLPF, 17500.0f }, { kNoiseStereoWidth, 1.28f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.54f }, { kClickFreq, 6700.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.12f }, { kDistTone, 0.12f }, { kDistMix, 0.18f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.2f }, { kCompAttack, 3.5f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.6f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.62f }, { kReverbDecay, 460.0f }, { kReverbDamp, 0.28f }, { kReverbGate, 195.0f }, { kReverbMix, 0.26f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.24f }, { kOutputGain, 0.3f }
            }
        },
        {
            "27 Punk Driven Open Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 7.5f }, { kPreDecay, 5.0f }, { kPreJitter, 0.06f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.84f },
                { kToneMode, 3.0f }, { kToneNote, 4.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2800.0f }, { kToneQ, 4.6f }, { kToneDecay, 185.0f }, { kTonePitchEnv, 6.5f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.92f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 235.0f }, { kNoiseCurve, -0.40f }, { kNoiseHPF, 1250.0f }, { kNoiseLPF, 17000.0f }, { kNoiseStereoWidth, 1.22f }, { kNoiseLevel, 0.94f },
                { kClickType, 1.0f }, { kClickLevel, 0.65f }, { kClickFreq, 6500.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.22f }, { kDistTone, 0.15f }, { kDistMix, 0.26f },
                { kCompThresh, -18.0f }, { kCompRatio, 5.8f }, { kCompAttack, 2.0f }, { kCompRelease, 55.0f }, { kCompMakeup, 3.6f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 16.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.48f }, { kReverbDecay, 310.0f }, { kReverbDamp, 0.35f }, { kReverbGate, 145.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.20f }, { kOutputGain, 0.5f }
            }
        },
        {
            "28 Carte Blanche 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 7.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.72f },
                { kToneMode, 0.0f }, { kToneNote, 10.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3400.0f }, { kToneQ, 3.6f }, { kToneDecay, 230.0f }, { kTonePitchEnv, 3.2f }, { kTonePitchDecay, 15.0f }, { kToneLevel, 0.88f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 4.0f }, { kNoiseDecay, 320.0f }, { kNoiseCurve, -0.28f }, { kNoiseHPF, 1400.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.42f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.50f }, { kClickFreq, 7300.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.10f }, { kDistTone, 0.16f }, { kDistMix, 0.16f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 4.0f }, { kCompRelease, 70.0f }, { kCompMakeup, 2.4f }, { kCompMix, 0.95f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.70f }, { kReverbDecay, 540.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 215.0f }, { kReverbMix, 0.30f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.32f }, { kOutputGain, 0.2f }
            }
        },
        {
            "29 Celestial Anahera Shimmer",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 9.0f }, { kPreDecay, 6.0f }, { kPreJitter, 0.08f }, { kFlamSpread, 0.40f }, { kPreLevel, 0.68f },
                { kToneMode, 2.0f }, { kToneNote, 3.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4200.0f }, { kToneQ, 3.8f }, { kToneDecay, 265.0f }, { kTonePitchEnv, 3.2f }, { kTonePitchDecay, 18.0f }, { kToneLevel, 0.84f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 2.5f }, { kNoiseHold, 5.0f }, { kNoiseDecay, 380.0f }, { kNoiseCurve, -0.22f }, { kNoiseHPF, 1550.0f }, { kNoiseLPF, 19700.0f }, { kNoiseStereoWidth, 1.58f }, { kNoiseLevel, 0.96f },
                { kClickType, 0.0f }, { kClickLevel, 0.44f }, { kClickFreq, 8000.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.26f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.6f }, { kCompAttack, 5.0f }, { kCompRelease, 80.0f }, { kCompMakeup, 2.2f }, { kCompMix, 0.92f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 24.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.82f }, { kReverbDecay, 740.0f }, { kReverbDamp, 0.18f }, { kReverbGate, 265.0f }, { kReverbMix, 0.36f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.5f }, { kStereoWidth, 1.40f }, { kOutputGain, 0.3f }
            }
        },
        {
            "30 High-Tension Ride-Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 8.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.06f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 2.0f }, { kToneNote, 7.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3500.0f }, { kToneQ, 5.8f }, { kToneDecay, 320.0f }, { kTonePitchEnv, 4.0f }, { kTonePitchDecay, 16.0f }, { kToneLevel, 0.94f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 1.0f }, { kNoiseHold, 4.0f }, { kNoiseDecay, 360.0f }, { kNoiseCurve, -0.25f }, { kNoiseHPF, 1450.0f }, { kNoiseLPF, 19000.0f }, { kNoiseStereoWidth, 1.45f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.58f }, { kClickFreq, 7800.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.10f }, { kDistTone, 0.20f }, { kDistMix, 0.16f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 4.0f }, { kCompRelease, 75.0f }, { kCompMakeup, 2.4f }, { kCompMix, 0.94f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.65f }, { kReverbDecay, 500.0f }, { kReverbDamp, 0.22f }, { kReverbGate, 220.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.32f }, { kOutputGain, 0.2f }
            }
        },

        // ====================================================================
        // 31 - 40: Armada, ASOT & Classic Dutch Underground Trance
        // ====================================================================
        {
            "31 Mainstage Off-Beat Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.5f }, { kPreDecay, 4.5f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 0.0f }, { kToneNote, 7.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3150.0f }, { kToneQ, 3.6f }, { kToneDecay, 210.0f }, { kTonePitchEnv, 4.5f }, { kTonePitchDecay, 13.0f }, { kToneLevel, 0.88f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 280.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1450.0f }, { kNoiseLPF, 19200.0f }, { kNoiseStereoWidth, 1.40f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.56f }, { kClickFreq, 7600.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.11f }, { kDistTone, 0.18f }, { kDistMix, 0.17f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.5f }, { kCompAttack, 3.5f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.8f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.62f }, { kReverbDecay, 450.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 190.0f }, { kReverbMix, 0.26f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.30f }, { kOutputGain, 0.3f }
            }
        },
        {
            "32 Trance 138 Anthem Open Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 7.0f }, { kPreDecay, 4.5f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.80f },
                { kToneMode, 2.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3650.0f }, { kToneQ, 4.2f }, { kToneDecay, 220.0f }, { kTonePitchEnv, 5.0f }, { kTonePitchDecay, 13.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 290.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1600.0f }, { kNoiseLPF, 19500.0f }, { kNoiseStereoWidth, 1.46f }, { kNoiseLevel, 0.95f },
                { kClickType, 0.0f }, { kClickLevel, 0.54f }, { kClickFreq, 8100.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.13f }, { kDistTone, 0.25f }, { kDistMix, 0.18f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.6f }, { kCompAttack, 3.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 2.8f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.66f }, { kReverbDecay, 490.0f }, { kReverbDamp, 0.20f }, { kReverbGate, 200.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.34f }, { kOutputGain, 0.4f }
            }
        },
        {
            "33 Melodic Progressive Pocket",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 7.5f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.65f },
                { kToneMode, 1.0f }, { kToneNote, 2.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2700.0f }, { kToneQ, 3.0f }, { kToneDecay, 195.0f }, { kTonePitchEnv, 3.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.80f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 2.5f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 255.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1250.0f }, { kNoiseLPF, 17800.0f }, { kNoiseStereoWidth, 1.32f }, { kNoiseLevel, 0.90f },
                { kClickType, 1.0f }, { kClickLevel, 0.44f }, { kClickFreq, 6600.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.12f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.6f }, { kCompAttack, 4.5f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.56f }, { kReverbDecay, 390.0f }, { kReverbDamp, 0.26f }, { kReverbGate, 175.0f }, { kReverbMix, 0.22f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.24f }, { kOutputGain, 0.0f }
            }
        },
        {
            "34 Who's Afraid Of 138?! Sizzle",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.84f },
                { kToneMode, 2.0f }, { kToneNote, 6.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4600.0f }, { kToneQ, 5.4f }, { kToneDecay, 190.0f }, { kTonePitchEnv, 7.0f }, { kTonePitchDecay, 11.0f }, { kToneLevel, 0.94f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 255.0f }, { kNoiseCurve, -0.38f }, { kNoiseHPF, 1850.0f }, { kNoiseLPF, 19900.0f }, { kNoiseStereoWidth, 1.38f }, { kNoiseLevel, 0.96f },
                { kClickType, 2.0f }, { kClickLevel, 0.68f }, { kClickFreq, 9200.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.18f }, { kDistTone, 0.28f }, { kDistMix, 0.24f },
                { kCompThresh, -18.0f }, { kCompRatio, 6.0f }, { kCompAttack, 2.0f }, { kCompRelease, 50.0f }, { kCompMakeup, 3.6f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.1f }, { kLimiterRelease, 15.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.55f }, { kReverbDecay, 360.0f }, { kReverbDamp, 0.22f }, { kReverbGate, 165.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.30f }, { kOutputGain, 0.5f }
            }
        },
        {
            "35 Dutch Trance 1999 Classic 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.72f },
                { kToneMode, 0.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2800.0f }, { kToneQ, 3.2f }, { kToneDecay, 205.0f }, { kTonePitchEnv, 3.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.88f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 280.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1150.0f }, { kNoiseLPF, 16800.0f }, { kNoiseStereoWidth, 1.18f }, { kNoiseLevel, 0.92f },
                { kClickType, 0.0f }, { kClickLevel, 0.52f }, { kClickFreq, 6400.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.11f }, { kDistTone, 0.08f }, { kDistMix, 0.16f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 3.5f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.5f }, { kCompMix, 0.95f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.60f }, { kReverbDecay, 420.0f }, { kReverbDamp, 0.30f }, { kReverbGate, 180.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.18f }, { kOutputGain, 0.2f }
            }
        },
        {
            "36 FSOE Golden Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.5f }, { kPreDecay, 4.5f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 2.0f }, { kToneNote, 5.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 4400.0f }, { kToneQ, 4.4f }, { kToneDecay, 225.0f }, { kTonePitchEnv, 4.5f }, { kTonePitchDecay, 13.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.5f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 295.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1800.0f }, { kNoiseLPF, 19900.0f }, { kNoiseStereoWidth, 1.48f }, { kNoiseLevel, 0.95f },
                { kClickType, 0.0f }, { kClickLevel, 0.56f }, { kClickFreq, 8600.0f },
                { kDistType, 3.0f }, { kDistDrive, 0.13f }, { kDistTone, 0.30f }, { kDistMix, 0.18f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.8f }, { kCompAttack, 3.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 3.0f }, { kCompMix, 0.98f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.68f }, { kReverbDecay, 510.0f }, { kReverbDamp, 0.18f }, { kReverbGate, 200.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.36f }, { kOutputGain, 0.4f }
            }
        },
        {
            "37 Live Rig Punch 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 5.5f }, { kPreDecay, 4.0f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 0.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3100.0f }, { kToneQ, 3.8f }, { kToneDecay, 185.0f }, { kTonePitchEnv, 4.0f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 245.0f }, { kNoiseCurve, -0.38f }, { kNoiseHPF, 1550.0f }, { kNoiseLPF, 19000.0f }, { kNoiseStereoWidth, 1.30f }, { kNoiseLevel, 0.94f },
                { kClickType, 0.0f }, { kClickLevel, 0.60f }, { kClickFreq, 7700.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.10f }, { kDistTone, 0.18f }, { kDistMix, 0.16f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.5f }, { kCompAttack, 2.8f }, { kCompRelease, 55.0f }, { kCompMakeup, 2.8f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 16.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.52f }, { kReverbDecay, 350.0f }, { kReverbDamp, 0.25f }, { kReverbGate, 160.0f }, { kReverbMix, 0.22f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.25f }, { kOutputGain, 0.3f }
            }
        },
        {
            "38 Subculture Tech Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 6.5f }, { kPreDecay, 4.5f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.82f },
                { kToneMode, 0.0f }, { kToneNote, 2.0f }, { kToneOctave, 7.0f }, { kToneFine, 0.0f }, { kToneFreq, 3900.0f }, { kToneQ, 4.5f }, { kToneDecay, 180.0f }, { kTonePitchEnv, 5.5f }, { kTonePitchDecay, 11.0f }, { kToneLevel, 0.92f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 2.5f }, { kNoiseDecay, 235.0f }, { kNoiseCurve, -0.40f }, { kNoiseHPF, 1750.0f }, { kNoiseLPF, 19400.0f }, { kNoiseStereoWidth, 1.32f }, { kNoiseLevel, 0.95f },
                { kClickType, 0.0f }, { kClickLevel, 0.64f }, { kClickFreq, 8400.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.16f }, { kDistTone, 0.22f }, { kDistMix, 0.20f },
                { kCompThresh, -17.0f }, { kCompRatio, 5.2f }, { kCompAttack, 2.2f }, { kCompRelease, 50.0f }, { kCompMakeup, 3.2f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 16.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.50f }, { kReverbDecay, 330.0f }, { kReverbDamp, 0.26f }, { kReverbGate, 150.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.26f }, { kOutputGain, 0.4f }
            }
        },
        {
            "39 Hard Tech Sizzle Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 5.5f }, { kPreDecay, 4.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.86f },
                { kToneMode, 3.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3450.0f }, { kToneQ, 5.8f }, { kToneDecay, 155.0f }, { kTonePitchEnv, 8.0f }, { kTonePitchDecay, 10.0f }, { kToneLevel, 0.94f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 2.0f }, { kNoiseDecay, 200.0f }, { kNoiseCurve, -0.45f }, { kNoiseHPF, 1900.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.20f }, { kNoiseLevel, 0.95f },
                { kClickType, 2.0f }, { kClickLevel, 0.74f }, { kClickFreq, 9200.0f },
                { kDistType, 2.0f }, { kDistDrive, 0.22f }, { kDistTone, 0.24f }, { kDistMix, 0.26f },
                { kCompThresh, -18.0f }, { kCompRatio, 6.5f }, { kCompAttack, 1.5f }, { kCompRelease, 45.0f }, { kCompMakeup, 3.8f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.1f }, { kLimiterRelease, 14.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.42f }, { kReverbDecay, 240.0f }, { kReverbDamp, 0.35f }, { kReverbGate, 120.0f }, { kReverbMix, 0.16f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.18f }, { kOutputGain, 0.5f }
            }
        },
        {
            "40 Wake Your Mind Open Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 8.5f }, { kPreDecay, 5.5f }, { kPreJitter, 0.07f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 3.0f }, { kToneNote, 4.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2750.0f }, { kToneQ, 3.8f }, { kToneDecay, 200.0f }, { kTonePitchEnv, 5.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.86f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 2.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 270.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1250.0f }, { kNoiseLPF, 17800.0f }, { kNoiseStereoWidth, 1.36f }, { kNoiseLevel, 0.92f },
                { kClickType, 1.0f }, { kClickLevel, 0.52f }, { kClickFreq, 6800.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.14f }, { kDistTone, 0.12f }, { kDistMix, 0.18f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.2f }, { kCompAttack, 4.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 2.5f }, { kCompMix, 0.94f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.58f }, { kReverbDecay, 400.0f }, { kReverbDamp, 0.28f }, { kReverbGate, 175.0f }, { kReverbMix, 0.24f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.4f }, { kStereoWidth, 1.28f }, { kOutputGain, 0.2f }
            }
        },

        // ====================================================================
        // 41 - 48: Modern EDM, Progressive & Underground Club Utility Hats
        // ====================================================================
        {
            "41 Arena Progressive Crisp Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.03f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.65f },
                { kToneMode, 0.0f }, { kToneNote, 7.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3050.0f }, { kToneQ, 3.2f }, { kToneDecay, 175.0f }, { kTonePitchEnv, 3.0f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.82f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.0f }, { kNoiseHold, 2.5f }, { kNoiseDecay, 225.0f }, { kNoiseCurve, -0.38f }, { kNoiseHPF, 1550.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.28f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.50f }, { kClickFreq, 7400.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.07f }, { kDistTone, 0.15f }, { kDistMix, 0.12f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.5f }, { kCompAttack, 4.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.50f }, { kReverbDecay, 340.0f }, { kReverbDamp, 0.25f }, { kReverbGate, 150.0f }, { kReverbMix, 0.18f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.22f }, { kOutputGain, 0.0f }
            }
        },
        {
            "42 Electro House Pluck Hat",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 5.0f }, { kPreDecay, 3.5f }, { kPreJitter, 0.02f }, { kFlamSpread, 0.30f }, { kPreLevel, 0.62f },
                { kToneMode, 3.0f }, { kToneNote, 5.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2850.0f }, { kToneQ, 4.2f }, { kToneDecay, 130.0f }, { kTonePitchEnv, 3.5f }, { kTonePitchDecay, 10.0f }, { kToneLevel, 0.80f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 1.5f }, { kNoiseDecay, 160.0f }, { kNoiseCurve, -0.48f }, { kNoiseHPF, 1650.0f }, { kNoiseLPF, 18200.0f }, { kNoiseStereoWidth, 1.12f }, { kNoiseLevel, 0.88f },
                { kClickType, 0.0f }, { kClickLevel, 0.58f }, { kClickFreq, 7800.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.06f }, { kDistTone, 0.10f }, { kDistMix, 0.12f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.8f }, { kCompAttack, 3.0f }, { kCompRelease, 45.0f }, { kCompMakeup, 2.0f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 15.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.35f }, { kReverbDecay, 200.0f }, { kReverbDamp, 0.35f }, { kReverbGate, 100.0f }, { kReverbMix, 0.12f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.3f }, { kStereoWidth, 1.10f }, { kOutputGain, 0.0f }
            }
        },
        {
            "43 Festival Mainstage Wide Hat",
            {
                { kPreBursts, 3.0f }, { kPreSpread, 9.0f }, { kPreDecay, 6.0f }, { kPreJitter, 0.10f }, { kFlamSpread, 0.45f }, { kPreLevel, 0.86f },
                { kToneMode, 0.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3450.0f }, { kToneQ, 3.6f }, { kToneDecay, 245.0f }, { kTonePitchEnv, 5.0f }, { kTonePitchDecay, 15.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 4.5f }, { kNoiseDecay, 335.0f }, { kNoiseCurve, -0.22f }, { kNoiseHPF, 1350.0f }, { kNoiseLPF, 19800.0f }, { kNoiseStereoWidth, 1.78f }, { kNoiseLevel, 0.96f },
                { kClickType, 0.0f }, { kClickLevel, 0.62f }, { kClickFreq, 8000.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.16f }, { kDistTone, 0.20f }, { kDistMix, 0.22f },
                { kCompThresh, -17.0f }, { kCompRatio, 5.0f }, { kCompAttack, 3.5f }, { kCompRelease, 75.0f }, { kCompMakeup, 3.5f }, { kCompMix, 1.0f },
                { kLimiterCeiling, -0.1f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.80f }, { kReverbDecay, 680.0f }, { kReverbDamp, 0.20f }, { kReverbGate, 245.0f }, { kReverbMix, 0.36f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.5f }, { kStereoWidth, 1.48f }, { kOutputGain, 0.5f }
            }
        },
        {
            "44 Underground Bunker 808-909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.04f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.74f },
                { kToneMode, 3.0f }, { kToneNote, 6.0f }, { kToneOctave, 5.0f }, { kToneFine, 0.0f }, { kToneFreq, 2300.0f }, { kToneQ, 4.2f }, { kToneDecay, 195.0f }, { kTonePitchEnv, 4.0f }, { kTonePitchDecay, 14.0f }, { kToneLevel, 0.90f },
                { kNoiseType, 3.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 255.0f }, { kNoiseCurve, -0.35f }, { kNoiseHPF, 1000.0f }, { kNoiseLPF, 15800.0f }, { kNoiseStereoWidth, 1.15f }, { kNoiseLevel, 0.90f },
                { kClickType, 1.0f }, { kClickLevel, 0.54f }, { kClickFreq, 5600.0f },
                { kDistType, 1.0f }, { kDistDrive, 0.19f }, { kDistTone, -0.05f }, { kDistMix, 0.22f },
                { kCompThresh, -16.0f }, { kCompRatio, 4.8f }, { kCompAttack, 3.0f }, { kCompRelease, 60.0f }, { kCompMakeup, 3.0f }, { kCompMix, 0.96f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 18.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.52f }, { kReverbDecay, 340.0f }, { kReverbDamp, 0.42f }, { kReverbGate, 150.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.12f }, { kOutputGain, 0.2f }
            }
        },
        {
            "45 Off-Beat Sidechain Swell Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 12.0f }, { kPreDecay, 8.0f }, { kPreJitter, 0.08f }, { kFlamSpread, 0.40f }, { kPreLevel, 0.65f },
                { kToneMode, 1.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3150.0f }, { kToneQ, 3.0f }, { kToneDecay, 255.0f }, { kTonePitchEnv, 2.0f }, { kTonePitchDecay, 20.0f }, { kToneLevel, 0.82f },
                { kNoiseType, 1.0f }, { kNoiseAttack, 14.0f }, { kNoiseHold, 5.0f }, { kNoiseDecay, 325.0f }, { kNoiseCurve, -0.18f }, { kNoiseHPF, 1350.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.48f }, { kNoiseLevel, 0.94f },
                { kClickType, 1.0f }, { kClickLevel, 0.25f }, { kClickFreq, 6400.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.07f }, { kDistTone, 0.15f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 3.8f }, { kCompAttack, 8.0f }, { kCompRelease, 80.0f }, { kCompMakeup, 2.2f }, { kCompMix, 0.90f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 22.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.68f }, { kReverbDecay, 520.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 210.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.45f }, { kStereoWidth, 1.35f }, { kOutputGain, 0.0f }
            }
        },
        {
            "46 Reverse Cymbal Pickup Hat",
            {
                { kPreBursts, 2.0f }, { kPreSpread, 8.0f }, { kPreDecay, 5.5f }, { kPreJitter, 0.06f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.76f },
                { kToneMode, 2.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3350.0f }, { kToneQ, 3.6f }, { kToneDecay, 225.0f }, { kTonePitchEnv, 4.0f }, { kTonePitchDecay, 15.0f }, { kToneLevel, 0.86f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 1.0f }, { kNoiseHold, 3.5f }, { kNoiseDecay, 295.0f }, { kNoiseCurve, -0.30f }, { kNoiseHPF, 1450.0f }, { kNoiseLPF, 19000.0f }, { kNoiseStereoWidth, 1.40f }, { kNoiseLevel, 0.92f },
                { kClickType, 0.0f }, { kClickLevel, 0.50f }, { kClickFreq, 7600.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.09f }, { kDistTone, 0.18f }, { kDistMix, 0.15f },
                { kCompThresh, -15.0f }, { kCompRatio, 4.0f }, { kCompAttack, 4.0f }, { kCompRelease, 70.0f }, { kCompMakeup, 2.5f }, { kCompMix, 0.94f },
                { kLimiterCeiling, -0.2f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.65f }, { kReverbDecay, 480.0f }, { kReverbDamp, 0.24f }, { kReverbGate, 195.0f }, { kReverbMix, 0.28f },
                { kReverseMode, 1.0f }, { kReverseMix, 0.68f }, { kStereoWidth, 1.30f }, { kOutputGain, 0.2f }
            }
        },
        {
            "47 Fast 16th Stutter Open Hat",
            {
                { kPreBursts, 4.0f }, { kPreSpread, 11.0f }, { kPreDecay, 5.0f }, { kPreJitter, 0.05f }, { kFlamSpread, 0.45f }, { kPreLevel, 0.88f },
                { kToneMode, 2.0f }, { kToneNote, 9.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 3550.0f }, { kToneQ, 4.0f }, { kToneDecay, 135.0f }, { kTonePitchEnv, 4.5f }, { kTonePitchDecay, 10.0f }, { kToneLevel, 0.84f },
                { kNoiseType, 2.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 1.5f }, { kNoiseDecay, 155.0f }, { kNoiseCurve, -0.52f }, { kNoiseHPF, 1700.0f }, { kNoiseLPF, 19200.0f }, { kNoiseStereoWidth, 1.48f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.54f }, { kClickFreq, 8000.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.08f }, { kDistTone, 0.15f }, { kDistMix, 0.14f },
                { kCompThresh, -14.0f }, { kCompRatio, 4.0f }, { kCompAttack, 2.5f }, { kCompRelease, 45.0f }, { kCompMakeup, 2.2f }, { kCompMix, 0.92f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 15.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.42f }, { kReverbDecay, 240.0f }, { kReverbDamp, 0.30f }, { kReverbGate, 110.0f }, { kReverbMix, 0.18f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.28f }, { kOutputGain, 0.0f }
            }
        },
        {
            "48 Pure Zero-Drive Clean 909",
            {
                { kPreBursts, 1.0f }, { kPreSpread, 6.0f }, { kPreDecay, 4.0f }, { kPreJitter, 0.02f }, { kFlamSpread, 0.35f }, { kPreLevel, 0.62f },
                { kToneMode, 0.0f }, { kToneNote, 6.0f }, { kToneOctave, 6.0f }, { kToneFine, 0.0f }, { kToneFreq, 2900.0f }, { kToneQ, 3.0f }, { kToneDecay, 190.0f }, { kTonePitchEnv, 3.0f }, { kTonePitchDecay, 12.0f }, { kToneLevel, 0.82f },
                { kNoiseType, 0.0f }, { kNoiseAttack, 0.0f }, { kNoiseHold, 3.0f }, { kNoiseDecay, 265.0f }, { kNoiseCurve, -0.32f }, { kNoiseHPF, 1350.0f }, { kNoiseLPF, 18800.0f }, { kNoiseStereoWidth, 1.18f }, { kNoiseLevel, 0.90f },
                { kClickType, 0.0f }, { kClickLevel, 0.46f }, { kClickFreq, 7000.0f },
                { kDistType, 0.0f }, { kDistDrive, 0.0f }, { kDistTone, 0.0f }, { kDistMix, 0.0f },
                { kCompThresh, -12.0f }, { kCompRatio, 3.0f }, { kCompAttack, 5.0f }, { kCompRelease, 65.0f }, { kCompMakeup, 1.5f }, { kCompMix, 0.85f },
                { kLimiterCeiling, -0.3f }, { kLimiterRelease, 20.0f }, { kLimiterEnabled, 1.0f },
                { kReverbSize, 0.50f }, { kReverbDecay, 350.0f }, { kReverbDamp, 0.25f }, { kReverbGate, 165.0f }, { kReverbMix, 0.20f },
                { kReverseMode, 0.0f }, { kReverseMix, 0.35f }, { kStereoWidth, 1.14f }, { kOutputGain, 0.0f }
            }
        }
    };
    return kPresets;
}

void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state, bool preservePitch) {
    for (const auto& [paramId, value] : preset.values) {
        if (preservePitch && (paramId == kToneOctave || paramId == kToneFreq || paramId == kToneNote || paramId == kToneFine)) {
            continue;
        }
        if (auto* param = state.getParameter(paramId)) {
            param->setValueNotifyingHost(param->convertTo0to1(value));
        }
    }
}

} // namespace openhatter::presets
