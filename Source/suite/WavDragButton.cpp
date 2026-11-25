#include "WavDragButton.h"

namespace suite {

WavDragButton::WavDragButton(std::function<juce::File()> makeFile)
    : juce::TextButton("Drag WAV"), makeFile_(std::move(makeFile)) {
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setTooltip("Drag the frozen hit out as a WAV. With no frozen hit, a fresh hit is rendered.");
}

void WavDragButton::mouseDown(const juce::MouseEvent& event) {
    dragStarted_ = false;
    juce::TextButton::mouseDown(event);
}

void WavDragButton::mouseDrag(const juce::MouseEvent& event) {
    if (dragStarted_ || makeFile_ == nullptr || event.getDistanceFromDragStart() < 8)
        return;
    const auto file = makeFile_();
    if (!file.existsAsFile())
        return;
    dragStarted_ = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles({ file.getFullPathName() }, false, this);
}

void WavDragButton::mouseUp(const juce::MouseEvent& event) {
    juce::TextButton::mouseUp(event);
    dragStarted_ = false;
}

} // namespace suite
