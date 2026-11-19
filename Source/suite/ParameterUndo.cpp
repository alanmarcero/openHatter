#include "ParameterUndo.h"

namespace suite {

ParameterUndo::ParameterUndo(juce::AudioProcessor& processor, juce::UndoManager& undo, std::function<void()> flush)
    : processor_(processor), undo_(undo), flush_(std::move(flush)) {
    for (auto* parameter : processor_.getParameters())
        parameter->addListener(this);
}

ParameterUndo::~ParameterUndo() {
    for (auto* parameter : processor_.getParameters())
        parameter->removeListener(this);
}

void ParameterUndo::parameterValueChanged(int, float) {}

void ParameterUndo::parameterGestureChanged(int, bool gestureIsStarting) {
    if (gestureIsStarting) {
        if (openGestures_++ == 0)
            undo_.beginNewTransaction("Edit");
        return;
    }
    if (openGestures_ == 0)
        return;
    if (--openGestures_ == 0 && flush_) {
        auto* manager = juce::MessageManager::getInstanceWithoutCreating();
        if (manager == nullptr || manager->isThisTheMessageThread())
            flush_();
    }
}

} // namespace suite
