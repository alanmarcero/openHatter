#include "ParameterUndo.h"

namespace suite {

namespace {

constexpr size_t kMaxSteps = 100;

// The capture and restore callbacks touch the parameter tree, which belongs to the message thread.
bool onMessageThread() {
    auto* manager = juce::MessageManager::getInstanceWithoutCreating();
    return manager == nullptr || manager->isThisTheMessageThread();
}

} // namespace

ParameterUndo::ParameterUndo(juce::AudioProcessor& processor, std::function<juce::ValueTree()> capture,
                             std::function<void(const juce::ValueTree&)> restore)
    : processor_(processor), capture_(std::move(capture)), restore_(std::move(restore)) {
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
    auto& history = histories_[slot_];
    history.undo.push_back(before_);
    if (history.undo.size() > kMaxSteps)
        history.undo.erase(history.undo.begin());
    history.redo.clear();
}

void ParameterUndo::undo() {
    auto& history = histories_[slot_];
    if (history.undo.empty())
        return;
    history.redo.push_back(capture_());
    restore_(history.undo.back());
    history.undo.pop_back();
}

void ParameterUndo::redo() {
    auto& history = histories_[slot_];
    if (history.redo.empty())
        return;
    history.undo.push_back(capture_());
    restore_(history.redo.back());
    history.redo.pop_back();
}

bool ParameterUndo::canUndo() const { return !histories_[slot_].undo.empty(); }

bool ParameterUndo::canRedo() const { return !histories_[slot_].redo.empty(); }

void ParameterUndo::selectSlot(int slot) { slot_ = slot == 1 ? 1u : 0u; }

void ParameterUndo::clearSlot(int slot) { histories_[slot == 1 ? 1u : 0u] = {}; }

void ParameterUndo::clearAll() { histories_ = {}; }

} // namespace suite
