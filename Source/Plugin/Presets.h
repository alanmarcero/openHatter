#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>
#include <vector>

namespace openhatter::presets {

struct FactoryPreset {
    const char* name;
    std::vector<std::pair<juce::String, float>> values;
};

const std::vector<FactoryPreset>& factoryPresets();
void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state,
           const std::array<bool, 6>& lockedSections);

} // namespace openhatter::presets
