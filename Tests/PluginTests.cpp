#include "Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Presets.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <utility>

namespace {
std::atomic<uint64_t> pluginCallbackAllocations { 0 };
thread_local bool countPluginCallbackAllocations = false;
}

void* operator new(std::size_t size) {
    if (countPluginCallbackAllocations)
        pluginCallbackAllocations.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    if (countPluginCallbackAllocations)
        pluginCallbackAllocations.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

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

TEST_CASE("OpenHatter metal spread, damping, and width controls are automatable", "[plugin][parameters]") {
    OpenHatterProcessor processor;
    for (const auto* id : { openhatter::params::kMetalSpread, openhatter::params::kMetalDampingSlope,
                            openhatter::params::kMetalStereoSpread }) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        CHECK(parameter->isAutomatable());
    }
}

TEST_CASE("OpenHatter metal controls map to render settings", "[plugin][parameters]") {
    OpenHatterProcessor processor;
    const std::array<std::pair<const char*, float>, 3> values {{
        { openhatter::params::kMetalSpread, 7.5f },
        { openhatter::params::kMetalDampingSlope, -0.4f },
        { openhatter::params::kMetalStereoSpread, 1.7f }
    }};
    for (const auto& [id, value] : values) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    const auto settings = openhatter::params::readSettings(processor.parameters);
    CHECK(std::abs(settings.synth.metalSpread - 7.5f) < 0.11f);
    CHECK(std::abs(settings.synth.metalDampingSlope + 0.4f) < 0.011f);
    CHECK(std::abs(settings.synth.metalStereoSpread - 1.7f) < 0.011f);

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    OpenHatterProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    for (const auto& [id, value] : values)
        CHECK(std::abs(restored.parameters.getRawParameterValue(id)->load() - value) < 0.11f);
}

TEST_CASE("OpenHatter presets reset metal controls unless the metal section is locked", "[plugin][presets][preset_lock]") {
    OpenHatterProcessor processor;
    const auto setValue = [&processor](const char* id, float value) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue(openhatter::params::kMetalSpread, 14.0f);
    setValue(openhatter::params::kMetalDampingSlope, 0.7f);
    setValue(openhatter::params::kMetalStereoSpread, 1.8f);
    processor.setCurrentProgram(1);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kMetalSpread)->load()) < 0.01f);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kMetalDampingSlope)->load()) < 0.01f);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kMetalStereoSpread)->load() - 1.0f) < 0.01f);

    processor.setSectionLocked(OpenHatterProcessor::PresetSection::cymbalAndTuning, true);
    setValue(openhatter::params::kMetalSpread, 7.0f);
    processor.setCurrentProgram(2);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kMetalSpread)->load() - 7.0f) < 0.1f);
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

TEST_CASE("OpenHatter exposes opt-in Full ADSR with a 20-second release range", "[plugin][midi]") {
    OpenHatterProcessor processor;
    auto* enabled = processor.parameters.getParameter("amp_enabled");
    auto* release = processor.parameters.getParameter(openhatter::params::kAmpRelease);
    REQUIRE(enabled != nullptr);
    REQUIRE(release != nullptr);
    REQUIRE(std::abs(processor.parameters.getRawParameterValue("amp_enabled")->load()) < 0.001f);
    REQUIRE(std::abs(release->convertFrom0to1(0.0f)) < 0.001f);
    REQUIRE(std::abs(release->convertFrom0to1(1.0f) - 20000.0f) < 0.1f);
}

TEST_CASE("Full ADSR applies matching note-off at its exact MIDI sample offset", "[plugin][midi]") {
    OpenHatterProcessor processor;
    const auto setValue = [&processor](const char* id, float value) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue("amp_enabled", 1.0f);
    setValue(openhatter::params::kAmpAttack, 0.05f);
    setValue(openhatter::params::kAmpDecay, 1.0f);
    setValue(openhatter::params::kAmpSustain, 1.0f);
    setValue(openhatter::params::kAmpRelease, 0.0f);
    setValue(openhatter::params::kReverbMix, 0.0f);
    setValue(openhatter::params::kDistMix, 0.0f);
    setValue(openhatter::params::kCompMix, 0.0f);
    setValue(openhatter::params::kLimiterEnabled, 0.0f);
    processor.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(110)), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 46), 128);
    processor.processBlock(buffer, midi);

    const auto* left = buffer.getReadPointer(0);
    const auto beforeNoteOff = std::max_element(left, left + 128, [](float lhs, float rhs) {
        return std::abs(lhs) < std::abs(rhs);
    });
    const auto afterNoteOff = std::max_element(left + 128, left + 512, [](float lhs, float rhs) {
        return std::abs(lhs) < std::abs(rhs);
    });
    REQUIRE(std::abs(*beforeNoteOff) > 0.001f);
    REQUIRE(std::abs(*afterNoteOff) < 1.0e-8f);
}

TEST_CASE("Full ADSR note-off matches both MIDI note and channel", "[plugin][midi]") {
    OpenHatterProcessor processor;
    OpenHatterProcessor reference;
    const auto setValue = [](OpenHatterProcessor& target, const char* id, float value) {
        auto* parameter = target.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue(processor, "amp_enabled", 1.0f);
    setValue(processor, openhatter::params::kAmpAttack, 0.05f);
    setValue(processor, openhatter::params::kAmpDecay, 1.0f);
    setValue(processor, openhatter::params::kAmpSustain, 1.0f);
    setValue(processor, openhatter::params::kAmpRelease, 0.0f);
    setValue(processor, openhatter::params::kReverbMix, 0.0f);
    setValue(processor, openhatter::params::kDistMix, 0.0f);
    setValue(processor, openhatter::params::kCompMix, 0.0f);
    setValue(processor, openhatter::params::kLimiterEnabled, 0.0f);
    setValue(reference, "amp_enabled", 1.0f);
    setValue(reference, openhatter::params::kAmpAttack, 0.05f);
    setValue(reference, openhatter::params::kAmpDecay, 1.0f);
    setValue(reference, openhatter::params::kAmpSustain, 1.0f);
    setValue(reference, openhatter::params::kAmpRelease, 0.0f);
    setValue(reference, openhatter::params::kReverbMix, 0.0f);
    setValue(reference, openhatter::params::kDistMix, 0.0f);
    setValue(reference, openhatter::params::kCompMix, 0.0f);
    setValue(reference, openhatter::params::kLimiterEnabled, 0.0f);
    processor.prepareToPlay(48000.0, 512);
    reference.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::AudioBuffer<float> referenceBuffer(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);
    midi.addEvent(juce::MidiMessage::noteOn(2, 60, static_cast<juce::uint8>(110)), 64);
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 128);
    juce::MidiBuffer referenceMidi;
    referenceMidi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);
    referenceMidi.addEvent(juce::MidiMessage::noteOn(2, 60, static_cast<juce::uint8>(110)), 64);
    processor.processBlock(buffer, midi);
    reference.processBlock(referenceBuffer, referenceMidi);

    REQUIRE(std::equal(buffer.getReadPointer(0) + 128, buffer.getReadPointer(0) + 512,
                       referenceBuffer.getReadPointer(0) + 128));
    REQUIRE(std::equal(buffer.getReadPointer(1) + 128, buffer.getReadPointer(1) + 512,
                       referenceBuffer.getReadPointer(1) + 128));
}

TEST_CASE("Full ADSR holds through sustain and completes a 20-second release", "[plugin][midi]") {
    OpenHatterProcessor processor;
    const auto setValue = [&processor](const char* id, float value) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue("amp_enabled", 1.0f);
    setValue(openhatter::params::kAmpAttack, 0.05f);
    setValue(openhatter::params::kAmpDecay, 1.0f);
    setValue(openhatter::params::kAmpSustain, 1.0f);
    setValue(openhatter::params::kAmpRelease, 20000.0f);
    setValue(openhatter::params::kToneLevel, 1.0f);
    setValue(openhatter::params::kNoiseLevel, 0.0f);
    setValue(openhatter::params::kClickLevel, 0.0f);
    setValue(openhatter::params::kPreLevel, 0.0f);
    setValue(openhatter::params::kReverbMix, 0.0f);
    setValue(openhatter::params::kDistMix, 0.0f);
    setValue(openhatter::params::kCompMix, 0.0f);
    setValue(openhatter::params::kLimiterEnabled, 0.0f);
    processor.prepareToPlay(1000.0, 22000);

    juce::AudioBuffer<float> buffer(2, 22000);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, static_cast<juce::uint8>(110)), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 69), 100);
    processor.processBlock(buffer, midi);

    const auto* left = buffer.getReadPointer(0);
    const auto duringRelease = std::max_element(left + 10000, left + 10200, [](float lhs, float rhs) {
        return std::abs(lhs) < std::abs(rhs);
    });
    const auto afterReleaseAndTail = std::max_element(left + 21000, left + 22000, [](float lhs, float rhs) {
        return std::abs(lhs) < std::abs(rhs);
    });
    REQUIRE(std::abs(*duringRelease) > 0.001f);
    REQUIRE(std::abs(*afterReleaseAndTail) < 1.0e-8f);
    REQUIRE(processor.getTailLengthSeconds() > 20.0);
}

TEST_CASE("MIDI note-off leaves the default one-shot output unchanged", "[plugin][midi]") {
    OpenHatterProcessor withNoteOff;
    OpenHatterProcessor withoutNoteOff;
    withNoteOff.prepareToPlay(48000.0, 256);
    withoutNoteOff.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> firstWith(2, 256);
    juce::AudioBuffer<float> firstWithout(2, 256);
    juce::MidiBuffer noteOn;
    noteOn.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(110)), 0);
    withNoteOff.processBlock(firstWith, noteOn);
    withoutNoteOff.processBlock(firstWithout, noteOn);

    juce::AudioBuffer<float> secondWith(2, 512);
    juce::AudioBuffer<float> secondWithout(2, 512);
    juce::MidiBuffer noteOff;
    noteOff.addEvent(juce::MidiMessage::noteOff(1, 46), 64);
    juce::MidiBuffer noMidi;
    withNoteOff.processBlock(secondWith, noteOff);
    withoutNoteOff.processBlock(secondWithout, noMidi);

    REQUIRE(std::equal(secondWith.getReadPointer(0), secondWith.getReadPointer(0) + 512,
                       secondWithout.getReadPointer(0)));
    REQUIRE(std::equal(secondWith.getReadPointer(1), secondWith.getReadPointer(1) + 512,
                       secondWithout.getReadPointer(1)));
}

TEST_CASE("Full ADSR audition remains a bounded one-shot preview", "[plugin][midi]") {
    OpenHatterProcessor processor;
    const auto setValue = [&processor](const char* id, float value) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue(openhatter::params::kAmpEnabled, 1.0f);
    setValue(openhatter::params::kAmpAttack, 1.0f);
    setValue(openhatter::params::kAmpDecay, 1.0f);
    setValue(openhatter::params::kAmpSustain, 1.0f);
    setValue(openhatter::params::kAmpRelease, 20000.0f);
    setValue(openhatter::params::kToneLevel, 1.0f);
    setValue(openhatter::params::kNoiseLevel, 0.0f);
    setValue(openhatter::params::kClickLevel, 0.0f);
    setValue(openhatter::params::kPreLevel, 0.0f);
    setValue(openhatter::params::kReverbMix, 0.0f);
    setValue(openhatter::params::kDistMix, 0.0f);
    setValue(openhatter::params::kCompMix, 0.0f);
    setValue(openhatter::params::kLimiterEnabled, 0.0f);
    setValue(openhatter::params::kRollEnabled, 1.0f);
    processor.prepareToPlay(48000.0, 96000);
    processor.triggerAudition();

    juce::AudioBuffer<float> buffer(2, 96000);
    juce::MidiBuffer noMidi;
    processor.processBlock(buffer, noMidi);

    float initialPeak = 0.0f;
    float latePeak = 0.0f;
    for (int sample = 0; sample < 128; ++sample)
        initialPeak = std::max(initialPeak, std::abs(buffer.getSample(0, sample)));
    for (int sample = 48000; sample < buffer.getNumSamples(); ++sample)
        latePeak = std::max(latePeak, std::abs(buffer.getSample(0, sample)));

    REQUIRE(initialPeak > 0.001f);
    REQUIRE(latePeak < 1.0e-8f);
}

TEST_CASE("MIDI note-off queueing adds no audio callback allocations", "[plugin][midi][realtime]") {
    OpenHatterProcessor processor;
    auto* adsr = processor.parameters.getParameter(openhatter::params::kAmpEnabled);
    REQUIRE(adsr != nullptr);
    adsr->setValueNotifyingHost(adsr->convertTo0to1(1.0f));
    processor.prepareToPlay(48000.0, 128);

    juce::AudioBuffer<float> startBuffer(2, 128);
    juce::MidiBuffer noteOn;
    noteOn.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(110)), 0);
    processor.processBlock(startBuffer, noteOn);

    juce::AudioBuffer<float> noteOffBuffer(2, 128);
    juce::AudioBuffer<float> noMidiBuffer(2, 128);
    juce::MidiBuffer noMidi;
    juce::MidiBuffer noteOff;
    noteOff.addEvent(juce::MidiMessage::noteOff(1, 46), 0);

    pluginCallbackAllocations.store(0, std::memory_order_relaxed);
    countPluginCallbackAllocations = true;
    processor.processBlock(noMidiBuffer, noMidi);
    countPluginCallbackAllocations = false;
    const auto noMidiAllocations = pluginCallbackAllocations.load(std::memory_order_relaxed);

    pluginCallbackAllocations.store(0, std::memory_order_relaxed);
    countPluginCallbackAllocations = true;
    processor.processBlock(noteOffBuffer, noteOff);
    countPluginCallbackAllocations = false;
    const auto noteOffAllocations = pluginCallbackAllocations.load(std::memory_order_relaxed);

    REQUIRE(noteOffAllocations == noMidiAllocations);
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
    setValue(openhatter::params::kAmpEnabled, 1.0f);
    setValue(openhatter::params::kAmpRelease, 20000.0f);

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
    REQUIRE(proc2.parameters.getRawParameterValue(openhatter::params::kAmpEnabled)->load() > 0.5f);
    REQUIRE(std::abs(proc2.parameters.getRawParameterValue(openhatter::params::kAmpRelease)->load() - 20000.0f) < 0.1f);
}

TEST_CASE("OpenHatterEditor renders every page in both control modes", "[plugin][gui]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    if (std::getenv("OPENHATTER_PERFORMANCE_CAPTURE") != nullptr) {
        const auto setCaptureValue = [&processor](const char* id, float value) {
            auto* parameter = processor.parameters.getParameter(id);
            REQUIRE(parameter != nullptr);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        setCaptureValue(openhatter::params::kRollEnabled, 1.0f);
        setCaptureValue(openhatter::params::kRollDivision, 2.0f);
        setCaptureValue(openhatter::params::kRollSwing, 0.12f);
        setCaptureValue(openhatter::params::kRollAccent, 0.5f);
    }

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

            const char* capturePath = nullptr;
            if (mode == 1 && page == 0)
                capturePath = std::getenv("OPENHATTER_SOUND_CAPTURE");
            if (mode == 1 && page == 1)
                capturePath = std::getenv("OPENHATTER_PERFORMANCE_CAPTURE");
            if (capturePath != nullptr) {
                juce::File outputFile(capturePath);
                REQUIRE(outputFile.getParentDirectory().createDirectory().wasOk());
                if (outputFile.exists())
                    REQUIRE(outputFile.deleteFile());
                auto output = outputFile.createOutputStream();
                REQUIRE(output != nullptr);
                juce::PNGImageFormat png;
                REQUIRE(png.writeImageToStream(snapshot, *output));
                output->flush();
            }
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
