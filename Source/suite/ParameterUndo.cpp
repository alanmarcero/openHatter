#include "ParameterUndo.h"

namespace suite {

namespace {

constexpr size_t kMaxSteps = 100;

// The callbacks touch the parameter tree, which belongs to the message thread.
bool onMessageThread() {
    auto* manager = juce::MessageManager::getInstanceWithoutCreating();
    return manager == nullptr || manager->isThisTheMessageThread();
}

} // namespace

ParameterUndo::ParameterUndo(juce::AudioProcessor& processor, std::function<juce::ValueTree()> capture,
                             std::function<void(const juce::ValueTree&)> restore, std::function<size_t()> activeSlot)
    : processor_(processor), capture_(std::move(capture)), restore_(std::move(restore)), activeSlot_(std::move(activeSlot)) {
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
        beginChange();
        return;
    }
    endChange();
}

void ParameterUndo::beginChange() {
    if (!onMessageThread())
        return;
    if (openChanges_++ == 0)
        before_ = capture_();
}

void ParameterUndo::endChange() {
    if (!onMessageThread() || openChanges_ == 0 || --openChanges_ > 0)
        return;
    if (before_.isEquivalentTo(capture_()))
        return;
    auto& history = histories_[activeSlot_()];
    history.undo.push_back(before_);
    if (history.undo.size() > kMaxSteps)
        history.undo.erase(history.undo.begin());
    history.redo.clear();
}

void ParameterUndo::undo() {
    auto& history = histories_[activeSlot_()];
    if (history.undo.empty())
        return;
    history.redo.push_back(capture_());
    restore_(history.undo.back());
    history.undo.pop_back();
}

void ParameterUndo::redo() {
    auto& history = histories_[activeSlot_()];
    if (history.redo.empty())
        return;
    history.undo.push_back(capture_());
    restore_(history.redo.back());
    history.redo.pop_back();
}

bool ParameterUndo::canUndo() const { return !histories_[activeSlot_()].undo.empty(); }

bool ParameterUndo::canRedo() const { return !histories_[activeSlot_()].redo.empty(); }

void ParameterUndo::clearSlot(int slot) { histories_[slot == 1 ? 1u : 0u] = {}; }

void ParameterUndo::clearAll() { histories_ = {}; }

} // namespace suite
