#include "Parameters.h"
#include <vector>

namespace openhatter::params {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Stick Bounce / Pre-Chick Articulation
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ kPreBursts, 1 }, "Stick Bounces", 1, 5, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreSpread, 1 }, "Bounce Spread", juce::NormalisableRange<float>(2.0f, 45.0f, 0.1f), 8.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreDecay, 1 }, "Bounce Decay", juce::NormalisableRange<float>(2.0f, 35.0f, 0.1f), 6.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreJitter, 1 }, "Bounce Humanize", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.08f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kFlamSpread, 1 }, "Bounce Pan", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreLevel, 1 }, "Bounce Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.65f));

    // Open / closed decay morph and tempo-synced echo
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDecayScale, 1 }, "Decay Scale", juce::NormalisableRange<float>(0.25f, 2.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEchoMix, 1 }, "Echo Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kAmpEnabled, 1 }, "Full ADSR", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpAttack, 1 }, "Amp Attack", juce::NormalisableRange<float>(0.05f, 20.0f, 0.05f), 0.35f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpDecay, 1 }, "Amp Decay", juce::NormalisableRange<float>(1.0f, 600.0f, 1.0f, 0.35f), 72.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpSustain, 1 }, "Amp Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.70f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpRelease, 1 }, "Amp Release", juce::NormalisableRange<float>(0.0f, 20000.0f, 1.0f, 0.35f), 180.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Air Sizzle & Shimmer Hiss
    juce::StringArray noiseTypes{
        "909 Crisp",
        "Anjuna Silk",
        "Velvet Shimmer",
        "Lo-Fi Crunch",
        "808 Analog Metal",
        "Bronze Shimmer",
        "Trance Sizzle",
        "Spectral Wash"
    };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kNoiseType, 1 }, "Hiss Type", noiseTypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseAttack, 1 }, "Off-Beat Swell", juce::NormalisableRange<float>(0.0f, 40.0f, 0.1f), 1.5f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseHold, 1 }, "Air Hold", juce::NormalisableRange<float>(0.0f, 120.0f, 0.1f), 12.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseDecay, 1 }, "Open Decay", juce::NormalisableRange<float>(20.0f, 2400.0f, 1.0f, 0.38f), 420.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseCurve, 1 }, "Tail Curve", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), -0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseHPF, 1 }, "Air HPF", juce::NormalisableRange<float>(60.0f, 6000.0f, 1.0f, 0.35f), 480.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseLPF, 1 }, "Air LPF", juce::NormalisableRange<float>(1500.0f, 20000.0f, 1.0f, 0.35f), 16500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseStereoWidth, 1 }, "Shimmer Width", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseLevel, 1 }, "Air Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.88f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kVelocityAir, 1 }, "Air Velocity Curve", juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.5f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kHissColor, 1 }, "Hiss Color", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kHissDensity, 1 }, "Hiss Density", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kHissDrive, 1 }, "Hiss Saturation", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f));

    // Spun Bronze Cymbal Body
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kBodyLevel, 1 }, "Body Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kBodyTone, 1 }, "Body Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kBodyDecay, 1 }, "Body Decay", juce::NormalisableRange<float>(25.0f, 900.0f, 1.0f, 0.40f), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Stick Chick / Transient
    juce::StringArray clickTypes{ "Crisp Stick", "Pedal Foot", "Laser Tick" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kClickType, 1 }, "Chick Type", clickTypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kClickLevel, 1 }, "Chick Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kClickFreq, 1 }, "Chick Freq", juce::NormalisableRange<float>(1000.0f, 14000.0f, 10.0f, 0.35f), 6800.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kVelocityChick, 1 }, "Chick Velocity Curve", juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.5f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kKeyTracking, 1 }, "Key Tracking", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Shimmer Saturation & Exciter
    juce::StringArray distTypes{ "Tape Warmth", "Tube Sheen", "Hard Clip", "Crystal Exciter" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kDistType, 1 }, "Exciter Type", distTypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDistDrive, 1 }, "Exciter Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.08f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDistTone, 1 }, "Exciter Tilt", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDistMix, 1 }, "Exciter Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f));

    // Compressor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompThresh, 1 }, "Comp Threshold", juce::NormalisableRange<float>(-40.0f, 0.0f, 0.5f), -14.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompRatio, 1 }, "Comp Ratio", juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f), 3.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompAttack, 1 }, "Comp Attack", juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.35f), 4.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompRelease, 1 }, "Comp Release", juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.35f), 65.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompMakeup, 1 }, "Comp Makeup", juce::NormalisableRange<float>(0.0f, 24.0f, 0.5f), 2.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompMix, 1 }, "Comp Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    // Limiter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kLimiterCeiling, 1 }, "Limiter Ceiling", juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.3f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kLimiterRelease, 1 }, "Limiter Release", juce::NormalisableRange<float>(5.0f, 100.0f, 0.5f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kLimiterEnabled, 1 }, "Limiter Enabled", true));

    // Trance Shimmer Reverb & Choke Gate
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbSize, 1 }, "Reverb Size", juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbDecay, 1 }, "Reverb Decay", juce::NormalisableRange<float>(50.0f, 2500.0f, 10.0f, 0.35f), 620.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbDamp, 1 }, "Reverb Damp", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbGate, 1 }, "Choke Gate", juce::NormalisableRange<float>(0.0f, 1500.0f, 5.0f), 320.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbMix, 1 }, "Reverb Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.24f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kVelocityReverb, 1 }, "Reverb Velocity Curve", juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.5f), 1.0f));

    // Reverse & Sync
    juce::StringArray revModes{ "Off", "Beat End (Sync)", "Beat 4 Auto", "1/2 Beat", "1/4 Beat" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kReverseMode, 1 }, "Reverse Mode", revModes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverseMix, 1 }, "Reverse Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // Global / Master
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kLockIn, 1 }, "Lock In (WAV)", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kStereoWidth, 1 }, "Master Width", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kOutputGain, 1 }, "Output Gain", juce::NormalisableRange<float>(-36.0f, 12.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    const auto makeFreqRange = [](float minHz, float maxHz, float midHz) {
        juce::NormalisableRange<float> range(minHz, maxHz, 0.1f);
        range.setSkewForCentre(midHz);
        return range;
    };
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqLowFreq, 1 }, "EQ Low Freq", makeFreqRange(25.0f, 400.0f, 100.0f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqLowDb, 1 }, "EQ Low", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqMidFreq, 1 }, "EQ Mid Freq", makeFreqRange(200.0f, 6000.0f, 1100.0f), 1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqMidDb, 1 }, "EQ Mid", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqHighFreq, 1 }, "EQ High Freq", makeFreqRange(2000.0f, 18000.0f, 6000.0f), 7500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqHighDb, 1 }, "EQ High", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ kMidiChokeNote, 1 }, "MIDI Choke Note", 0, 127, 42));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ kMidiChokeCc, 1 }, "MIDI Choke CC", 0, 119, 4));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kRollEnabled, 1 }, "Roll Enabled", false));
    juce::StringArray rollDivisions{ "1/8", "1/16", "1/32" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kRollDivision, 1 }, "Roll Division", rollDivisions, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kRollSwing, 1 }, "Roll Swing", juce::NormalisableRange<float>(0.0f, 0.49f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kRollAccent, 1 }, "Roll Accent", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ kPreset, 1 }, "Preset Index", 0, 55, 0));

    return { params.begin(), params.end() };
}

openhatter::core::EngineSettings readSettings(const juce::AudioProcessorValueTreeState& apvts) {
    openhatter::core::EngineSettings s;

    s.synth.preChickCount = static_cast<int>(*apvts.getRawParameterValue(kPreBursts));
    s.synth.preChickSpreadMs = *apvts.getRawParameterValue(kPreSpread);
    s.synth.preChickDecayMs = *apvts.getRawParameterValue(kPreDecay);
    s.synth.preChickJitter = *apvts.getRawParameterValue(kPreJitter);
    if (const auto* pFlamSpread = apvts.getRawParameterValue(kFlamSpread)) {
        s.synth.preChickFlamSpread = *pFlamSpread;
    }
    s.synth.preChickLevel = *apvts.getRawParameterValue(kPreLevel);

    if (const auto* pDecayScale = apvts.getRawParameterValue(kDecayScale)) s.synth.decayScale = *pDecayScale;
    if (const auto* pEchoMix = apvts.getRawParameterValue(kEchoMix)) s.effects.echoMix = *pEchoMix;
    s.synth.fullAdsrEnabled = *apvts.getRawParameterValue(kAmpEnabled) > 0.5f;
    s.synth.ampAttackMs = *apvts.getRawParameterValue(kAmpAttack);
    s.synth.ampDecayMs = *apvts.getRawParameterValue(kAmpDecay);
    s.synth.ampSustain = *apvts.getRawParameterValue(kAmpSustain);
    s.synth.ampReleaseMs = *apvts.getRawParameterValue(kAmpRelease);

    s.synth.noiseType = static_cast<openhatter::core::NoiseType>(static_cast<int>(*apvts.getRawParameterValue(kNoiseType)));
    s.synth.noiseAttackMs = *apvts.getRawParameterValue(kNoiseAttack);
    s.synth.noiseHoldMs = *apvts.getRawParameterValue(kNoiseHold);
    s.synth.noiseDecayMs = *apvts.getRawParameterValue(kNoiseDecay);
    s.synth.noiseCurve = *apvts.getRawParameterValue(kNoiseCurve);
    s.synth.noiseHPF = *apvts.getRawParameterValue(kNoiseHPF);
    s.synth.noiseLPF = *apvts.getRawParameterValue(kNoiseLPF);
    s.synth.noiseStereoWidth = *apvts.getRawParameterValue(kNoiseStereoWidth);
    s.synth.noiseLevel = *apvts.getRawParameterValue(kNoiseLevel);
    s.synth.velocityAir = *apvts.getRawParameterValue(kVelocityAir);
    if (const auto* pColor = apvts.getRawParameterValue(kHissColor)) s.synth.hissColor = *pColor;
    if (const auto* pDensity = apvts.getRawParameterValue(kHissDensity)) s.synth.hissDensity = *pDensity;
    if (const auto* pDrive = apvts.getRawParameterValue(kHissDrive)) s.synth.hissDrive = *pDrive;
    if (const auto* pBodyLevel = apvts.getRawParameterValue(kBodyLevel)) s.synth.bodyLevel = *pBodyLevel;
    if (const auto* pBodyTone = apvts.getRawParameterValue(kBodyTone)) s.synth.bodyTone = *pBodyTone;
    if (const auto* pBodyDecay = apvts.getRawParameterValue(kBodyDecay)) s.synth.bodyDecayMs = *pBodyDecay;

    if (const auto* pClickType = apvts.getRawParameterValue(kClickType)) {
        s.synth.chickType = static_cast<openhatter::core::ChickType>(static_cast<int>(*pClickType));
    }
    s.synth.chickLevel = *apvts.getRawParameterValue(kClickLevel);
    s.synth.chickFreqHz = *apvts.getRawParameterValue(kClickFreq);
    s.synth.velocityChick = *apvts.getRawParameterValue(kVelocityChick);
    if (const auto* pKeyTracking = apvts.getRawParameterValue(kKeyTracking)) s.synth.keyTracking = *pKeyTracking;

    s.synth.reverbSize = *apvts.getRawParameterValue(kReverbSize);
    s.synth.reverbDecayMs = *apvts.getRawParameterValue(kReverbDecay);
    s.synth.reverbDamp = *apvts.getRawParameterValue(kReverbDamp);
    s.synth.reverbGateMs = *apvts.getRawParameterValue(kReverbGate);
    s.synth.reverbMix = *apvts.getRawParameterValue(kReverbMix);
    s.synth.velocityReverb = *apvts.getRawParameterValue(kVelocityReverb);

    s.effects.distType = static_cast<openhatter::core::DistortionType>(static_cast<int>(*apvts.getRawParameterValue(kDistType)));
    s.effects.distDrive = *apvts.getRawParameterValue(kDistDrive);
    s.effects.distTone = *apvts.getRawParameterValue(kDistTone);
    s.effects.distMix = *apvts.getRawParameterValue(kDistMix);

    s.effects.compressor.thresholdDb = *apvts.getRawParameterValue(kCompThresh);
    s.effects.compressor.ratio = *apvts.getRawParameterValue(kCompRatio);
    s.effects.compressor.attackMs = *apvts.getRawParameterValue(kCompAttack);
    s.effects.compressor.releaseMs = *apvts.getRawParameterValue(kCompRelease);
    s.effects.compressor.makeupDb = *apvts.getRawParameterValue(kCompMakeup);
    s.effects.compressor.mix = *apvts.getRawParameterValue(kCompMix);

    s.effects.limiter.ceilingDb = *apvts.getRawParameterValue(kLimiterCeiling);
    s.effects.limiter.releaseMs = *apvts.getRawParameterValue(kLimiterRelease);
    s.effects.limiter.enabled = *apvts.getRawParameterValue(kLimiterEnabled) > 0.5f;

    s.effects.reverseMode = static_cast<openhatter::core::ReverseSyncMode>(static_cast<int>(*apvts.getRawParameterValue(kReverseMode)));
    s.effects.reverseMix = *apvts.getRawParameterValue(kReverseMix);

    s.effects.eq.lowFreqHz = *apvts.getRawParameterValue(kEqLowFreq);
    s.effects.eq.lowGainDb = *apvts.getRawParameterValue(kEqLowDb);
    s.effects.eq.midFreqHz = *apvts.getRawParameterValue(kEqMidFreq);
    s.effects.eq.midGainDb = *apvts.getRawParameterValue(kEqMidDb);
    s.effects.eq.highFreqHz = *apvts.getRawParameterValue(kEqHighFreq);
    s.effects.eq.highGainDb = *apvts.getRawParameterValue(kEqHighDb);
    s.effects.stereoWidth = *apvts.getRawParameterValue(kStereoWidth);
    s.effects.outputGainDb = *apvts.getRawParameterValue(kOutputGain);
    s.midiChokeNote = static_cast<int>(*apvts.getRawParameterValue(kMidiChokeNote));
    s.midiChokeCc = static_cast<int>(*apvts.getRawParameterValue(kMidiChokeCc));
    s.rollEnabled = *apvts.getRawParameterValue(kRollEnabled) > 0.5f;
    s.rollDivision = static_cast<int>(*apvts.getRawParameterValue(kRollDivision));
    s.rollSwing = *apvts.getRawParameterValue(kRollSwing);
    s.rollAccent = *apvts.getRawParameterValue(kRollAccent);

    return s;
}

} // namespace openhatter::params
