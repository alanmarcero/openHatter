#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

namespace suite {

// Groups AudioProcessorValueTreeState edits into one UndoManager transaction per gesture.
class ParameterUndo : private juce::AudioProcessorParameter::Listener {
public:
    ParameterUndo(juce::AudioProcessor& processor, juce::UndoManager& undo, std::function<void()> flush);
    ~ParameterUndo() override;

private:
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    juce::AudioProcessor& processor_;
    juce::UndoManager& undo_;
    std::function<void()> flush_;
    int openGestures_ = 0;
};

} // namespace suite
