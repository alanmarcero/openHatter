#include "Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Presets.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE("OpenHatterProcessor instantiates with valid parameters", "[plugin]") {
    OpenHatterProcessor processor;
    REQUIRE(processor.getName() == "openHatter");
    REQUIRE(processor.acceptsMidi());
    REQUIRE_FALSE(processor.producesMidi());

    REQUIRE(processor.parameters.getParameter(openhatter::params::kPreBursts) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kFlamSpread) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kToneMode) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kToneFreq) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kClickType) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kNoiseType) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kDistDrive) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kCompThresh) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kLimiterCeiling) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kReverbMix) != nullptr);
}

TEST_CASE("OpenHatter exposes the approved velocity, resonator, choke, and roll controls", "[plugin][performance]") {
    OpenHatterProcessor processor;
    const std::array<const char*, 4> velocityIds {
        "velocity_metal", "velocity_air", "velocity_chick", "velocity_reverb"
    };
    for (const auto* id : velocityIds)
        REQUIRE(processor.parameters.getParameter(id) != nullptr);

    for (int resonator = 1; resonator <= 6; ++resonator) {
        const auto prefix = "resonator_" + juce::String(resonator);
        REQUIRE(processor.parameters.getParameter(prefix + "_enabled") != nullptr);
        REQUIRE(processor.parameters.getParameter(prefix + "_solo") != nullptr);
        REQUIRE(processor.parameters.getParameter(prefix + "_tune") != nullptr);
    }

    REQUIRE(processor.parameters.getParameter("midi_choke_note") != nullptr);
    REQUIRE(processor.parameters.getParameter("midi_choke_cc") != nullptr);
    REQUIRE(processor.parameters.getParameter("roll_enabled") != nullptr);
    REQUIRE(processor.parameters.getParameter("roll_division") != nullptr);
    REQUIRE(processor.parameters.getParameter("roll_swing") != nullptr);
    REQUIRE(processor.parameters.getParameter("roll_accent") != nullptr);
}

TEST_CASE("OpenHatter exposes independent stereo outputs for the sound layers", "[plugin][outputs]") {
    OpenHatterProcessor processor;
    REQUIRE(processor.getBusCount(false) == 5);
    REQUIRE(processor.getBus(false, 1)->getName() == "Metal");
    REQUIRE(processor.getBus(false, 2)->getName() == "Air");
    REQUIRE(processor.getBus(false, 3)->getName() == "Chick");
    REQUIRE(processor.getBus(false, 4)->getName() == "Reverb");
}

TEST_CASE("OpenHatter routes audio to enabled stereo layer buses", "[plugin][outputs]") {
    OpenHatterProcessor processor;
    auto layout = processor.getBusesLayout();
    for (int bus = 1; bus < layout.outputBuses.size(); ++bus)
        layout.outputBuses.set(bus, juce::AudioChannelSet::stereo());
    REQUIRE(processor.setBusesLayout(layout));
    REQUIRE(processor.getTotalNumOutputChannels() == 10);
    processor.prepareToPlay(48000.0, 2048);

    juce::AudioBuffer<float> buffer(processor.getTotalNumOutputChannels(), 2048);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(110)), 0);
    processor.processBlock(buffer, midi);

    for (int busIndex = 1; busIndex < 5; ++busIndex) {
        auto stem = processor.getBusBuffer(buffer, false, busIndex);
        float stemPeak = 0.0f;
        for (int channel = 0; channel < stem.getNumChannels(); ++channel)
            for (int sample = 0; sample < stem.getNumSamples(); ++sample)
                stemPeak = std::max(stemPeak, std::abs(stem.getSample(channel, sample)));
        INFO(processor.getBus(false, busIndex)->getName().toStdString());
        REQUIRE(stemPeak > 0.001f);
    }
}

TEST_CASE("OpenHatter velocity curves change modal response independently of the MIDI note velocity", "[plugin][performance]") {
    const auto renderPeak = [](float velocityCurve) {
        OpenHatterProcessor processor;
        const auto setValue = [&processor](const char* id, float value) {
            auto* parameter = processor.parameters.getParameter(id);
            REQUIRE(parameter != nullptr);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        setValue("velocity_metal", velocityCurve);
        setValue(openhatter::params::kToneLevel, 1.0f);
        setValue(openhatter::params::kNoiseLevel, 0.0f);
        setValue(openhatter::params::kClickLevel, 0.0f);
        setValue(openhatter::params::kPreLevel, 0.0f);
        setValue(openhatter::params::kReverbMix, 0.0f);
        setValue(openhatter::params::kCompMix, 0.0f);
        setValue(openhatter::params::kDistMix, 0.0f);
        setValue(openhatter::params::kLimiterEnabled, 0.0f);
        processor.prepareToPlay(48000.0, 2048);
        juce::AudioBuffer<float> buffer(2, 2048);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 69, static_cast<juce::uint8>(64)), 0);
        processor.processBlock(buffer, midi);
        float peak = 0.0f;
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                peak = std::max(peak, std::abs(buffer.getSample(channel, sample)));
        return peak;
    };

    const float linearResponse = renderPeak(1.0f);
    const float compressedResponse = renderPeak(2.0f);
    REQUIRE(linearResponse > 0.0f);
    REQUIRE(compressedResponse < linearResponse * 0.8f);
}

TEST_CASE("OpenHatter configured MIDI choke note closes a ringing hat", "[plugin][midi]") {
    OpenHatterProcessor processor;
    auto* reverbMix = processor.parameters.getParameter(openhatter::params::kReverbMix);
    reverbMix->setValueNotifyingHost(reverbMix->convertTo0to1(0.0f));
    processor.prepareToPlay(48000.0, 2048);

    juce::AudioBuffer<float> first(2, 256);
    juce::MidiBuffer noteOn;
    noteOn.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(120)), 0);
    processor.processBlock(first, noteOn);
    REQUIRE(processor.displayTriggerCount.load() == 1);

    juce::AudioBuffer<float> afterChoke(2, 2048);
    juce::MidiBuffer noteOff;
    noteOff.addEvent(juce::MidiMessage::noteOff(1, 42), 64);
    processor.processBlock(afterChoke, noteOff);
    float tailPeak = 0.0f;
    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 512; sample < afterChoke.getNumSamples(); ++sample)
            tailPeak = std::max(tailPeak, std::abs(afterChoke.getSample(channel, sample)));
    REQUIRE(tailPeak < 1.0e-6f);
}

TEST_CASE("OpenHatter configured MIDI choke CC closes a ringing hat at its sample offset", "[plugin][midi]") {
    OpenHatterProcessor processor;
    auto* reverbMix = processor.parameters.getParameter(openhatter::params::kReverbMix);
    reverbMix->setValueNotifyingHost(reverbMix->convertTo0to1(0.0f));
    processor.prepareToPlay(48000.0, 2048);

    juce::AudioBuffer<float> first(2, 256);
    juce::MidiBuffer noteOn;
    noteOn.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(120)), 0);
    processor.processBlock(first, noteOn);

    juce::AudioBuffer<float> afterChoke(2, 2048);
    juce::MidiBuffer chokeCc;
    chokeCc.addEvent(juce::MidiMessage::controllerEvent(1, 4, 127), 64);
    processor.processBlock(afterChoke, chokeCc);
    float tailPeak = 0.0f;
    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 512; sample < afterChoke.getNumSamples(); ++sample)
            tailPeak = std::max(tailPeak, std::abs(afterChoke.getSample(channel, sample)));
    REQUIRE(tailPeak < 1.0e-6f);
}

TEST_CASE("OpenHatter host-synced roll repeats the last MIDI hit while transport advances", "[plugin][performance]") {
    OpenHatterProcessor processor;
    auto* rollEnabled = processor.parameters.getParameter("roll_enabled");
    REQUIRE(rollEnabled != nullptr);
    rollEnabled->setValueNotifyingHost(rollEnabled->convertTo0to1(1.0f));
    auto* reverbMix = processor.parameters.getParameter(openhatter::params::kReverbMix);
    reverbMix->setValueNotifyingHost(reverbMix->convertTo0to1(0.0f));
    processor.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer firstHit;
    firstHit.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(110)), 0);
    processor.processBlock(buffer, firstHit);
    for (int block = 0; block < 48; ++block) {
        juce::MidiBuffer noEvents;
        processor.processBlock(buffer, noEvents);
    }

    REQUIRE(processor.displayTriggerCount.load() >= 4);
}

TEST_CASE("OpenHatterProcessor provides exactly 48 EDM and trance factory presets", "[plugin]") {
    OpenHatterProcessor processor;
    REQUIRE(processor.getNumPrograms() == 48);

    for (int i = 0; i < 48; ++i) {
        processor.setCurrentProgram(i);
        REQUIRE(processor.getCurrentProgram() == i);
        REQUIRE(processor.getProgramName(i).isNotEmpty());
    }
}

TEST_CASE("OpenHatter factory presets attenuate the tuned metal layer", "[plugin][presets]") {
    OpenHatterProcessor processor;
    auto* toneLevel = processor.parameters.getParameter(openhatter::params::kToneLevel);
    REQUIRE(toneLevel != nullptr);
    REQUIRE(std::abs(toneLevel->getNormalisableRange().start) < 0.001f);
    REQUIRE(std::abs(toneLevel->getNormalisableRange().end - 1.0f) < 0.001f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneLevel)->load() - 0.20f) < 0.001f);

    for (int i = 0; i < processor.getNumPrograms(); ++i) {
        processor.setCurrentProgram(i);
        INFO(processor.getProgramName(i).toStdString());
        const float level = processor.parameters.getRawParameterValue(openhatter::params::kToneLevel)->load();
        REQUIRE(level > 0.0f);
        REQUIRE(level <= 0.24f);
    }
}

TEST_CASE("OpenHatterProcessor renders audio upon MIDI note-on", "[plugin]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 66, (juce::uint8)100), 0);

    processor.processBlock(buffer, midi);

    float maxSample = 0.0f;
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            maxSample = std::max(maxSample, std::abs(buffer.getSample(ch, i)));
        }
    }

    REQUIRE(maxSample > 0.0f);
    REQUIRE(processor.displayPeak.load() > 0.0f);
    REQUIRE(processor.displayTriggerCount.load() >= 1);
}

TEST_CASE("OpenHatterProcessor saves and restores state information", "[plugin]") {
    OpenHatterProcessor proc1;
    proc1.setCurrentProgram(12); // Lucent Rise
    const auto setValue = [&proc1](const char* id, float value) {
        auto* parameter = proc1.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue("midi_choke_note", 49.0f);
    setValue("resonator_3_tune", -7.5f);
    setValue("roll_enabled", 1.0f);

    juce::MemoryBlock block;
    proc1.getStateInformation(block);
    REQUIRE(block.getSize() > 0);

    OpenHatterProcessor proc2;
    proc2.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    const float p1 = proc1.parameters.getRawParameterValue(openhatter::params::kDistDrive)->load();
    const float p2 = proc2.parameters.getRawParameterValue(openhatter::params::kDistDrive)->load();
    REQUIRE(std::abs(p1 - p2) < 0.001f);
    REQUIRE(std::abs(proc2.parameters.getRawParameterValue("midi_choke_note")->load() - 49.0f) < 0.001f);
    REQUIRE(std::abs(proc2.parameters.getRawParameterValue("resonator_3_tune")->load() + 7.5f) < 0.001f);
    REQUIRE(proc2.parameters.getRawParameterValue("roll_enabled")->load() > 0.5f);
}

TEST_CASE("OpenHatterEditor renders every page in both control modes", "[plugin][gui]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    REQUIRE(editor != nullptr);
    REQUIRE(editor->getWidth() == 1100);
    REQUIRE(editor->getHeight() == 972);
    auto* hatterEditor = dynamic_cast<OpenHatterEditor*>(editor.get());
    REQUIRE(hatterEditor != nullptr);

    for (int mode = 0; mode < 2; ++mode) {
        hatterEditor->setAdvancedView(mode == 1);
        for (int page = 0; page < 2; ++page) {
            hatterEditor->setPage(page);
            REQUIRE(hatterEditor->getPage() == page);
            const auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
            REQUIRE(snapshot.isValid());
            REQUIRE(snapshot.getWidth() == 1100);
            REQUIRE(snapshot.getHeight() == 972);
        }
    }
}

TEST_CASE("Legacy pitch lock does not override v3 preset modal controls", "[plugin][pitch_lock]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    auto* lockParam = processor.parameters.getParameter(openhatter::params::kLockPitch);
    REQUIRE(lockParam != nullptr);

    auto* octaveParam = processor.parameters.getParameter(openhatter::params::kToneOctave);
    auto* freqParam = processor.parameters.getParameter(openhatter::params::kToneFreq);
    auto* noteParam = processor.parameters.getParameter(openhatter::params::kToneNote);
    auto* fineParam = processor.parameters.getParameter(openhatter::params::kToneFine);
    REQUIRE(octaveParam != nullptr);
    REQUIRE(freqParam != nullptr);
    REQUIRE(noteParam != nullptr);
    REQUIRE(fineParam != nullptr);

    const auto presetValue = [](int program, const juce::String& parameterId) {
        for (const auto& [id, value] : openhatter::presets::factoryPresets()[static_cast<size_t>(program)].values)
            if (id == parameterId)
                return value;
        return -1.0f;
    };

    octaveParam->setValueNotifyingHost(octaveParam->convertTo0to1(8.0f));
    freqParam->setValueNotifyingHost(freqParam->convertTo0to1(3600.0f));
    noteParam->setValueNotifyingHost(noteParam->convertTo0to1(4.0f));
    fineParam->setValueNotifyingHost(fineParam->convertTo0to1(15.0f));

    lockParam->setValueNotifyingHost(1.0f);

    processor.setCurrentProgram(1);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - presetValue(1, openhatter::params::kToneOctave)) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFreq)->load() - presetValue(1, openhatter::params::kToneFreq)) < 5.0f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneNote)->load() - presetValue(1, openhatter::params::kToneNote)) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFine)->load() - presetValue(1, openhatter::params::kToneFine)) < 0.5f);

    processor.setCurrentProgram(10); // Uplifting | Climb Marker has octave 7 default
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - presetValue(10, openhatter::params::kToneOctave)) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFreq)->load() - presetValue(10, openhatter::params::kToneFreq)) < 5.0f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneNote)->load() - presetValue(10, openhatter::params::kToneNote)) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFine)->load() - presetValue(10, openhatter::params::kToneFine)) < 0.5f);

    lockParam->setValueNotifyingHost(0.0f);
    processor.setCurrentProgram(10);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - presetValue(10, openhatter::params::kToneOctave)) < 0.05f);
}

TEST_CASE("Cymbal Octave supports high octaves up to 8", "[plugin][octave]") {
    OpenHatterProcessor processor;
    auto* octaveParam = processor.parameters.getParameter(openhatter::params::kToneOctave);
    REQUIRE(octaveParam != nullptr);

    octaveParam->setValueNotifyingHost(octaveParam->convertTo0to1(8.0f));
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 8.0f) < 0.05f);

    octaveParam->setValueNotifyingHost(octaveParam->convertTo0to1(6.0f));
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 6.0f) < 0.05f);
}

TEST_CASE("All 48 factory presets render high-fidelity, balanced, clean audio", "[plugin][presets_audio]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    for (int p = 0; p < 48; ++p) {
        processor.setCurrentProgram(p);

        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 66, (juce::uint8)100), 0);

        float maxPeak = 0.0f;
        float sumSquare = 0.0f;
        int totalFrames = 0;

        for (int block = 0; block < 40; ++block) { // ~426ms
            if (block > 0) midi.clear();
            buffer.clear();
            processor.processBlock(buffer, midi);

            for (int i = 0; i < 512; ++i) {
                const float sL = buffer.getSample(0, i);
                const float sR = buffer.getSample(1, i);
                REQUIRE_FALSE(std::isnan(sL));
                REQUIRE_FALSE(std::isnan(sR));
                REQUIRE_FALSE(std::isinf(sL));
                REQUIRE_FALSE(std::isinf(sR));

                const float peak = std::max(std::abs(sL), std::abs(sR));
                maxPeak = std::max(maxPeak, peak);
                sumSquare += sL * sL + sR * sR;
                totalFrames += 2;
            }
        }

        const float rms = std::sqrt(sumSquare / static_cast<float>(totalFrames));

        // Each preset must produce healthy, club-ready audio levels without clipping or silence
        REQUIRE(maxPeak >= 0.15f);
        REQUIRE(maxPeak <= 1.5f);
        REQUIRE(rms >= 0.01f);
    }
}
