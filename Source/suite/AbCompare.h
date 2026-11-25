#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <array>

namespace suite {

// Two full-state slots. The active slot follows the live sound until the user switches.
class AbCompare {
public:
    void prime(const juce::MemoryBlock& state);
    void storeActive(const juce::MemoryBlock& state);
    void copyActiveToInactive();
    void copySlot(int from, int to);
    void setActive(int slot) noexcept;
    void setSlots(int active, juce::MemoryBlock slotA, juce::MemoryBlock slotB);

    [[nodiscard]] int active() const noexcept { return active_; }
    [[nodiscard]] const juce::MemoryBlock& slot(int index) const noexcept;

private:
    std::array<juce::MemoryBlock, 2> slots_{};
    int active_ = 0;
};

} // namespace suite
