#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace suite {

// Starts an external file drag. The callback writes the WAV before the drag begins.
class WavDragButton : public juce::TextButton {
public:
    explicit WavDragButton(std::function<juce::File()> makeFile);

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    std::function<juce::File()> makeFile_;
    bool dragStarted_ = false;
};

} // namespace suite
