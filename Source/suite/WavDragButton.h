#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace suite {

// Starts an external file drag of a WAV produced by the callback.
class WavDragButton : public juce::TextButton {
public:
    explicit WavDragButton(std::function<juce::MemoryBlock()> renderWav);

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    std::function<juce::MemoryBlock()> renderWav_;
    bool dragStarted_ = false;
};

} // namespace suite
