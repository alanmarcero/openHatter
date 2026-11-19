#include "AbCompare.h"

namespace suite {

void AbCompare::prime(const juce::MemoryBlock& state) {
    slots_[0] = state;
    slots_[1] = state;
    active_ = 0;
}

void AbCompare::storeActive(const juce::MemoryBlock& state) {
    slots_[static_cast<size_t>(active_)] = state;
}

void AbCompare::copyActiveToInactive() {
    slots_[static_cast<size_t>(1 - active_)] = slots_[static_cast<size_t>(active_)];
}

void AbCompare::setActive(int slot) noexcept {
    active_ = slot == 1 ? 1 : 0;
}

void AbCompare::setSlots(int active, juce::MemoryBlock slotA, juce::MemoryBlock slotB) {
    slots_[0] = std::move(slotA);
    slots_[1] = std::move(slotB);
    setActive(active);
}

const juce::MemoryBlock& AbCompare::slot(int index) const noexcept {
    return slots_[index == 1 ? 1 : 0];
}

} // namespace suite
