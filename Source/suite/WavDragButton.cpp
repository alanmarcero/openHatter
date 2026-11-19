#include "WavDragButton.h"

namespace suite {

WavDragButton::WavDragButton(std::function<juce::MemoryBlock()> renderWav)
    : juce::TextButton("Drag"), renderWav_(std::move(renderWav)) {}

void WavDragButton::mouseDown(const juce::MouseEvent& event) {
    dragStarted_ = false;
    juce::TextButton::mouseDown(event);
}

void WavDragButton::mouseDrag(const juce::MouseEvent& event) {
    if (dragStarted_ || renderWav_ == nullptr || event.getDistanceFromDragStart() < 8)
        return;
    const auto wav = renderWav_();
    if (wav.getSize() < 12)
        return;
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("openHatter-" + juce::Uuid().toString() + ".wav");
    if (!file.replaceWithData(wav.getData(), wav.getSize()))
        return;
    dragStarted_ = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles({ file.getFullPathName() }, false, this);
}

void WavDragButton::mouseUp(const juce::MouseEvent& event) {
    juce::TextButton::mouseUp(event);
    dragStarted_ = false;
}

} // namespace suite
