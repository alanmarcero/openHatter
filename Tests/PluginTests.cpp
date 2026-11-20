#include "Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Presets.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <new>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

namespace {
std::atomic<uint64_t> pluginCallbackAllocations { 0 };
thread_local bool countPluginCallbackAllocations = false;

void checkVisibleControls(juce::Component& component, juce::Component& editor,
                          std::vector<juce::Rectangle<int>>& targets) {
    for (auto* child : component.getChildren()) {
        if (!child->isVisible()) continue;
        if (auto* button = dynamic_cast<juce::TextButton*>(child)) {
            INFO(button->getButtonText());
            CHECK(button->getWidth() >= 44);
            CHECK(button->getHeight() >= 36);
            CHECK(button->getTooltip().isNotEmpty());
            const auto font = button->getLookAndFeel().getTextButtonFont(*button, button->getHeight());
            CHECK(font.getHeight() >= 14.0f);
            CHECK(juce::GlyphArrangement::getStringWidthInt(font, button->getButtonText()) <= button->getWidth() - 8);
        } else if (auto* box = dynamic_cast<juce::ComboBox*>(child)) {
            INFO(box->getName());
            CHECK(box->getHeight() >= 36);
            const auto font = box->getLookAndFeel().getComboBoxFont(*box);
            for (int item = 0; item < box->getNumItems(); ++item)
                CHECK(juce::GlyphArrangement::getStringWidthInt(font, box->getItemText(item)) <= box->getWidth() - 32);
        } else if (!dynamic_cast<juce::Slider*>(child)) {
            checkVisibleControls(*child, editor, targets);
            continue;
        }
        const auto bounds = editor.getLocalArea(child, child->getLocalBounds());
        REQUIRE(editor.getLocalBounds().contains(bounds));
        CHECK(std::none_of(targets.begin(), targets.end(), [&](const auto& other) { return other.intersects(bounds); }));
        targets.push_back(bounds);
    }
}
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
    REQUIRE(processor.parameters.getParameter(openhatter::params::kClickType) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kNoiseType) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kHissColor) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kHissDensity) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kHissDrive) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kBodyLevel) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kBodyTone) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kBodyDecay) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kKeyTracking) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kDistDrive) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kCompThresh) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kLimiterCeiling) != nullptr);
    REQUIRE(processor.parameters.getParameter(openhatter::params::kReverbMix) != nullptr);
}

TEST_CASE("OpenHatter decay morph and echo controls are automatable", "[plugin][parameters]") {
    OpenHatterProcessor processor;
    for (const auto* id : { openhatter::params::kDecayScale, openhatter::params::kEchoMix }) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        CHECK(parameter->isAutomatable());
    }
}

TEST_CASE("OpenHatter decay scale and tempo-synced echo shape output", "[plugin][dsp]") {
    OpenHatterProcessor dryProc, enhancedProc;
    dryProc.prepareToPlay(48000.0, 16000);
    enhancedProc.prepareToPlay(48000.0, 16000);
    for (const auto& [id, val] : { std::pair{openhatter::params::kDecayScale, 1.5f},
                                   {openhatter::params::kEchoMix, 0.8f} }) {
        auto* param = enhancedProc.parameters.getParameter(id);
        REQUIRE(param != nullptr);
        param->setValueNotifyingHost(param->convertTo0to1(val));
    }
    juce::AudioBuffer<float> dryBuf(2, 16000), enhancedBuf(2, 16000);
    juce::MidiBuffer midi1, midi2;
    midi1.addEvent(juce::MidiMessage::noteOn(1, 66, 1.0f), 0);
    midi2.addEvent(juce::MidiMessage::noteOn(1, 66, 1.0f), 0);
    dryProc.processBlock(dryBuf, midi1);
    enhancedProc.processBlock(enhancedBuf, midi2);
    float lateDry = 0.0f, lateEnhanced = 0.0f;
    for (int i = 8000; i < 16000; ++i) {
        lateDry += std::abs(dryBuf.getSample(0, i));
        lateEnhanced += std::abs(enhancedBuf.getSample(0, i));
    }
    REQUIRE(lateEnhanced > lateDry + 1.0f);
}

TEST_CASE("OpenHatter decay morph and echo map to render settings and survive state restore", "[plugin][parameters]") {
    OpenHatterProcessor processor;
    const std::array<std::pair<const char*, float>, 2> values {{
        { openhatter::params::kDecayScale, 1.6f },
        { openhatter::params::kEchoMix, 0.45f }
    }};
    for (const auto& [id, value] : values) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    const auto settings = openhatter::params::readSettings(processor.parameters);
    CHECK(std::abs(settings.synth.decayScale - 1.6f) < 0.011f);
    CHECK(std::abs(settings.effects.echoMix - 0.45f) < 0.011f);

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    OpenHatterProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    for (const auto& [id, value] : values)
        CHECK(std::abs(restored.parameters.getRawParameterValue(id)->load() - value) < 0.011f);
}

TEST_CASE("OpenHatter presets reset the envelope unless the envelope section is locked", "[plugin][presets][preset_lock]") {
    OpenHatterProcessor processor;
    const auto setValue = [&processor](const char* id, float value) {
        auto* parameter = processor.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue(openhatter::params::kAmpAttack, 14.0f);
    processor.setCurrentProgram(1);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kAmpAttack)->load() - 0.15f) < 0.06f);

    processor.setSectionLocked(OpenHatterProcessor::PresetSection::envelope, true);
    setValue(openhatter::params::kAmpAttack, 7.0f);
    processor.setCurrentProgram(2);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kAmpAttack)->load() - 7.0f) < 0.1f);
}

TEST_CASE("OpenHatter exposes the approved velocity, choke, and roll controls", "[plugin][performance]") {
    OpenHatterProcessor processor;
    const std::array<const char*, 3> velocityIds {
        "velocity_air", "velocity_chick", "velocity_reverb"
    };
    for (const auto* id : velocityIds)
        REQUIRE(processor.parameters.getParameter(id) != nullptr);

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
    setValue(openhatter::params::kNoiseLevel, 1.0f);
    setValue(openhatter::params::kNoiseHPF, 100.0f);
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
    setValue(openhatter::params::kNoiseLevel, 1.0f);
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
    REQUIRE(processor.getBusCount(false) == 4);
    REQUIRE(processor.getBus(false, 1)->getName() == "Air");
    REQUIRE(processor.getBus(false, 2)->getName() == "Chick");
    REQUIRE(processor.getBus(false, 3)->getName() == "Reverb");
}

TEST_CASE("OpenHatter routes audio to enabled stereo layer buses", "[plugin][outputs]") {
    OpenHatterProcessor processor;
    auto layout = processor.getBusesLayout();
    for (int bus = 1; bus < layout.outputBuses.size(); ++bus)
        layout.outputBuses.set(bus, juce::AudioChannelSet::stereo());
    REQUIRE(processor.setBusesLayout(layout));
    REQUIRE(processor.getTotalNumOutputChannels() == 8);
    processor.prepareToPlay(48000.0, 2048);

    juce::AudioBuffer<float> buffer(processor.getTotalNumOutputChannels(), 2048);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(110)), 0);
    processor.processBlock(buffer, midi);

    for (int busIndex = 1; busIndex < 4; ++busIndex) {
        auto stem = processor.getBusBuffer(buffer, false, busIndex);
        float stemPeak = 0.0f;
        for (int channel = 0; channel < stem.getNumChannels(); ++channel)
            for (int sample = 0; sample < stem.getNumSamples(); ++sample)
                stemPeak = std::max(stemPeak, std::abs(stem.getSample(channel, sample)));
        INFO(processor.getBus(false, busIndex)->getName().toStdString());
        REQUIRE(stemPeak > 0.001f);
    }
}

TEST_CASE("OpenHatter air velocity curve shapes the response independently of the MIDI note velocity", "[plugin][performance]") {
    const auto renderPeak = [](float velocityCurve) {
        OpenHatterProcessor processor;
        const auto setValue = [&processor](const char* id, float value) {
            auto* parameter = processor.parameters.getParameter(id);
            REQUIRE(parameter != nullptr);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        setValue("velocity_air", velocityCurve);
        setValue(openhatter::params::kNoiseLevel, 1.0f);
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

TEST_CASE("OpenHatterProcessor provides 56 EDM and trance factory presets", "[plugin]") {
    OpenHatterProcessor processor;
    REQUIRE(processor.getNumPrograms() == 56);

    for (int i = 0; i < 56; ++i) {
        processor.setCurrentProgram(i);
        REQUIRE(processor.getCurrentProgram() == i);
        REQUIRE(processor.getProgramName(i).isNotEmpty());
    }
}

TEST_CASE("OpenHatter factory presets retain a clear broadband air layer", "[plugin][presets]") {
    OpenHatterProcessor processor;
    for (int i = 0; i < processor.getNumPrograms(); ++i) {
        processor.setCurrentProgram(i);
        INFO(processor.getProgramName(i).toStdString());
        const float level = processor.parameters.getRawParameterValue(openhatter::params::kNoiseLevel)->load();
        REQUIRE(level >= 0.70f);
        REQUIRE(level <= 1.0f);
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
    proc1.setCurrentProgram(12);
    const auto setValue = [&proc1](const char* id, float value) {
        auto* parameter = proc1.parameters.getParameter(id);
        REQUIRE(parameter != nullptr);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setValue("midi_choke_note", 49.0f);
    setValue("decay_scale", 1.4f);
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
    REQUIRE(std::abs(proc2.parameters.getRawParameterValue("decay_scale")->load() - 1.4f) < 0.011f);
    REQUIRE(proc2.parameters.getRawParameterValue("roll_enabled")->load() > 0.5f);
    REQUIRE(proc2.parameters.getRawParameterValue(openhatter::params::kAmpEnabled)->load() > 0.5f);
    REQUIRE(std::abs(proc2.parameters.getRawParameterValue(openhatter::params::kAmpRelease)->load() - 20000.0f) < 0.1f);
}

TEST_CASE("Earlier banks keep their saved sound across a second save", "[plugin][presets]") {
    OpenHatterProcessor processor;
    auto legacy = processor.parameters.copyState();
    legacy.setProperty("preset_catalog_version", 3, nullptr);
    legacy.setProperty("section_lock_1", true, nullptr);
    for (auto child : legacy) {
        if (child.getProperty("id") == openhatter::params::kNoiseDecay)
            child.setProperty("value", 419.0f, nullptr);
        if (child.getProperty("id") == openhatter::params::kAmpEnabled)
            child.setProperty("value", 1.0f, nullptr);
    }
    juce::MemoryBlock bytes;
    juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), bytes);
    processor.setStateInformation(bytes.getData(), static_cast<int>(bytes.getSize()));
    CHECK(processor.hasLegacySound());
    processor.getStateInformation(bytes);
    OpenHatterProcessor restored;
    restored.setStateInformation(bytes.getData(), static_cast<int>(bytes.getSize()));
    CHECK(restored.hasLegacySound());
    CHECK(restored.isSectionLocked(OpenHatterProcessor::PresetSection::envelope));
    CHECK(std::abs(restored.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load() - 419.0f) < .01f);
    CHECK(restored.parameters.getRawParameterValue(openhatter::params::kAmpEnabled)->load() > .5f);
    restored.setCurrentProgram(0);
    CHECK_FALSE(restored.hasLegacySound());
}

TEST_CASE("The cymbal SVG follows playback, seeks, stop and the internal beat", "[plugin][gui][beat]") {
    struct Host : juce::AudioPlayHead {
        PositionInfo position;
        juce::Optional<PositionInfo> getPosition() const override { return position; }
    } host;
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000, 480);
    processor.setPlayHead(&host);
    juce::AudioBuffer<float> audio(2, 480);
    juce::MidiBuffer midi;
    const auto frame = [&](double ppq, double bpm, bool playing, float expected) {
        host.position.setPpqPosition(ppq);
        host.position.setBpm(bpm);
        host.position.setIsPlaying(playing);
        processor.processBlock(audio, midi);
        CHECK(std::abs(processor.displayBeatPhase.load() - expected) < .0001f);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        return editor->createComponentSnapshot({ 28, 196, 295, 146 });
    };
    const auto raised = frame(-2.52, 120.0, true, .50f);
    const auto closed = frame(14.24, 60.0, true, .25f);
    const auto stopped = frame(14.44, 90.0, false, 0.0f);
    const auto stoppedAfterSeek = frame(-5.72, 150.0, false, 0.0f);
    int changedPixels = 0, stoppedDifference = 0;
    for (int y = 0; y < raised.getHeight(); ++y) {
        for (int x = 0; x < raised.getWidth(); ++x) {
            changedPixels += raised.getPixelAt(x, y) != closed.getPixelAt(x, y);
            stoppedDifference += stopped.getPixelAt(x, y) != stoppedAfterSeek.getPixelAt(x, y);
        }
    }
    CHECK(changedPixels > 700);
    CHECK(stoppedDifference == 0);

    processor.setPlayHead(nullptr);
    OpenHatterProcessor unhosted;
    unhosted.prepareToPlay(48000, 480);
    unhosted.processBlock(audio, midi);
    CHECK(std::abs(unhosted.displayBeatPhase.load() - .023f) < .0001f);
}

TEST_CASE("Native editor browses, previews and edits held and one-shot hats", "[plugin][gui]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 66, static_cast<juce::uint8>(104)), 0);
    processor.processBlock(audio, midi);
    midi.clear();
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    const auto find = [&](auto&& self, juce::Component& parent, const juce::String& name) -> juce::Component* {
        for (auto* child : parent.getChildren()) {
            if (!child->isVisible()) continue;
            if (auto* button = dynamic_cast<juce::TextButton*>(child); button && button->getButtonText() == name) return child;
            if (child->getName() == name || child->getComponentID() == name) return child;
            if (auto* found = self(self, *child, name)) return found;
        }
        return nullptr;
    };
    const auto click = [&](const char* name) {
        auto* target = find(find, *editor, name);
        INFO(name);
        REQUIRE(dynamic_cast<juce::TextButton*>(target) != nullptr);
        const auto pos = target->getLocalBounds().getCentre().toFloat();
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(), pos,
            juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, target, target, now, pos, now, 1, false);
        target->mouseDown(event);
        target->mouseUp(event);
    };
    const auto capture = [&](const char* page) {
        std::vector<juce::Rectangle<int>> targets;
        checkVisibleControls(*editor, *editor, targets);
        const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        REQUIRE(image.isValid());
        if (const auto* directory = std::getenv("OPENHATTER_CAPTURE_DIR")) {
            auto file = juce::File(directory).getChildFile(juce::String("openHatter-") + page + ".png");
            REQUIRE(file.deleteFile());
            auto stream = file.createOutputStream();
            REQUIRE(stream != nullptr);
            REQUIRE(juce::PNGImageFormat{}.writeImageToStream(image, *stream));
        }
    };

    capture("hat");
    auto* presets = dynamic_cast<juce::ComboBox*>(find(find, *editor, "Factory preset"));
    REQUIRE(presets != nullptr);
    CHECK(presets->getRootMenu()->getNumItems() == 6);
    presets->setSelectedId(56, juce::sendNotificationSync);
    CHECK(processor.getCurrentProgram() == 55);
    click("Next");
    CHECK(processor.getCurrentProgram() == 0);
    click("Next");
    CHECK(processor.getCurrentProgram() == 1);
    click("Previous");
    CHECK(processor.getCurrentProgram() == 0);
    click("Preview hat");
    processor.processBlock(audio, midi);
    CHECK(audio.getMagnitude(0, 512) > .05f);
    CHECK(processor.displayTriggerCount.load() == 2);
    CHECK(find(find, *editor, openhatter::params::kNoiseHold) == nullptr);
    click("Detailed controls");
    CHECK(find(find, *editor, openhatter::params::kNoiseHold) != nullptr);
    capture("detail");
    click("Envelope");
    click("Hold notes");
    REQUIRE(processor.parameters.getRawParameterValue(openhatter::params::kAmpEnabled)->load() > .5f);
    for (auto [id, value] : { std::pair{ openhatter::params::kAmpAttack, 9.0f },
                              { openhatter::params::kAmpDecay, 160.0f }, { openhatter::params::kAmpSustain, .65f },
                              { openhatter::params::kAmpRelease, 20000.0f } }) {
        auto* slider = dynamic_cast<juce::Slider*>(find(find, *editor, id));
        REQUIRE(slider != nullptr);
        slider->setValue(value, juce::sendNotificationSync);
        CHECK(std::abs(processor.parameters.getRawParameterValue(id)->load() - value) < .01f);
    }
    capture("envelope");
    click("One-shot");
    CHECK(processor.parameters.getRawParameterValue(openhatter::params::kAmpEnabled)->load() < .5f);
    click("Performance");
    click("Roll off");
    CHECK(processor.parameters.getRawParameterValue(openhatter::params::kRollEnabled)->load() > .5f);
    capture("performance");
    click("Effects");
    click("Lock");
    CHECK(processor.isSectionLocked(OpenHatterProcessor::PresetSection::exciter));
    capture("effects");
}

TEST_CASE("OpenHatter ADSR points drag interactively and update envelope knobs and parameters", "[plugin][gui][adsr]") {
    OpenHatterProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    const auto findButton = [&](auto&& self, juce::Component& parent, const juce::String& text) -> juce::TextButton* {
        for (auto* child : parent.getChildren()) {
            if (auto* btn = dynamic_cast<juce::TextButton*>(child); btn && btn->getButtonText() == text) return btn;
            if (auto* nested = self(self, *child, text)) return nested;
        }
        return nullptr;
    };
    auto* envTab = static_cast<juce::Component*>(findButton(findButton, *editor, "Envelope"));
    REQUIRE(envTab != nullptr);
    const auto clickPos = envTab->getLocalBounds().getCentre().toFloat();
    const auto clickTime = juce::Time::getCurrentTime();
    const juce::MouseEvent tabDown(juce::Desktop::getInstance().getMainMouseSource(), clickPos,
        juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, envTab, envTab, clickTime, clickPos, clickTime, 1, false);
    envTab->mouseDown(tabDown);
    envTab->mouseUp(tabDown);

    openhatter::ui::EnvelopePanel* envPanel = nullptr;
    std::ranges::for_each(editor->getChildren(), [&](juce::Component* child) {
        if (auto* candidate = dynamic_cast<openhatter::ui::EnvelopePanel*>(child)) envPanel = candidate;
    });
    REQUIRE(envPanel != nullptr);
    REQUIRE(envPanel->isVisible());

    const auto plot = envPanel->getLocalBounds().reduced(40, 0).withTrimmedTop(114).withTrimmedBottom(200).toFloat();
    const auto getProp = [&](const char* id) {
        auto* p = processor.parameters.getParameter(id);
        REQUIRE(p != nullptr);
        return p->getValue();
    };
    const auto relA = getProp(openhatter::params::kAmpAttack);
    const auto relD = getProp(openhatter::params::kAmpDecay);
    const auto relS = getProp(openhatter::params::kAmpSustain);
    const auto relR = getProp(openhatter::params::kAmpRelease);

    const float attackX = plot.getX() + plot.getWidth() * (.04f + .20f * relA);
    const float releaseX = plot.getRight() - plot.getWidth() * (.08f + .20f * relR);
    const float sustainY = plot.getBottom() - plot.getHeight() * relS;

    const auto dragPoint = [&](juce::Point<float> from, juce::Point<float> to) {
        const auto now = juce::Time::getCurrentTime();
        const auto& mouse = juce::Desktop::getInstance().getMainMouseSource();
        envPanel->mouseDown(juce::MouseEvent(mouse, from, juce::ModifierKeys::leftButtonModifier,
                                             1, 0, 0, 0, 0, envPanel, envPanel, now, from, now, 1, false));
        envPanel->mouseDrag(juce::MouseEvent(mouse, to, juce::ModifierKeys::leftButtonModifier,
                                             1, 0, 0, 0, 0, envPanel, envPanel, now, from, now, 1, false));
        envPanel->mouseUp(juce::MouseEvent(mouse, to, juce::ModifierKeys{},
                                           1, 0, 0, 0, 0, envPanel, envPanel, now, from, now, 1, false));
    };

    dragPoint({ attackX, plot.getY() }, { plot.getX() + plot.getWidth() * .20f, plot.getY() });
    CHECK(getProp(openhatter::params::kAmpAttack) > 0.70f);

    const float newAttackX = plot.getX() + plot.getWidth() * (.04f + .20f * getProp(openhatter::params::kAmpAttack));
    dragPoint({ newAttackX + plot.getWidth() * (.10f + .20f * relD), sustainY },
              { newAttackX + plot.getWidth() * .26f, plot.getBottom() - plot.getHeight() * .35f });
    CHECK(getProp(openhatter::params::kAmpDecay) > 0.70f);
    CHECK(std::abs(getProp(openhatter::params::kAmpSustain) - 0.35f) < 0.05f);

    dragPoint({ releaseX, plot.getBottom() - plot.getHeight() * getProp(openhatter::params::kAmpSustain) },
              { plot.getRight() - plot.getWidth() * .24f, plot.getBottom() - plot.getHeight() * .55f });
    CHECK(getProp(openhatter::params::kAmpRelease) > 0.70f);
    CHECK(std::abs(getProp(openhatter::params::kAmpSustain) - 0.55f) < 0.05f);
}

TEST_CASE("All factory hats speak promptly with rich body and full tails", "[plugin][presets_audio]") {
    int upliftingCount = 0;
    int longHatCount = 0;
    int closedHatCount = 0;
    std::ranges::for_each(std::views::iota(0, 56), [&](int program) {
        OpenHatterProcessor processor;
        processor.setCurrentProgram(program);
        const auto name = processor.getProgramName(program);
        if (name.startsWith("Uplifting / ")) ++upliftingCount;
        const float noiseDecayMs = processor.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load();
        if (noiseDecayMs >= 420.0f) ++longHatCount;
        if (noiseDecayMs <= 120.0f) ++closedHatCount;

        processor.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 66, static_cast<juce::uint8>(100)), 0);
        const int totalBlocks = std::clamp(static_cast<int>(std::ceil(processor.getTailLengthSeconds() * 48000.0 / 512.0)) + 16,
                                           48, 260);
        float peak = 0, attackPeak = 0, tailPeak = 0;
        double activeEnergy = 0;
        std::ranges::for_each(std::views::iota(0, totalBlocks), [&](int block) {
            processor.processBlock(buffer, midi);
            midi.clear();
            std::ranges::for_each(std::views::iota(0, 2), [&](int ch) {
                std::ranges::for_each(std::views::iota(0, 512), [&](int i) {
                    const float value = buffer.getSample(ch, i);
                    REQUIRE(std::isfinite(value));
                    peak = std::max(peak, std::abs(value));
                    if (block == 0) attackPeak = std::max(attackPeak, std::abs(value));
                    if (block < 4) activeEnergy += value * value;
                    if (block >= totalBlocks - 6) tailPeak = std::max(tailPeak, std::abs(value));
                });
            });
        });
        // Loudness of the hit itself. A tight closed hat is quiet if this
        // window is stretched out to 170 ms, so the check stays on the attack.
        const double rms = std::sqrt(activeEnergy / (2 * 4 * 512));
        INFO(name.toStdString());
        CAPTURE(peak, attackPeak, rms, tailPeak, totalBlocks);
        CHECK(attackPeak >= .25f);
        CHECK(peak >= .35f);
        CHECK(peak <= 1.0f);
        CHECK(rms >= .055);
        CHECK(tailPeak < .01f);
    });
    REQUIRE(upliftingCount == 24);
    REQUIRE(longHatCount >= 24);
    REQUIRE(closedHatCount >= 6);
}

TEST_CASE("Factory presets are level-matched, distinct and free of DC or clipping", "[plugin][presets_audio]") {
    struct Fingerprint {
        std::array<double, 6> bands{};
        double length = 0.0;
        float peakDb = -100.0f;
    };
    std::vector<Fingerprint> prints;
    std::vector<std::string> names;
    prints.reserve(56);
    std::ranges::for_each(std::views::iota(0, 56), [&](int program) {
        OpenHatterProcessor processor;
        processor.setCurrentProgram(program);
        processor.prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> buffer(2, 256);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 66, static_cast<juce::uint8>(100)), 0);
        std::vector<float> mono;
        mono.reserve(48000 / 4);
        float peak = 0.0f;
        double sum = 0.0;
        const int blocks = 48; // 0.256 s, enough for the attack and a short hat
        for (int block = 0; block < blocks; ++block) {
            processor.processBlock(buffer, midi);
            midi.clear();
            for (int i = 0; i < 256; ++i) {
                const float left = buffer.getSample(0, i);
                const float right = buffer.getSample(1, i);
                REQUIRE(std::isfinite(left));
                REQUIRE(std::isfinite(right));
                peak = std::max(peak, std::max(std::abs(left), std::abs(right)));
                const float mid = 0.5f * (left + right);
                mono.push_back(mid);
                sum += mid;
            }
        }
        const float peakDb = 20.0f * std::log10(std::max(peak, 1.0e-8f));
        INFO(processor.getProgramName(program).toStdString());
        CHECK(peakDb > -8.0f);
        CHECK(peakDb < -1.2f);
        CHECK(std::abs(sum / static_cast<double>(mono.size())) < 0.0015);

        constexpr int nfft = 1024;
        const int start = 480; // 10 ms
        std::array<double, 6> bands{};
        for (int k = 1; k < nfft / 2; k += 2) {
            double re = 0.0;
            double im = 0.0;
            for (int n = 0; n < nfft; ++n) {
                const double angle = 2.0 * 3.14159265358979323846 * k * n / nfft;
                const float sample = mono[static_cast<size_t>(start + n)];
                re += sample * std::cos(angle);
                im -= sample * std::sin(angle);
            }
            const double power = re * re + im * im;
            const double hz = k * 48000.0 / nfft;
            const int band = hz < 1000.0 ? 0 : hz < 2500.0 ? 1 : hz < 5000.0 ? 2 : hz < 8000.0 ? 3 : hz < 12000.0 ? 4 : 5;
            bands[static_cast<size_t>(band)] += power;
        }
        const double total = std::accumulate(bands.begin(), bands.end(), 0.0) + 1.0e-12;
        for (double& band : bands) band /= total;
        double envPeak = 0.0;
        for (float sample : mono) envPeak = std::max(envPeak, static_cast<double>(std::abs(sample)));
        int last = 0;
        for (int i = 0; i < static_cast<int>(mono.size()); ++i)
            if (std::abs(mono[static_cast<size_t>(i)]) > envPeak * 0.03) last = i;
        prints.push_back(Fingerprint{ bands, std::log(std::max(0.03, last / 48000.0)), peakDb });
        names.push_back(processor.getProgramName(program).toStdString());
    });

    double worst = -1.0;
    size_t worstA = 0;
    size_t worstB = 1;
    for (size_t i = 0; i < prints.size(); ++i) {
        for (size_t j = i + 1; j < prints.size(); ++j) {
            constexpr double lengthWeight = 0.9;
            double dot = prints[i].length * lengthWeight * prints[j].length * lengthWeight;
            double ni = prints[i].length * lengthWeight * prints[i].length * lengthWeight;
            double nj = prints[j].length * lengthWeight * prints[j].length * lengthWeight;
            for (size_t b = 0; b < 6; ++b) {
                dot += prints[i].bands[b] * prints[j].bands[b];
                ni += prints[i].bands[b] * prints[i].bands[b];
                nj += prints[j].bands[b] * prints[j].bands[b];
            }
            const double similarity = dot / (std::sqrt(ni * nj) + 1.0e-12);
            if (similarity > worst) {
                worst = similarity;
                worstA = i;
                worstB = j;
            }
        }
    }
    // Copies land on 1. Two hats can share a hiss family and still differ in
    // length, stick and shell, so the bar sits just under a duplicate.
    INFO(names[worstA] + " ~ " + names[worstB]);
    CHECK(worst < 0.99995);
}

TEST_CASE("Most factory hats stay noisy instead of ringing like a bell", "[plugin][presets_audio][tone]") {
    // Peakiness is the strongest bin between 900 Hz and 12 kHz divided by the
    // median bin, measured on the tail. A sustaining partial stack scores high.
    // A quiet tail (the hat already finished) is not a bell.
    int ringing = 0;
    int noisy = 0;
    std::ranges::for_each(std::views::iota(0, 56), [&](int program) {
        OpenHatterProcessor processor;
        processor.setCurrentProgram(program);
        processor.prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> buffer(2, 256);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 66, static_cast<juce::uint8>(100)), 0);
        std::vector<float> mono;
        mono.reserve(8000);
        for (int block = 0; block < 32; ++block) {
            processor.processBlock(buffer, midi);
            midi.clear();
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                mono.push_back(0.5f * (buffer.getSample(0, i) + buffer.getSample(1, i)));
        }
        const int start = static_cast<int>(48000 * 0.05);
        const int end = static_cast<int>(48000 * 0.16);
        double energy = 0.0;
        for (int i = start; i < end; ++i)
            energy += static_cast<double>(mono[static_cast<size_t>(i)]) * mono[static_cast<size_t>(i)];
        const double rms = std::sqrt(energy / static_cast<double>(end - start));
        const auto name = processor.getProgramName(program).toStdString();
        if (20.0 * std::log10(rms + 1.0e-12) < -42.0) {
            ++noisy;
            return;
        }
        constexpr int nfft = 2048;
        const auto window = [](int n) {
            return 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979323846 * n / 2047.0);
        };
        std::vector<double> mag(nfft / 2 + 1, 0.0);
        int frames = 0;
        for (int pos = start; pos + nfft <= end; pos += nfft / 2) {
            for (int k = 0; k <= nfft / 2; ++k) {
                double re = 0.0;
                double im = 0.0;
                for (int n = 0; n < nfft; ++n) {
                    const double angle = 2.0 * 3.14159265358979323846 * k * n / nfft;
                    const double sample = mono[static_cast<size_t>(pos + n)] * window(n);
                    re += sample * std::cos(angle);
                    im -= sample * std::sin(angle);
                }
                mag[static_cast<size_t>(k)] += std::sqrt(re * re + im * im);
            }
            ++frames;
        }
        REQUIRE(frames > 0);
        std::vector<double> band;
        for (int k = 0; k <= nfft / 2; ++k) {
            const double hz = k * 48000.0 / nfft;
            if (hz >= 900.0 && hz <= 12000.0)
                band.push_back(mag[static_cast<size_t>(k)] / frames);
        }
        std::sort(band.begin(), band.end());
        const double median = band[band.size() / 2] + 1.0e-12;
        const double peak = *std::max_element(band.begin(), band.end());
        const double peakiness = peak / median;
        INFO(name + " peakiness " + std::to_string(peakiness));
        if (peakiness >= 14.0) ++ringing;
        else ++noisy;
    });
    // The two rides keep a bell. The rest of the bank should not.
    CHECK(ringing >= 2);
    CHECK(ringing <= 4);
    CHECK(noisy >= 50);
}

TEST_CASE("OpenHatter exposes 3-band EQ with glide and Output controls on the main page", "[plugin][ui][eq]") {
    openhatter::core::ThreeBandEq eqHigh, eqCut;
    eqHigh.prepare(48000.0);
    eqCut.prepare(48000.0);
    const openhatter::core::ThreeBandEqSettings boost{ 80.0f, 0.0f, 1000.0f, 0.0f, 4000.0f, 12.0f };
    const openhatter::core::ThreeBandEqSettings cut{ 80.0f, 0.0f, 1000.0f, 0.0f, 4000.0f, -12.0f };
    double rmsBoost = 0.0;
    double rmsCut = 0.0;
    std::ranges::for_each(std::views::iota(0, 4800), [&](int i) {
        const float x = 0.25f * std::sin(2.0f * 3.14159265f * 8000.0f * static_cast<float>(i) / 48000.0f);
        float bL = x;
        float bR = x;
        eqHigh.process(bL, bR, boost);
        float cL = x;
        float cR = x;
        eqCut.process(cL, cR, cut);
        if (i >= 2400) {
            rmsBoost += static_cast<double>(bL) * bL;
            rmsCut += static_cast<double>(cL) * cL;
        }
    });
    REQUIRE(10.0 * std::log10(rmsBoost / rmsCut) > 20.0);

    OpenHatterProcessor processor;
    std::ranges::for_each(std::array<const char*, 6>{
        "eq_low_freq", "eq_low_db", "eq_mid_freq", "eq_mid_db", "eq_high_freq", "eq_high_db"},
        [&](const char* id) { REQUIRE(processor.parameters.getParameter(id) != nullptr); });

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    auto* outputPanel = [&] {
        openhatter::ui::OutputPanel* found = nullptr;
        std::ranges::for_each(editor->getChildren(), [&](juce::Component* child) {
            if (auto* candidate = dynamic_cast<openhatter::ui::OutputPanel*>(child)) found = candidate;
        });
        return found;
    }();
    REQUIRE(outputPanel != nullptr);
    CHECK(outputPanel->isVisible());

    const auto findById = [](auto&& self, juce::Component& parent, const juce::String& id) -> juce::Component* {
        if (parent.getComponentID() == id) return &parent;
        for (auto* child : parent.getChildren()) {
            if (auto* match = self(self, *child, id)) return match;
        }
        return nullptr;
    };
    std::ranges::for_each(std::array<const char*, 8>{
        "eq_low_db", "eq_mid_db", "eq_high_db", "stereo_width",
        "output_gain", "limiter_ceiling", "limiter_release", "limiter_enabled"},
        [&](const char* id) {
            auto* control = findById(findById, *outputPanel, id);
            REQUIRE(control != nullptr);
            CHECK(control->isVisible());
        });

    auto* presets = [&] {
        juce::ComboBox* box = nullptr;
        std::ranges::for_each(editor->getChildren(), [&](juce::Component* child) {
            if (auto* candidate = dynamic_cast<juce::ComboBox*>(child); candidate && candidate->getName() == "Factory preset")
                box = candidate;
        });
        return box;
    }();
    REQUIRE(presets != nullptr);
}

namespace {

void flushParameters(OpenHatterProcessor& processor) { processor.parameters.copyState(); }

void gesture(juce::RangedAudioParameter& parameter, float plain) {
    parameter.beginChangeGesture();
    parameter.setValueNotifyingHost(parameter.convertTo0to1(plain));
    parameter.endChangeGesture();
}

} // namespace

TEST_CASE("Parameter edits undo and redo as grouped gestures", "[plugin][undo]") {
    OpenHatterProcessor processor;
    auto* gain = processor.parameters.getParameter(openhatter::params::kOutputGain);
    auto* decay = processor.parameters.getParameter(openhatter::params::kNoiseDecay);
    REQUIRE(gain != nullptr);
    REQUIRE(decay != nullptr);
    const float gain0 = processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load();
    const float decay0 = processor.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load();

    gain->beginChangeGesture();
    decay->beginChangeGesture();
    gain->setValueNotifyingHost(gain->convertTo0to1(4.0f));
    decay->setValueNotifyingHost(decay->convertTo0to1(400.0f));
    decay->endChangeGesture();
    gain->endChangeGesture();
    flushParameters(processor);

    gesture(*gain, -6.0f);
    flushParameters(processor);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() + 6.0f) < 0.05f);

    processor.undo();
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - 4.0f) < 0.05f);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load() - 400.0f) < 0.5f);
    processor.undo();
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - gain0) < 0.05f);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load() - decay0) < 0.5f);
    processor.redo();
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - 4.0f) < 0.05f);
    CHECK(processor.canRedo());
}

TEST_CASE("Editor shortcuts and buttons undo parameter changes", "[plugin][undo][gui]") {
    OpenHatterProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    auto* gain = processor.parameters.getParameter(openhatter::params::kOutputGain);
    REQUIRE(gain != nullptr);
    const float original = processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load();
    gesture(*gain, 5.0f);
    flushParameters(processor);
    REQUIRE(editor->keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 'z')));
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - original) < 0.05f);
    REQUIRE(editor->keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 'z')));
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - 5.0f) < 0.05f);

    const auto findButton = [&](auto&& self, juce::Component& parent, const juce::String& text) -> juce::TextButton* {
        for (auto* child : parent.getChildren()) {
            if (!child->isVisible()) continue;
            if (auto* button = dynamic_cast<juce::TextButton*>(child); button && button->getButtonText() == text)
                return button;
            if (auto* nested = self(self, *child, text)) return nested;
        }
        return nullptr;
    };
    const auto click = [](juce::Component& target) {
        const auto pos = target.getLocalBounds().getCentre().toFloat();
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(), pos,
            juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, &target, &target, now, pos, now, 1, false);
        target.mouseDown(event);
        target.mouseUp(event);
    };
    auto* undo = findButton(findButton, *editor, "Undo");
    REQUIRE(undo != nullptr);
    click(*undo);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - original) < 0.05f);
}

TEST_CASE("A/B compare restores full sound and section locks", "[plugin][ab]") {
    OpenHatterProcessor processor;
    auto* gain = processor.parameters.getParameter(openhatter::params::kOutputGain);
    REQUIRE(gain != nullptr);
    gain->setValueNotifyingHost(gain->convertTo0to1(3.5f));
    processor.setSectionLocked(OpenHatterProcessor::PresetSection::envelope, true);
    processor.copyAbToOther();
    gain->setValueNotifyingHost(gain->convertTo0to1(-8.0f));
    processor.setSectionLocked(OpenHatterProcessor::PresetSection::envelope, false);
    processor.selectAbSlot(1);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() - 3.5f) < 0.05f);
    CHECK(processor.isSectionLocked(OpenHatterProcessor::PresetSection::envelope));
    processor.selectAbSlot(0);
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kOutputGain)->load() + 8.0f) < 0.05f);
    CHECK_FALSE(processor.isSectionLocked(OpenHatterProcessor::PresetSection::envelope));
}

TEST_CASE("Drag export uses the locked hit and otherwise renders a fresh WAV", "[plugin][wav]") {
    OpenHatterProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    auto fresh = processor.exportHatWav();
    REQUIRE(fresh.getSize() > 44);
    CHECK(std::memcmp(fresh.getData(), "RIFF", 4) == 0);
    CHECK_FALSE(processor.isHatLockedIn());

    auto* decay = processor.parameters.getParameter(openhatter::params::kNoiseDecay);
    auto* lock = processor.parameters.getParameter(openhatter::params::kLockIn);
    REQUIRE(decay != nullptr);
    REQUIRE(lock != nullptr);
    const auto fingerprint = [](const juce::MemoryBlock& block) {
        uint32_t hash = 2166136261u;
        const auto* bytes = static_cast<const uint8_t*>(block.getData());
        for (size_t i = 0; i < block.getSize(); ++i)
            hash = (hash ^ bytes[i]) * 16777619u;
        return std::pair<size_t, uint32_t>{ block.getSize(), hash };
    };
    decay->setValueNotifyingHost(decay->convertTo0to1(80.0f));
    auto shorter = processor.exportHatWav();
    CHECK(shorter.getSize() > 44);
    CHECK(fingerprint(shorter) != fingerprint(fresh));

    lock->setValueNotifyingHost(1.0f);
    auto locked = processor.exportHatWav();
    CHECK(processor.isHatLockedIn());
    decay->setValueNotifyingHost(decay->convertTo0to1(900.0f));
    auto stillLocked = processor.exportHatWav();
    CHECK(fingerprint(stillLocked) == fingerprint(locked));

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    OpenHatterProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    CHECK(restored.isHatLockedIn());
    CHECK(fingerprint(restored.exportHatWav()) == fingerprint(locked));

    lock->setValueNotifyingHost(0.0f);
    auto unlocked = processor.exportHatWav();
    CHECK_FALSE(processor.isHatLockedIn());
    CHECK(unlocked.getSize() > 44);
    CHECK(fingerprint(unlocked) != fingerprint(locked));
}

TEST_CASE("User presets load through section locks and the editor size is part of the session", "[plugin][presets][resize]") {
    OpenHatterProcessor processor;
    processor.setSectionLocked(OpenHatterProcessor::PresetSection::air, true);
    auto* decay = processor.parameters.getParameter(openhatter::params::kNoiseDecay);
    REQUIRE(decay != nullptr);
    decay->setValueNotifyingHost(decay->convertTo0to1(180.0f));
    auto tree = processor.currentPresetTree();
    decay->setValueNotifyingHost(decay->convertTo0to1(900.0f));
    processor.loadUserPreset(tree, "Saved Hat");
    CHECK(processor.activeUserPreset() == "Saved Hat");
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load() - 900.0f) < 0.5f);

    processor.setSectionLocked(OpenHatterProcessor::PresetSection::air, false);
    processor.loadUserPreset(tree, "Saved Hat");
    CHECK(std::abs(processor.parameters.getRawParameterValue(openhatter::params::kNoiseDecay)->load() - 180.0f) < 0.5f);

    processor.setEditorSize(1480, 1120);
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    OpenHatterProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    CHECK(restored.editorSize() == juce::Point<int>(1480, 1120));
    CHECK(restored.activeUserPreset() == "Saved Hat");

    std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
    CHECK(editor->getWidth() == 1480);
    CHECK(editor->getHeight() == 1120);
    CHECK(editor->isResizable());
    REQUIRE(editor->getConstrainer() != nullptr);
    CHECK(editor->getConstrainer()->getFixedAspectRatio() == 0.0);
    CHECK(editor->getConstrainer()->getMinimumWidth() == OpenHatterProcessor::kEditorWidth);
    CHECK(editor->getConstrainer()->getMinimumHeight() == OpenHatterProcessor::kEditorHeight);
    CHECK(editor->getConstrainer()->getMaximumWidth() > editor->getConstrainer()->getMinimumWidth());

    editor->setSize(OpenHatterProcessor::kEditorWidth, OpenHatterProcessor::kEditorHeight);
    std::vector<juce::Rectangle<int>> compact;
    checkVisibleControls(*editor, *editor, compact);
    editor->setSize(1600, 1200);
    std::vector<juce::Rectangle<int>> wide;
    checkVisibleControls(*editor, *editor, wide);

    const auto findButton = [&](auto&& self, juce::Component& parent, const juce::String& text) -> juce::TextButton* {
        for (auto* child : parent.getChildren()) {
            if (!child->isVisible()) continue;
            if (auto* button = dynamic_cast<juce::TextButton*>(child); button && button->getButtonText() == text)
                return button;
            if (auto* nested = self(self, *child, text)) return nested;
        }
        return nullptr;
    };
    auto* presets = findButton(findButton, *editor, "Presets");
    REQUIRE(presets != nullptr);
    const auto click = [](juce::Component& target) {
        const auto pos = target.getLocalBounds().getCentre().toFloat();
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(), pos,
            juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, &target, &target, now, pos, now, 1, false);
        target.mouseDown(event);
        target.mouseUp(event);
    };
    click(*presets);
    std::vector<juce::Rectangle<int>> browser;
    checkVisibleControls(*editor, *editor, browser);
    auto* search = [&]() -> juce::TextEditor* {
        const auto walk = [&](auto&& self, juce::Component& parent) -> juce::TextEditor* {
            for (auto* child : parent.getChildren()) {
                if (!child->isVisible()) continue;
                if (auto* field = dynamic_cast<juce::TextEditor*>(child); field && field->getName() == "Preset search")
                    return field;
                if (auto* nested = self(self, *child)) return nested;
            }
            return nullptr;
        };
        return walk(walk, *editor);
    }();
    REQUIRE(search != nullptr);
    suite::PresetBrowser* presetBrowser = nullptr;
    const auto findBrowser = [&](auto&& self, juce::Component& parent) -> void {
        if (presetBrowser != nullptr) return;
        if (auto* candidate = dynamic_cast<suite::PresetBrowser*>(&parent)) presetBrowser = candidate;
        for (auto* child : parent.getChildren())
            if (child->isVisible()) self(self, *child);
    };
    findBrowser(findBrowser, *editor);
    REQUIRE(presetBrowser != nullptr);
    search->setText("Euphoria", false);
    presetBrowser->refresh();
    CHECK(presetBrowser->rowCount() >= 1);
    CHECK(presetBrowser->rowCount() < restored.getNumPrograms());
}
