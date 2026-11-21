#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>
#include <vector>

namespace suite {

// Keeps one undo history per A/B slot. Each gesture or preset load stores the state from before it.
class ParameterUndo : private juce::AudioProcessorParameter::Listener {
public:
    ParameterUndo(juce::AudioProcessor& processor, std::function<juce::ValueTree()> capture,
                  std::function<void(const juce::ValueTree&)> restore, std::function<size_t()> activeSlot);
    ~ParameterUndo() override;

    // Records a preset load or another non-gesture change as one undo step.
    void beginChange();
    void endChange();

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    void clearSlot(int slot);
    void clearAll();

private:
    struct History {
        std::vector<juce::ValueTree> undo;
        std::vector<juce::ValueTree> redo;
    };

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    juce::AudioProcessor& processor_;
    std::function<juce::ValueTree()> capture_;
    std::function<void(const juce::ValueTree&)> restore_;
    std::function<size_t()> activeSlot_;
    std::array<History, 2> histories_{};
    int openChanges_ = 0;
    juce::ValueTree before_;
};

} // namespace suite
