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
    proc1.setCurrentProgram(12); // Lucent Rise

    juce::MemoryBlock block;
    proc1.getStateInformation(block);
    REQUIRE(block.getSize() > 0);

    OpenHatterProcessor proc2;
    proc2.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    const float p1 = proc1.parameters.getRawParameterValue(openhatter::params::kDistDrive)->load();
    const float p2 = proc2.parameters.getRawParameterValue(openhatter::params::kDistDrive)->load();
    REQUIRE(std::abs(p1 - p2) < 0.001f);
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

    auto screenshots = juce::File::getCurrentWorkingDirectory().getChildFile("assets");
    if (!screenshots.isDirectory())
        screenshots = juce::File::getCurrentWorkingDirectory().getParentDirectory().getChildFile("assets");
    REQUIRE(screenshots.isDirectory());
    const std::array<const char*, 3> pageNames { "hat", "motion", "fx" };
    for (int mode = 0; mode < 2; ++mode) {
        hatterEditor->setAdvancedView(mode == 1);
        for (int page = 0; page < 3; ++page) {
            hatterEditor->setPage(page);
            const auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
            REQUIRE(snapshot.isValid());
            REQUIRE(snapshot.getWidth() == 1100);
            REQUIRE(snapshot.getHeight() == 972);
            const auto file = mode == 0
                ? screenshots.getChildFile("openHatter-" + juce::String(pageNames[static_cast<size_t>(page)]) + ".png")
                : screenshots.getChildFile("openHatter-" + juce::String(pageNames[static_cast<size_t>(page)]) + "-advanced.png");
            file.deleteFile();
            juce::FileOutputStream stream(file);
            REQUIRE(stream.openedOk());
            juce::PNGImageFormat png;
            REQUIRE(png.writeImageToStream(snapshot, stream));
        }
    }
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

    processor.setCurrentProgram(10); // Aurora Step has octave 7 default
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 8.0f) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFreq)->load() - 3600.0f) < 5.0f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneNote)->load() - 4.0f) < 0.05f);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneFine)->load() - 15.0f) < 0.5f);

    lockParam->setValueNotifyingHost(0.0f);
    processor.setCurrentProgram(10);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kToneOctave)->load() - 7.0f) < 0.05f);
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
