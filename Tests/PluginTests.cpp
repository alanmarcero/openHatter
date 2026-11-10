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

TEST_CASE("OpenHatterProcessor provides exactly 48 EDM and trance factory presets", "[plugin]") {
    OpenHatterProcessor processor;
    REQUIRE(processor.getNumPrograms() == 48);

    for (int i = 0; i < 48; ++i) {
        processor.setCurrentProgram(i);
        REQUIRE(processor.getCurrentProgram() == i);
        REQUIRE(processor.getProgramName(i).isNotEmpty());
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
    proc1.setCurrentProgram(12); // Daniel Kandi 138 Euphoria

    juce::MemoryBlock block;
    proc1.getStateInformation(block);
    REQUIRE(block.getSize() > 0);

    OpenHatterProcessor proc2;
    proc2.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    const float p1 = proc1.parameters.getRawParameterValue(openhatter::params::kDistDrive)->load();
    const float p2 = proc2.parameters.getRawParameterValue(openhatter::params::kDistDrive)->load();
    REQUIRE(std::abs(p1 - p2) < 0.001f);
}

TEST_CASE("OpenHatterEditor instantiates with correct dimensions and captures live screenshots", "[plugin][gui]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    REQUIRE(editor != nullptr);
    REQUIRE(editor->getWidth() == 1100);
    REQUIRE(editor->getHeight() == 740);

    // 1. Capture Simple Mode screenshot
    juce::Image snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
    REQUIRE(snapshot.isValid());
    REQUIRE(snapshot.getWidth() == 1100);
    REQUIRE(snapshot.getHeight() == 740);

    juce::File repoScreenshot = juce::File::getCurrentWorkingDirectory().getChildFile("assets/openHatter-screenshot.png");
    if (repoScreenshot.getParentDirectory().exists()) {
        repoScreenshot.deleteFile();
        juce::FileOutputStream stream(repoScreenshot);
        if (stream.openedOk()) {
            juce::PNGImageFormat png;
            png.writeImageToStream(snapshot, stream);
        }
    }

    // 2. Capture Advanced Mode screenshot
    if (auto* hatterEditor = dynamic_cast<OpenHatterEditor*>(editor.get())) {
        processor.setCurrentProgram(12); // 13 Daniel Kandi 138 Euphoria
        hatterEditor->setAdvancedView(true);
        juce::Image advSnapshot = editor->createComponentSnapshot(editor->getLocalBounds());
        REQUIRE(advSnapshot.isValid());
        juce::File advFile = juce::File::getCurrentWorkingDirectory().getChildFile("assets/openHatter-advanced-screenshot.png");
        if (advFile.getParentDirectory().exists()) {
            advFile.deleteFile();
            juce::FileOutputStream stream(advFile);
            if (stream.openedOk()) {
                juce::PNGImageFormat png;
                png.writeImageToStream(advSnapshot, stream);
            }
        }
    }

    juce::File tempFile = juce::File::createTempFile(".png");
    {
        juce::FileOutputStream stream(tempFile);
        REQUIRE(stream.openedOk());
        juce::PNGImageFormat png;
        REQUIRE(png.writeImageToStream(snapshot, stream));
    }
    REQUIRE(tempFile.existsAsFile());
    REQUIRE(tempFile.getSize() > 0);
    tempFile.deleteFile();
}

TEST_CASE("Lock Pitch preserves note, octave, and fine pitch across preset changes", "[plugin][pitch_lock]") {
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

    octaveParam->setValueNotifyingHost(octaveParam->convertTo0to1(8.0f));
    freqParam->setValueNotifyingHost(freqParam->convertTo0to1(3600.0f));
    noteParam->setValueNotifyingHost(noteParam->convertTo0to1(4.0f));
    fineParam->setValueNotifyingHost(fineParam->convertTo0to1(15.0f));

    lockParam->setValueNotifyingHost(1.0f);

    processor.setCurrentProgram(1);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 8.0f) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFreq)->load() - 3600.0f) < 5.0f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneNote)->load() - 4.0f) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFine)->load() - 15.0f) < 0.5f);

    processor.setCurrentProgram(10); // Preset 11 has octave 5 default
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 8.0f) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFreq)->load() - 3600.0f) < 5.0f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneNote)->load() - 4.0f) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFine)->load() - 15.0f) < 0.5f);

    lockParam->setValueNotifyingHost(0.0f);
    processor.setCurrentProgram(10);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 5.0f) < 0.05f);
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
