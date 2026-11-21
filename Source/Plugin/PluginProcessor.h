#pragma once

#include "AbCompare.h"
#include "HatEngine.h"
#include "ParameterUndo.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

class OpenHatterProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::AsyncUpdater {
public:
    enum class PresetSection : size_t { articulation, envelope, air, exciter, dynamics, spaceAndOutput, output, count };

    // The editor lays out at this design size and scales from 70% to 200%.
    static constexpr int kEditorWidth = 1060;
    static constexpr int kEditorHeight = 904;
    static constexpr int kEditorMinWidth = kEditorWidth * 7 / 10;
    static constexpr int kEditorMinHeight = kEditorHeight * 7 / 10;
    static constexpr int kEditorMaxWidth = kEditorWidth * 2;
    static constexpr int kEditorMaxHeight = kEditorHeight * 2;

    OpenHatterProcessor();
    ~OpenHatterProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "openHatter"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram_.load(); }
    bool hasLegacySound() const noexcept { return legacySound_.load(); }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void triggerAudition() noexcept { auditionPending_.store(true); }
    [[nodiscard]] bool isHatLockedIn() const noexcept { return displayLockedIn.load(); }
    [[nodiscard]] bool isSectionLocked(PresetSection section) const noexcept;
    void setSectionLocked(PresetSection section, bool locked) noexcept;

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] juce::UndoManager& undoManager() noexcept { return undoManager_; }

    [[nodiscard]] int abSlot() const noexcept { return ab_.active(); }
    void selectAbSlot(int slot);
    void copyAbToOther();

    void setEditorSize(int width, int height) noexcept;
    [[nodiscard]] juce::Point<int> editorSize() const noexcept { return { editorWidth_, editorHeight_ }; }

    // Locked render when Lock In is on, otherwise a fresh offline hit that is not stored.
    [[nodiscard]] juce::MemoryBlock exportHatWav();
    void serviceLockIn();

    [[nodiscard]] juce::String activeUserPreset() const;
    void loadUserPreset(const juce::ValueTree& state, const juce::String& name);
    void markPresetSaved(const juce::String& name);
    [[nodiscard]] juce::ValueTree currentPresetTree();
    // The loaded preset's name, with " *" after an edit. Empty for a sound from an earlier bank.
    [[nodiscard]] juce::String presetDisplayName();

    juce::UndoManager undoManager_;
    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float> displayPeak{ 0.0f };
    std::atomic<float> displayBeatPhase{ -1.0f };
    std::atomic<float> displayBpm{ 138.0f };
    std::atomic<float> displayBeatsPerBar{ 4.0f };
    std::atomic<float> displayHatProgress{ 1.0f };
    std::atomic<float> displayVelocity{ 1.0f };
    std::atomic<int> displayMidiNoteNumber{ 60 };
    std::atomic<float> displayMidiFrequency{ 261.625565f };
    std::atomic<uint32_t> displayTriggerCount{ 0 };
    std::atomic<bool> displayMidiNoteReceived{ false };
    std::atomic<bool> displayLockedIn{ false };

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void flushUndo();
    void applyBinaryState(const void* data, int sizeInBytes, bool loadSessionExtras);
    [[nodiscard]] juce::MemoryBlock captureSlotState();
    void restoreSlotState(const juce::MemoryBlock& block);
    void setUserPresetName(const juce::String& name);
    [[nodiscard]] std::vector<uint8_t> renderOfflineWav() const;
    void storeLockedWav(std::vector<uint8_t> wav);
    void clearLockedWav();
    [[nodiscard]] std::vector<uint8_t> copyLockedWav() const;
    [[nodiscard]] std::array<bool, static_cast<size_t>(PresetSection::count)> sectionLockSnapshot() const;

    static constexpr std::size_t kMidiEventCapacityPerBlock = 2048;
    std::unique_ptr<suite::ParameterUndo> parameterUndo_;
    suite::AbCompare ab_;
    openhatter::core::HatEngine engine_;
    std::array<openhatter::core::MidiTrigger, kMidiEventCapacityPerBlock> midiTriggers_{};
    std::array<openhatter::core::MidiCcEvent, kMidiEventCapacityPerBlock> midiCcEvents_{};
    std::atomic<bool> auditionPending_{ false };
    std::array<std::atomic<bool>, static_cast<size_t>(PresetSection::count)> sectionLocks_{};
    std::atomic<int> currentProgram_{ 0 };
    std::atomic<bool> legacySound_{ false };
    std::atomic<bool> presetEdited_{ false };
    std::atomic<bool> restoring_{ false };
    std::atomic<bool> lockWasOn_{ false };
    std::atomic<bool> lockRenderPending_{ false };
    std::atomic<bool> clearLockRequested_{ false };
    double sampleRate_ = 0.0;
    int editorWidth_ = kEditorWidth;
    int editorHeight_ = kEditorHeight;
    mutable std::mutex wavMutex_;
    std::vector<uint8_t> lockedWav_;
};
