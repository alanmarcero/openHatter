#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <set>
#include <utility>
#include <vector>

namespace openhatter::presets {

struct FactoryPreset {
    const char* name;
    std::vector<std::pair<juce::String, float>> values;
};

const std::vector<FactoryPreset>& factoryPresets();
void apply(const FactoryPreset& preset, juce::AudioProcessorValueTreeState& state,
           const std::array<bool, 8>& lockedSections,
           const std::set<juce::String>& lockedParams = {});
void applyValues(const std::vector<std::pair<juce::String, float>>& values,
                 juce::AudioProcessorValueTreeState& state,
                 const std::array<bool, 8>& lockedSections,
                 const std::set<juce::String>& lockedParams = {});

} // namespace openhatter::presets
