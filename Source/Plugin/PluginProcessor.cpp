#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <algorithm>
#include <ranges>
#include <string_view>

namespace {

void stripSlotExtras(juce::ValueTree& tree) {
    for (const char* key : { "ab_a", "ab_b", "ab_active", "editor_width", "editor_height" })
        tree.removeProperty(key, nullptr);
}

} // namespace

OpenHatterProcessor::OpenHatterProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withOutput("Air", juce::AudioChannelSet::stereo(), false)
          .withOutput("Chick", juce::AudioChannelSet::stereo(), false)
          .withOutput("Reverb", juce::AudioChannelSet::stereo(), false)),
      parameters(*this, nullptr, "PARAMETERS", openhatter::params::createParameterLayout()) {
    for (auto& lock : sectionLocks_) lock.store(false, std::memory_order_relaxed);
    parameterUndo_ = std::make_unique<suite::ParameterUndo>(*this,
        [this] {
            auto state = parameters.copyState();
            state.setProperty("legacy_sound", hasLegacySound(), nullptr);
            state.setProperty("preset_edited", presetEdited_.load(), nullptr);
            return state;
        },
        [this](const juce::ValueTree& state) {
            parameters.replaceState(state.createCopy());
            legacySound_.store(static_cast<bool>(state.getProperty("legacy_sound")));
            presetEdited_.store(static_cast<bool>(state.getProperty("preset_edited")));
            if (auto* presetParam = parameters.getRawParameterValue(openhatter::params::kPreset))
                currentProgram_ = std::clamp(static_cast<int>(*presetParam), 0, getNumPrograms() - 1);
        },
        [this] { return static_cast<size_t>(ab_.active()); });
    setCurrentProgram(0);
    std::ranges::for_each(getParameters(), [this](juce::AudioProcessorParameter* parameter) {
        parameters.addParameterListener(dynamic_cast<juce::RangedAudioParameter*>(parameter)->getParameterID(), this);
    });
    parameterUndo_->clearAll();
    ab_.prime(captureSlotState());
}

OpenHatterProcessor::~OpenHatterProcessor() {
    cancelPendingUpdate();
    std::ranges::for_each(getParameters(), [this](juce::AudioProcessorParameter* parameter) {
        parameters.removeParameterListener(dynamic_cast<juce::RangedAudioParameter*>(parameter)->getParameterID(), this);
    });
}

void OpenHatterProcessor::prepareToPlay(double sampleRate, int) {
    sampleRate_ = sampleRate;
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
            displayNoteCount.fetch_add(1);
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
            // Hosts send CC 120 and up on stop, so Learn skips them.
            if (msg.getControllerNumber() < 120) {
                displayLastController.store(msg.getControllerNumber());
                displayControllerCount.fetch_add(1);
            }
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

    // The meter reads with exchange, so keep the loudest block since its last read.
    const float blockPeak = engine_.currentPeak();
    float shownPeak = displayPeak.load();
    while (blockPeak > shownPeak && !displayPeak.compare_exchange_weak(shownPeak, blockPeak)) {}
    displayBeatPhase.store(engine_.currentBeatPhase());
    displayHatProgress.store(engine_.currentHatProgress());
    displayTriggerCount.store(engine_.triggerSerial());
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
        parameterUndo_->beginChange();
        openhatter::presets::apply(presets[static_cast<size_t>(index)], parameters, sectionLockSnapshot());
        if (auto* param = parameters.getParameter(openhatter::params::kPreset))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(index)));
        setLoadedPreset({});
        parameterUndo_->endChange();
    }
}

void OpenHatterProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto* manager = juce::MessageManager::getInstanceWithoutCreating();
    if (manager == nullptr || manager->isThisTheMessageThread())
        serviceLockIn();
    ab_.storeActive(captureSlotState());
    auto state = parameters.copyState();
    stripSlotExtras(state);
    state.setProperty("preset_catalog_version", 7, nullptr);
    state.setProperty("legacy_sound", hasLegacySound(), nullptr);
    state.setProperty("preset_edited", presetEdited_.load(), nullptr);
    std::ranges::for_each(std::views::iota(size_t{0}, sectionLocks_.size()), [&](size_t i) {
        state.setProperty("section_lock_" + juce::String(i), sectionLocks_[i].load(std::memory_order_relaxed), nullptr);
    });
    const auto wav = copyLockedWav();
    if (!wav.empty())
        state.setProperty("locked_wav", juce::MemoryBlock(wav.data(), wav.size()).toBase64Encoding(), nullptr);
    state.setProperty("editor_width", editorWidth_, nullptr);
    state.setProperty("editor_height", editorHeight_, nullptr);
    state.setProperty("ab_active", ab_.active(), nullptr);
    state.setProperty("ab_a", ab_.slot(0).toBase64Encoding(), nullptr);
    state.setProperty("ab_b", ab_.slot(1).toBase64Encoding(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OpenHatterProcessor::setStateInformation(const void* data, int sizeInBytes) {
    applyBinaryState(data, sizeInBytes, true);
    parameterUndo_->clearAll();
}

bool OpenHatterProcessor::isSectionLocked(PresetSection section) const noexcept {
    const auto index = static_cast<size_t>(section);
    return index < sectionLocks_.size() && sectionLocks_[index].load(std::memory_order_relaxed);
}

void OpenHatterProcessor::setSectionLocked(PresetSection section, bool locked) noexcept {
    const auto index = static_cast<size_t>(section);
    if (index < sectionLocks_.size()) sectionLocks_[index].store(locked, std::memory_order_relaxed);
}

std::array<bool, static_cast<size_t>(OpenHatterProcessor::PresetSection::count)>
OpenHatterProcessor::sectionLockSnapshot() const {
    std::array<bool, static_cast<size_t>(PresetSection::count)> locks{};
    for (size_t i = 0; i < locks.size(); ++i)
        locks[i] = sectionLocks_[i].load(std::memory_order_relaxed);
    return locks;
}

void OpenHatterProcessor::undo() { parameterUndo_->undo(); }

void OpenHatterProcessor::redo() { parameterUndo_->redo(); }

bool OpenHatterProcessor::canUndo() const { return parameterUndo_->canUndo(); }

bool OpenHatterProcessor::canRedo() const { return parameterUndo_->canRedo(); }

void OpenHatterProcessor::selectAbSlot(int slot) {
    slot = slot == 1 ? 1 : 0;
    if (slot == ab_.active())
        return;
    ab_.storeActive(captureSlotState());
    ab_.setActive(slot);
    restoreSlotState(ab_.slot(slot));
}

void OpenHatterProcessor::copyAbToOther() {
    ab_.storeActive(captureSlotState());
    ab_.copyActiveToInactive();
    parameterUndo_->clearSlot(1 - ab_.active());
}

void OpenHatterProcessor::setEditorSize(int width, int height) noexcept {
    if (width < kEditorMinWidth || height < kEditorMinHeight)
        return;
    const auto fitted = fitEditorSize(width, height);
    editorWidth_ = fitted.x;
    editorHeight_ = fitted.y;
}

juce::Point<int> OpenHatterProcessor::fitEditorSize(int width, int height) noexcept {
    const double designWidth = kEditorWidth;
    const double scale = std::clamp(std::min(width / designWidth, height / static_cast<double>(kEditorHeight)),
                                    kEditorMinWidth / designWidth, kEditorMaxWidth / designWidth);
    return { juce::roundToInt(kEditorWidth * scale), juce::roundToInt(kEditorHeight * scale) };
}

juce::MemoryBlock OpenHatterProcessor::exportHatWav() {
    if (clearLockRequested_.exchange(false))
        clearLockedWav();
    serviceLockIn();
    auto wav = copyLockedWav();
    if (wav.empty())
        wav = renderOfflineWav();
    return { wav.data(), wav.size() };
}

void OpenHatterProcessor::serviceLockIn() {
    if (parameters.getRawParameterValue(openhatter::params::kLockIn)->load() <= 0.5f) {
        lockRenderPending_.store(false);
        return;
    }
    if (!lockRenderPending_.exchange(false))
        return;
    storeLockedWav(renderOfflineWav());
}

juce::String OpenHatterProcessor::activeUserPreset() const {
    return parameters.state.getProperty("user_preset").toString();
}

void OpenHatterProcessor::loadUserPreset(const juce::ValueTree& state, const juce::String& name) {
    std::vector<std::pair<juce::String, float>> values;
    for (const auto& child : state) {
        const auto id = child.getProperty("id").toString();
        if (id.isEmpty() || id == openhatter::params::kPreset || id == openhatter::params::kLockIn)
            continue;
        values.emplace_back(id, static_cast<float>(child.getProperty("value")));
    }
    parameterUndo_->beginChange();
    openhatter::presets::applyValues(values, parameters, sectionLockSnapshot());
    setLoadedPreset(name);
    parameterUndo_->endChange();
}

void OpenHatterProcessor::setLoadedPreset(const juce::String& name) {
    legacySound_.store(false);
    parameters.state.setProperty("user_preset", name, nullptr);
    presetEdited_.store(false);
}

juce::String OpenHatterProcessor::presetDisplayName() {
    if (hasLegacySound())
        return {};
    auto name = activeUserPreset();
    if (name.isEmpty())
        name = getProgramName(getCurrentProgram());
    if (presetEdited_.load())
        name << " *";
    return name;
}

juce::ValueTree OpenHatterProcessor::currentPresetTree() {
    auto tree = parameters.copyState();
    stripSlotExtras(tree);
    std::ranges::for_each(std::array{ "locked_wav", "preset_catalog_version", "legacy_sound", "user_preset", "preset_edited" },
        [&tree](const char* key) { tree.removeProperty(key, nullptr); });
    for (size_t i = 0; i < sectionLocks_.size(); ++i)
        tree.removeProperty("section_lock_" + juce::String(i), nullptr);
    std::vector<juce::ValueTree> drop;
    for (const auto& child : tree) {
        const auto id = child.getProperty("id").toString();
        if (id == openhatter::params::kPreset || id == openhatter::params::kLockIn)
            drop.push_back(child);
    }
    for (const auto& child : drop)
        tree.removeChild(child, nullptr);
    return tree;
}

void OpenHatterProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (restoring_ || parameterID == openhatter::params::kPreset)
        return;
    if (parameterID != openhatter::params::kLockIn) {
        presetEdited_.store(true);
        return;
    }
    const bool want = newValue > 0.5f;
    const bool was = lockWasOn_.exchange(want);
    if (want && !was)
        lockRenderPending_.store(true);
    if (!want && was)
        clearLockRequested_.store(true);
    triggerAsyncUpdate();
}

void OpenHatterProcessor::handleAsyncUpdate() {
    if (clearLockRequested_.exchange(false))
        clearLockedWav();
    serviceLockIn();
}

void OpenHatterProcessor::applyBinaryState(const void* data, int sizeInBytes, bool loadSessionExtras) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr || !xmlState->hasTagName(parameters.state.getType()))
        return;
    auto state = juce::ValueTree::fromXml(*xmlState);
    const int editorW = static_cast<int>(state.getProperty("editor_width", 0));
    const int editorH = static_cast<int>(state.getProperty("editor_height", 0));
    const int abActive = static_cast<int>(state.getProperty("ab_active", 0));
    const auto encodedA = state.getProperty("ab_a").toString();
    const auto encodedB = state.getProperty("ab_b").toString();
    juce::MemoryBlock wavBlock;
    const bool hadWav = state.hasProperty("locked_wav")
        && wavBlock.fromBase64Encoding(state.getProperty("locked_wav").toString());
    stripSlotExtras(state);
    state.removeProperty("locked_wav", nullptr);

    restoring_ = true;
    parameters.replaceState(state);
    restoring_ = false;

    const int catalog = static_cast<int>(state.getProperty("preset_catalog_version", 0));
    if (catalog < 6) {
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
    legacySound_.store(catalog != 7 || static_cast<bool>(state.getProperty("legacy_sound", false)));
    std::ranges::for_each(std::views::iota(size_t{0}, sectionLocks_.size()), [&](size_t i) {
        const auto property = "section_lock_" + juce::String(i);
        sectionLocks_[i].store(catalog >= 2 && static_cast<bool>(state.getProperty(property, false)),
                               std::memory_order_relaxed);
    });
    if (auto* presetParam = parameters.getRawParameterValue(openhatter::params::kPreset))
        currentProgram_ = std::clamp(static_cast<int>(*presetParam), 0, getNumPrograms() - 1);
    presetEdited_.store(static_cast<bool>(state.getProperty("preset_edited", false)));

    if (hadWav && wavBlock.getSize() > 0) {
        const auto* bytes = static_cast<const uint8_t*>(wavBlock.getData());
        storeLockedWav(std::vector<uint8_t>(bytes, bytes + wavBlock.getSize()));
        lockWasOn_.store(parameters.getRawParameterValue(openhatter::params::kLockIn)->load() > 0.5f);
        lockRenderPending_.store(false);
    } else if (parameters.getRawParameterValue(openhatter::params::kLockIn)->load() > 0.5f) {
        clearLockedWav();
        lockWasOn_.store(true);
        lockRenderPending_.store(true);
    } else {
        clearLockedWav();
        lockWasOn_.store(false);
        lockRenderPending_.store(false);
    }

    if (loadSessionExtras) {
        setEditorSize(editorW, editorH);
        juce::MemoryBlock slotA;
        juce::MemoryBlock slotB;
        if (encodedA.isNotEmpty() && encodedB.isNotEmpty() && slotA.fromBase64Encoding(encodedA)
            && slotB.fromBase64Encoding(encodedB) && slotA.getSize() > 0 && slotB.getSize() > 0)
            ab_.setSlots(abActive, std::move(slotA), std::move(slotB));
        else
            ab_.prime(captureSlotState());
    }
}

juce::MemoryBlock OpenHatterProcessor::captureSlotState() {
    auto state = parameters.copyState();
    stripSlotExtras(state);
    state.setProperty("preset_catalog_version", 7, nullptr);
    state.setProperty("legacy_sound", hasLegacySound(), nullptr);
    state.setProperty("preset_edited", presetEdited_.load(), nullptr);
    std::ranges::for_each(std::views::iota(size_t{0}, sectionLocks_.size()), [&](size_t i) {
        state.setProperty("section_lock_" + juce::String(i), sectionLocks_[i].load(std::memory_order_relaxed), nullptr);
    });
    const auto wav = copyLockedWav();
    if (!wav.empty())
        state.setProperty("locked_wav", juce::MemoryBlock(wav.data(), wav.size()).toBase64Encoding(), nullptr);
    else
        state.removeProperty("locked_wav", nullptr);
    juce::MemoryBlock block;
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, block);
    return block;
}

void OpenHatterProcessor::restoreSlotState(const juce::MemoryBlock& block) {
    if (block.getSize() == 0)
        return;
    const int width = editorWidth_;
    const int height = editorHeight_;
    const int active = ab_.active();
    auto slotA = ab_.slot(0);
    auto slotB = ab_.slot(1);
    applyBinaryState(block.getData(), static_cast<int>(block.getSize()), false);
    editorWidth_ = width;
    editorHeight_ = height;
    ab_.setSlots(active, std::move(slotA), std::move(slotB));
}

std::vector<uint8_t> OpenHatterProcessor::renderOfflineWav() const {
    openhatter::core::HatEngine offline;
    offline.prepare(sampleRate_ > 0.0 ? sampleRate_ : 48000.0);
    offline.lockCurrentHat(openhatter::params::readSettings(parameters));
    return offline.wavCache().toWavBytes();
}

void OpenHatterProcessor::storeLockedWav(std::vector<uint8_t> wav) {
    const bool locked = !wav.empty();
    {
        std::lock_guard<std::mutex> guard(wavMutex_);
        lockedWav_ = std::move(wav);
    }
    displayLockedIn.store(locked);
}

void OpenHatterProcessor::clearLockedWav() {
    {
        std::lock_guard<std::mutex> guard(wavMutex_);
        lockedWav_.clear();
    }
    displayLockedIn.store(false);
}

std::vector<uint8_t> OpenHatterProcessor::copyLockedWav() const {
    std::lock_guard<std::mutex> guard(wavMutex_);
    return lockedWav_;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OpenHatterProcessor();
}
