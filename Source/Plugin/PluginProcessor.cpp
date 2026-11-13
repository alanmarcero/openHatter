#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"
#include "Presets.h"

OpenHatterProcessor::OpenHatterProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", openhatter::params::createParameterLayout()) {
    for (auto& lock : sectionLocks_) lock.store(false, std::memory_order_relaxed);
}

void OpenHatterProcessor::prepareToPlay(double sampleRate, int) {
    engine_.prepare(sampleRate);
}

bool OpenHatterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()) {
        return false;
    }
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void OpenHatterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        buffer.clear(ch, 0, numSamples);
    }

    if (numChannels < 2) {
        return;
    }

    openhatter::core::Transport transport;
    if (auto* playHead = getPlayHead()) {
        if (auto posOpt = playHead->getPosition()) {
            if (posOpt->getBpm()) {
                transport.bpm = *posOpt->getBpm();
                displayBpm.store(static_cast<float>(transport.bpm));
            }
            if (posOpt->getPpqPosition()) {
                transport.ppqPosition = *posOpt->getPpqPosition();
            }
            if (posOpt->getTimeSignature()) {
                transport.beatsPerBar = static_cast<double>(posOpt->getTimeSignature()->numerator);
                displayBeatsPerBar.store(static_cast<float>(transport.beatsPerBar));
            }
            transport.isPlaying = posOpt->getIsPlaying();
        }
    }

    std::vector<openhatter::core::MidiTrigger> triggers;
    std::vector<openhatter::core::MidiCcEvent> ccEvents;

    for (const auto meta : midi) {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) {
            const float vel = msg.getFloatVelocity();
            displayVelocity.store(vel, std::memory_order_relaxed);
            triggers.push_back({
                .sampleOffset = meta.samplePosition,
                .noteNumber = msg.getNoteNumber(),
                .velocity = vel,
                .audition = false
            });
        } else if (msg.isController()) {
            ccEvents.push_back({
                .sampleOffset = meta.samplePosition,
                .controller = msg.getControllerNumber(),
                .value = msg.getControllerValue()
            });
        }
    }

    if (auditionPending_.exchange(false)) {
        displayVelocity.store(1.0f, std::memory_order_relaxed);
        triggers.push_back({
            .sampleOffset = 0,
            .noteNumber = 66, // F#4
            .velocity = 1.0f,
            .audition = true
        });
    }

    const auto settings = openhatter::params::readSettings(parameters);
    float* channelPointers[2] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };

    engine_.process(settings, transport, triggers, channelPointers, 2, numSamples, ccEvents);

    displayPeak.store(engine_.currentPeak());
    displayBeatPhase.store(engine_.currentBeatPhase());
    displayHatProgress.store(engine_.currentHatProgress());
    displayTriggerCount.store(engine_.triggerSerial());
    displayLockedIn.store(engine_.isHatLocked());
}

juce::AudioProcessorEditor* OpenHatterProcessor::createEditor() {
    return new OpenHatterEditor(*this);
}

int OpenHatterProcessor::getNumPrograms() {
    return static_cast<int>(openhatter::presets::factoryPresets().size());
}

const juce::String OpenHatterProcessor::getProgramName(int index) {
    const auto& presets = openhatter::presets::factoryPresets();
    if (index >= 0 && index < static_cast<int>(presets.size())) {
        return presets[static_cast<size_t>(index)].name;
    }
    return {};
}

void OpenHatterProcessor::setCurrentProgram(int index) {
    const auto& presets = openhatter::presets::factoryPresets();
    if (index >= 0 && index < static_cast<int>(presets.size())) {
        currentProgram_ = index;
        const bool preservePitch = parameters.getRawParameterValue(openhatter::params::kLockPitch) != nullptr
            && *parameters.getRawParameterValue(openhatter::params::kLockPitch) > 0.5f;
        std::array<bool, static_cast<size_t>(PresetSection::count)> locks{};
        for (size_t i = 0; i < locks.size(); ++i)
            locks[i] = sectionLocks_[i].load(std::memory_order_relaxed);
        openhatter::presets::apply(presets[static_cast<size_t>(index)], parameters, locks, preservePitch);
        if (auto* param = parameters.getParameter(openhatter::params::kPreset)) {
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(index)));
        }
    }
}

void OpenHatterProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    state.setProperty("preset_catalog_version", 2, nullptr);
    for (size_t i = 0; i < sectionLocks_.size(); ++i)
        state.setProperty("section_lock_" + juce::String(i), sectionLocks_[i].load(std::memory_order_relaxed), nullptr);
    if (engine_.isHatLocked()) {
        const auto wav = engine_.wavCache().toWavBytes();
        if (!wav.empty()) {
            state.setProperty("locked_wav", juce::MemoryBlock(wav.data(), wav.size()).toBase64Encoding(), nullptr);
        }
    }
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OpenHatterProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xmlState);
        parameters.replaceState(state);
        const bool hasV2Catalog = static_cast<int>(state.getProperty("preset_catalog_version", 0)) >= 2;
        for (size_t i = 0; i < sectionLocks_.size(); ++i) {
            const auto property = "section_lock_" + juce::String(i);
            sectionLocks_[i].store(hasV2Catalog && static_cast<bool>(state.getProperty(property, false)),
                                   std::memory_order_relaxed);
        }
        if (auto* presetParam = parameters.getRawParameterValue(openhatter::params::kPreset))
            currentProgram_ = std::clamp(static_cast<int>(*presetParam), 0, getNumPrograms() - 1);
        if (state.hasProperty("locked_wav")) {
            juce::MemoryBlock block;
            block.fromBase64Encoding(state.getProperty("locked_wav").toString());
            const auto* bytePtr = static_cast<const uint8_t*>(block.getData());
            std::vector<uint8_t> wavBytes(bytePtr, bytePtr + block.getSize());
            engine_.restoreLockedWav(wavBytes);
            displayLockedIn.store(true);
        }
    }
}

bool OpenHatterProcessor::isSectionLocked(PresetSection section) const noexcept {
    const auto index = static_cast<size_t>(section);
    return index < sectionLocks_.size() && sectionLocks_[index].load(std::memory_order_relaxed);
}

void OpenHatterProcessor::setSectionLocked(PresetSection section, bool locked) noexcept {
    const auto index = static_cast<size_t>(section);
    if (index < sectionLocks_.size()) sectionLocks_[index].store(locked, std::memory_order_relaxed);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OpenHatterProcessor();
}
