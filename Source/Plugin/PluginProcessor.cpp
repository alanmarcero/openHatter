#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <ranges>
#include <string_view>

OpenHatterProcessor::OpenHatterProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withOutput("Air", juce::AudioChannelSet::stereo(), false)
          .withOutput("Chick", juce::AudioChannelSet::stereo(), false)
          .withOutput("Reverb", juce::AudioChannelSet::stereo(), false)),
      parameters(*this, nullptr, "PARAMETERS", openhatter::params::createParameterLayout()) {
    for (auto& lock : sectionLocks_) lock.store(false, std::memory_order_relaxed);
    setCurrentProgram(0);
}

void OpenHatterProcessor::prepareToPlay(double sampleRate, int) {
    engine_.prepare(sampleRate);
}

bool OpenHatterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()) {
        return false;
    }
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    for (int bus = 1; bus < layouts.outputBuses.size(); ++bus) {
        const auto& channels = layouts.outputBuses[bus];
        if (channels != juce::AudioChannelSet::disabled() && channels != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void OpenHatterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.clear(ch, 0, numSamples);
    }

    auto mainOutput = getBusBuffer(buffer, false, 0);
    if (mainOutput.getNumChannels() < 2) {
        return;
    }

    const auto settings = openhatter::params::readSettings(parameters);

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
            transport.isPlaying = posOpt->getIsPlaying()
                || (wrapperType == wrapperType_Standalone && !posOpt->getPpqPosition());
        }
    }

    std::size_t triggerCount = 0;
    std::size_t ccEventCount = 0;
    const auto appendTrigger = [this, &triggerCount](const openhatter::core::MidiTrigger& trigger) noexcept {
        if (triggerCount < midiTriggers_.size())
            midiTriggers_[triggerCount++] = trigger;
    };
    const auto appendCcEvent = [this, &ccEventCount](const openhatter::core::MidiCcEvent& event) noexcept {
        if (ccEventCount < midiCcEvents_.size())
            midiCcEvents_[ccEventCount++] = event;
    };
    int auditionNote = displayMidiNoteNumber.load(std::memory_order_relaxed);

    for (const auto meta : midi) {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) {
            const float vel = msg.getFloatVelocity();
            auditionNote = msg.getNoteNumber();
            displayMidiNoteNumber.store(auditionNote, std::memory_order_relaxed);
            displayMidiFrequency.store(openhatter::core::midiNoteToHz(auditionNote), std::memory_order_relaxed);
            displayMidiNoteReceived.store(true, std::memory_order_relaxed);
            displayVelocity.store(vel, std::memory_order_relaxed);
            appendTrigger({
                .sampleOffset = meta.samplePosition,
                .noteNumber = msg.getNoteNumber(),
                .velocity = vel,
                .audition = false,
                .midiChannel = msg.getChannel()
            });
        } else if (msg.isNoteOff() && msg.getNoteNumber() == settings.midiChokeNote) {
            appendTrigger({
                .sampleOffset = meta.samplePosition,
                .noteNumber = msg.getNoteNumber(),
                .velocity = 0.0f,
                .choke = true
            });
        } else if (msg.isNoteOff()) {
            appendTrigger({
                .sampleOffset = meta.samplePosition,
                .noteNumber = msg.getNoteNumber(),
                .midiChannel = msg.getChannel(),
                .noteOff = true
            });
        } else if (msg.isController()) {
            appendCcEvent({
                .sampleOffset = meta.samplePosition,
                .controller = msg.getControllerNumber(),
                .value = msg.getControllerValue()
            });
        }
    }

    if (auditionPending_.exchange(false)) {
        appendTrigger({
            .sampleOffset = 0,
            .noteNumber = auditionNote,
            .velocity = 1.0f,
            .audition = true
        });
    }

    float* channelPointers[2] = { mainOutput.getWritePointer(0), mainOutput.getWritePointer(1) };

    auto airOutput = getBusBuffer(buffer, false, 1);
    auto chickOutput = getBusBuffer(buffer, false, 2);
    auto reverbOutput = getBusBuffer(buffer, false, 3);
    openhatter::core::LayerOutputBuffers layerOutputs;
    if (airOutput.getNumChannels() >= 2) {
        layerOutputs.airLeft = airOutput.getWritePointer(0);
        layerOutputs.airRight = airOutput.getWritePointer(1);
    }
    if (chickOutput.getNumChannels() >= 2) {
        layerOutputs.chickLeft = chickOutput.getWritePointer(0);
        layerOutputs.chickRight = chickOutput.getWritePointer(1);
    }
    if (reverbOutput.getNumChannels() >= 2) {
        layerOutputs.reverbLeft = reverbOutput.getWritePointer(0);
        layerOutputs.reverbRight = reverbOutput.getWritePointer(1);
    }

    engine_.process(settings, transport,
                    std::span<const openhatter::core::MidiTrigger>(midiTriggers_.data(), triggerCount),
                    channelPointers, 2, numSamples,
                    std::span<const openhatter::core::MidiCcEvent>(midiCcEvents_.data(), ccEventCount),
                    &layerOutputs);

    displayPeak.store(engine_.currentPeak());
    displayBeatPhase.store(engine_.currentBeatPhase());
    displayHatProgress.store(engine_.currentHatProgress());
    displayTriggerCount.store(engine_.triggerSerial());
    displayLockedIn.store(false, std::memory_order_relaxed);
}

double OpenHatterProcessor::getTailLengthSeconds() const {
    const auto settings = openhatter::params::readSettings(parameters);
    const float scale = std::clamp(settings.synth.decayScale, 0.25f, 2.0f);
    const float airMs = settings.synth.noiseAttackMs + settings.synth.noiseHoldMs
        + settings.synth.noiseDecayMs * scale;
    const float bodyMs = settings.synth.bodyLevel > 0.001f
        ? settings.synth.noiseAttackMs + settings.synth.bodyDecayMs * scale
        : 0.0f;
    const float dryTailMs = std::max(airMs, bodyMs);
    const float synthTailMs = std::max({ dryTailMs, settings.synth.reverbDecayMs, settings.synth.reverbGateMs });
    const float releaseMs = settings.synth.fullAdsrEnabled
        ? std::clamp(settings.synth.ampReleaseMs, 0.0f, 20000.0f)
        : 0.0f;
    const float preRollMs = static_cast<float>(std::clamp(settings.synth.preChickCount, 1, 5) - 1)
        * std::max(0.0f, settings.synth.preChickSpreadMs);
    const float dynamicsTailMs = std::max(settings.effects.compressor.releaseMs, settings.effects.limiter.releaseMs);
    const float reverseTailMs = settings.effects.reverseMode == openhatter::core::ReverseSyncMode::Off ? 0.0f : 2000.0f;
    const float echoTailMs = settings.effects.echoMix > 0.001f ? 1200.0f : 0.0f;
    const float tailMs = releaseMs + preRollMs + synthTailMs + dynamicsTailMs + echoTailMs + 80.0f;
    return std::max(tailMs, reverseTailMs) * 0.001;
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
        legacySound_.store(false);
        std::array<bool, static_cast<size_t>(PresetSection::count)> locks{};
        for (size_t i = 0; i < locks.size(); ++i)
            locks[i] = sectionLocks_[i].load(std::memory_order_relaxed);
        openhatter::presets::apply(presets[static_cast<size_t>(index)], parameters, locks);
        if (auto* param = parameters.getParameter(openhatter::params::kPreset)) {
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(index)));
        }
    }
}

void OpenHatterProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    state.setProperty("preset_catalog_version", 6, nullptr);
    state.setProperty("legacy_sound", hasLegacySound(), nullptr);
    std::ranges::for_each(std::views::iota(size_t{0}, sectionLocks_.size()), [&](size_t i) {
        state.setProperty("section_lock_" + juce::String(i), sectionLocks_[i].load(std::memory_order_relaxed), nullptr);
    });
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OpenHatterProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr || !xmlState->hasTagName(parameters.state.getType())) return;
    auto state = juce::ValueTree::fromXml(*xmlState);
    state.removeProperty("locked_wav", nullptr);
    parameters.replaceState(state);
    const int catalog = static_cast<int>(state.getProperty("preset_catalog_version", 0));
    if (catalog < 6) {
        // Older v4.4 and earlier sessions had no bronze body or hiss macro params; default them neutral if absent.
        constexpr std::array<const char*, 6> kNewEngineParams{{
            openhatter::params::kHissColor, openhatter::params::kHissDensity, openhatter::params::kHissDrive,
            openhatter::params::kBodyLevel, openhatter::params::kBodyTone, openhatter::params::kBodyDecay
        }};
        std::ranges::for_each(kNewEngineParams, [&](const char* paramId) {
            if (state.getChildWithProperty("id", paramId).isValid()) return;
            if (auto* param = parameters.getParameter(paramId)) {
                const float neutral = std::string_view(paramId) == openhatter::params::kBodyDecay ? 150.0f : 0.0f;
                param->setValueNotifyingHost(param->convertTo0to1(neutral));
            }
        });
    }
    legacySound_.store(catalog != 6 || static_cast<bool>(state.getProperty("legacy_sound", false)));
    std::ranges::for_each(std::views::iota(size_t{0}, sectionLocks_.size()), [&](size_t i) {
        const auto property = "section_lock_" + juce::String(i);
        sectionLocks_[i].store(catalog >= 2 && static_cast<bool>(state.getProperty(property, false)),
                               std::memory_order_relaxed);
    });
    if (auto* presetParam = parameters.getRawParameterValue(openhatter::params::kPreset))
        currentProgram_ = std::clamp(static_cast<int>(*presetParam), 0, getNumPrograms() - 1);
    displayLockedIn.store(false, std::memory_order_relaxed);
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
