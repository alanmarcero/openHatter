#include "Parameters.h"
#include <optional>
#include <vector>

namespace openhatter::params {

namespace {

using FloatAttributes = juce::AudioParameterFloatAttributes;

float parseMs(const juce::String& text) {
    const auto trimmed = text.trim().toLowerCase();
    const float number = trimmed.getFloatValue();
    if (trimmed.endsWith("ms")) return number;
    if (trimmed.endsWith("s")) return number * 1000.0f;
    return number;
}

float parseHz(const juce::String& text) {
    const float number = text.getFloatValue();
    if (text.containsIgnoreCase("k")) return number * 1000.0f;
    return number;
}

juce::String formatPercent(float amount) {
    return juce::String(juce::roundToInt(amount * 100.0f)) + "%";
}

float parsePercent(const juce::String& text) {
    return text.getFloatValue() / 100.0f;
}

juce::String formatSignedPercent(float amount) {
    const int percent = juce::roundToInt(amount * 100.0f);
    return juce::String(percent > 0 ? "+" : "") + juce::String(percent) + "%";
}

juce::String formatRatio(float ratio) {
    return juce::String(ratio, 1) + ":1";
}

// At 0 ms the reverb gate stays open, so the display shows Off.
juce::String formatGate(float ms) {
    if (ms <= 0.0f) return "Off";
    return formatMs(ms);
}

float parseGate(const juce::String& text) {
    if (text.trim().equalsIgnoreCase("off")) return 0.0f;
    return parseMs(text);
}

FloatAttributes withText(std::function<juce::String(float)> format, std::function<float(const juce::String&)> parse) {
    return FloatAttributes()
        .withStringFromValueFunction([formatValue = std::move(format)](float value, int maximumLength) {
            const auto text = formatValue(value);
            if (maximumLength <= 0) return text;
            return text.substring(0, maximumLength);
        })
        .withValueFromStringFunction(std::move(parse));
}

} // namespace

juce::String formatMs(float ms) {
    if (ms <= 0.0f) return "0 ms";
    if (ms >= 999.5f) return juce::String(ms / 1000.0f, 2) + " s";
    if (ms >= 9.95f) return juce::String(juce::roundToInt(ms)) + " ms";
    return juce::String(ms, ms >= 0.995f ? 1 : 2) + " ms";
}

juce::String formatDb(float db) {
    const int tenths = juce::roundToInt(db * 10.0f);
    if (tenths == 0) return "0.0 dB";
    return juce::String(tenths > 0 ? "+" : "") + juce::String(static_cast<float>(tenths) / 10.0f, 1) + " dB";
}

juce::String formatHz(float hz) {
    if (hz >= 9995.0f) return juce::String(hz / 1000.0f, 1) + " kHz";
    if (hz >= 999.5f) return juce::String(hz / 1000.0f, 2) + " kHz";
    return juce::String(juce::roundToInt(hz)) + " Hz";
}

float eqNoteToHz(int midiNote) noexcept { return suite::eq::noteToHz(midiNote); }

int eqHzToNearestMidi(float hz, int minMidi, int maxMidi) noexcept {
    return suite::eq::nearestMidi(hz, minMidi, maxMidi);
}

juce::String formatEqNote(float hz, int minMidi, int maxMidi) {
    return suite::eq::formatNote(hz, minMidi, maxMidi);
}

std::pair<int, int> eqBandMidiLimits(const juce::String& freqParamId) noexcept {
    return suite::eq::bandLimits(freqParamId);
}

float parseEqNoteOrHz(const juce::String& text, int minMidi, int maxMidi) {
    return suite::eq::parseNoteOrHz(text, minMidi, maxMidi, [](const juce::String& trimmed) {
        return parseHz(trimmed);
    });
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    const auto msText = withText(formatMs, parseMs);
    const auto hzText = withText(formatHz, parseHz);
    const auto percentText = withText(formatPercent, parsePercent);
    const auto signedPercentText = withText(formatSignedPercent, parsePercent);
    const auto parseNumber = [](const juce::String& text) { return text.getFloatValue(); };
    const auto dbText = withText(formatDb, parseNumber);

    // Stick Bounce / Pre-Chick Articulation
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ kPreBursts, 1 }, "Stick Strikes", 1, 5, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreSpread, 1 }, "Bounce Spread", juce::NormalisableRange<float>(2.0f, 45.0f, 0.1f), 8.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreDecay, 1 }, "Bounce Decay", juce::NormalisableRange<float>(2.0f, 35.0f, 0.1f), 6.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreJitter, 1 }, "Bounce Humanize", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.08f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kFlamSpread, 1 }, "Bounce Pan", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kPreLevel, 1 }, "Bounce Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.65f,
        percentText));

    // Tempo-synced echo
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEchoMix, 1 }, "Echo Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kAmpEnabled, 1 }, "Hold Notes", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpAttack, 1 }, "Amp Attack", juce::NormalisableRange<float>(0.05f, 20.0f, 0.05f), 0.35f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpDecay, 1 }, "Amp Decay", juce::NormalisableRange<float>(1.0f, 600.0f, 1.0f, 0.35f), 72.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpSustain, 1 }, "Amp Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.70f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kAmpRelease, 1 }, "Amp Release", juce::NormalisableRange<float>(0.0f, 20000.0f, 1.0f, 0.35f), 180.0f,
        msText));

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
        juce::ParameterID{ kNoiseType, 1 }, "Air Type", noiseTypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseAttack, 1 }, "Air Attack", juce::NormalisableRange<float>(0.0f, 40.0f, 0.1f), 1.5f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseHold, 1 }, "Air Hold", juce::NormalisableRange<float>(0.0f, 120.0f, 0.1f), 12.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseDecay, 1 }, "Air Decay", juce::NormalisableRange<float>(20.0f, 2400.0f, 1.0f, 0.38f), 420.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseCurve, 1 }, "Air Curve", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), -0.35f,
        signedPercentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseHPF, 1 }, "Air High-pass", juce::NormalisableRange<float>(60.0f, 6000.0f, 1.0f, 0.35f), 480.0f,
        hzText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseLPF, 1 }, "Air Low-pass", juce::NormalisableRange<float>(1500.0f, 20000.0f, 1.0f, 0.35f), 16500.0f,
        hzText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseStereoWidth, 1 }, "Air Width", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.2f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kNoiseLevel, 1 }, "Air Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.88f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kVelocity, 1 }, "Velocity", juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.5f), 1.0f));

    // Spun Bronze Cymbal Body
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kBodyLevel, 1 }, "Body Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.55f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kBodyTone, 1 }, "Body Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.45f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kBodyDecay, 1 }, "Body Decay", juce::NormalisableRange<float>(25.0f, 900.0f, 1.0f, 0.40f), 150.0f,
        msText));

    // Stick Chick / Transient
    juce::StringArray clickTypes{ "Crisp Stick", "Pedal Foot", "Laser Tick" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kClickType, 1 }, "Stick Contact", clickTypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kClickLevel, 1 }, "Stick Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.45f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kClickFreq, 1 }, "Stick Tone", juce::NormalisableRange<float>(1000.0f, 14000.0f, 10.0f, 0.35f), 6800.0f,
        hzText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kKeyTracking, 1 }, "Key Tracking", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        percentText));

    // Shimmer Saturation & Exciter
    juce::StringArray distTypes{ "Tape Warmth", "Tube Sheen", "Hard Clip", "Crystal Exciter" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kDistType, 1 }, "Exciter Type", distTypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDistDrive, 1 }, "Exciter Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.08f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDistTone, 1 }, "Exciter Sheen", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.15f,
        signedPercentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kDistMix, 1 }, "Exciter Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f,
        percentText));

    // Compressor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompThresh, 1 }, "Comp Threshold", juce::NormalisableRange<float>(-40.0f, 0.0f, 0.5f), -14.0f,
        dbText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompRatio, 1 }, "Comp Ratio", juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f), 3.5f,
        withText(formatRatio, parseNumber)));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompAttack, 1 }, "Comp Attack", juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.35f), 4.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompRelease, 1 }, "Comp Release", juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.35f), 65.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompMakeup, 1 }, "Comp Makeup", juce::NormalisableRange<float>(0.0f, 24.0f, 0.5f), 2.0f,
        dbText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kCompMix, 1 }, "Comp Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        percentText));

    // Limiter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kLimiterCeiling, 1 }, "Limiter Ceiling", juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.3f,
        dbText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kLimiterRelease, 1 }, "Limiter Release", juce::NormalisableRange<float>(5.0f, 100.0f, 0.5f), 20.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kLimiterEnabled, 1 }, "Limiter", true));

    // Trance Shimmer Reverb & Choke Gate
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbSize, 1 }, "Reverb Size", juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.55f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbDecay, 1 }, "Reverb Decay", juce::NormalisableRange<float>(50.0f, 2500.0f, 10.0f, 0.35f), 620.0f,
        msText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbDamp, 1 }, "Reverb Damping", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbGate, 1 }, "Reverb Gate", juce::NormalisableRange<float>(0.0f, 1500.0f, 5.0f), 320.0f,
        withText(formatGate, parseGate)));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverbMix, 1 }, "Reverb Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.24f,
        percentText));

    // Reverse & Sync
    juce::StringArray revModes{ "Off", "Beat End (Sync)", "Beat 4 Auto", "1/2 Beat", "1/4 Beat" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kReverseMode, 1 }, "Reverse Sync", revModes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kReverseMix, 1 }, "Reverse Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f,
        percentText));

    // Global / Master
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kLockIn, 1 }, "Lock In (WAV)", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kStereoWidth, 1 }, "Output Width", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.15f,
        percentText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kOutputGain, 1 }, "Output Gain", juce::NormalisableRange<float>(-36.0f, 12.0f, 0.5f), 0.0f,
        dbText));
    const auto makeNoteRange = [](int minMidi, int maxMidi) {
        const float minHz = eqNoteToHz(minMidi);
        const float maxHz = eqNoteToHz(maxMidi);
        return juce::NormalisableRange<float>(
            minHz,
            maxHz,
            [minMidi, maxMidi](float, float, float proportion) {
                const float clamped = std::clamp(proportion, 0.0f, 1.0f);
                const int midi = juce::roundToInt(static_cast<float>(minMidi)
                    + clamped * static_cast<float>(maxMidi - minMidi));
                return eqNoteToHz(std::clamp(midi, minMidi, maxMidi));
            },
            [minMidi, maxMidi](float, float, float value) {
                const int midi = eqHzToNearestMidi(value, minMidi, maxMidi);
                return static_cast<float>(midi - minMidi) / static_cast<float>(maxMidi - minMidi);
            },
            [minMidi, maxMidi](float, float, float value) {
                return eqNoteToHz(eqHzToNearestMidi(value, minMidi, maxMidi));
            });
    };
    const auto makeNoteText = [](int minMidi, int maxMidi) {
        return FloatAttributes()
            .withStringFromValueFunction([minMidi, maxMidi](float hz, int) { return formatEqNote(hz, minMidi, maxMidi); })
            .withValueFromStringFunction([minMidi, maxMidi](const juce::String& text) { return parseEqNoteOrHz(text, minMidi, maxMidi); });
    };
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqLowFreq, 1 }, "Low EQ Freq", makeNoteRange(kEqLowMinMidi, kEqLowMaxMidi), eqNoteToHz(40),
        makeNoteText(kEqLowMinMidi, kEqLowMaxMidi)));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqLowDb, 1 }, "Low EQ Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        dbText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqMidFreq, 1 }, "Mid EQ Freq", makeNoteRange(kEqMidMinMidi, kEqMidMaxMidi), eqNoteToHz(83),
        makeNoteText(kEqMidMinMidi, kEqMidMaxMidi)));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqMidDb, 1 }, "Mid EQ Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        dbText));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqHighFreq, 1 }, "High EQ Freq", makeNoteRange(kEqHighMinMidi, kEqHighMaxMidi), eqNoteToHz(118),
        makeNoteText(kEqHighMinMidi, kEqHighMaxMidi)));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ kEqHighDb, 1 }, "High EQ Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        dbText));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ kRollEnabled, 1 }, "Roll Enabled", false));
    juce::StringArray rollDivisions{ "1/8", "1/16", "1/32" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ kRollDivision, 1 }, "Roll Division", rollDivisions, 1));
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
    s.synth.velocityCurve = *apvts.getRawParameterValue(kVelocity);
    if (const auto* pBodyLevel = apvts.getRawParameterValue(kBodyLevel)) s.synth.bodyLevel = *pBodyLevel;
    if (const auto* pBodyTone = apvts.getRawParameterValue(kBodyTone)) s.synth.bodyTone = *pBodyTone;
    if (const auto* pBodyDecay = apvts.getRawParameterValue(kBodyDecay)) s.synth.bodyDecayMs = *pBodyDecay;

    if (const auto* pClickType = apvts.getRawParameterValue(kClickType)) {
        s.synth.chickType = static_cast<openhatter::core::ChickType>(static_cast<int>(*pClickType));
    }
    s.synth.chickLevel = *apvts.getRawParameterValue(kClickLevel);
    s.synth.chickFreqHz = *apvts.getRawParameterValue(kClickFreq);
    if (const auto* pKeyTracking = apvts.getRawParameterValue(kKeyTracking)) s.synth.keyTracking = *pKeyTracking;

    s.synth.reverbSize = *apvts.getRawParameterValue(kReverbSize);
    s.synth.reverbDecayMs = *apvts.getRawParameterValue(kReverbDecay);
    s.synth.reverbDamp = *apvts.getRawParameterValue(kReverbDamp);
    s.synth.reverbGateMs = *apvts.getRawParameterValue(kReverbGate);
    s.synth.reverbMix = *apvts.getRawParameterValue(kReverbMix);

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
    s.rollEnabled = *apvts.getRawParameterValue(kRollEnabled) > 0.5f;
    s.rollDivision = static_cast<int>(*apvts.getRawParameterValue(kRollDivision));

    return s;
}

} // namespace openhatter::params
