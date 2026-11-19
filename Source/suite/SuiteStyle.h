#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace suite {

struct Style {
    juce::Colour background { 0xff0d1317 };
    juce::Colour panel { 0xff152026 };
    juce::Colour edge { 0xff283842 };
    juce::Colour text { 0xffe6f0f5 };
    juce::Colour textDim { 0xff869ca8 };
    juce::Colour accent { 0xff48f0c8 };
    juce::Colour accentAlt { 0xffffbe3b };
};

} // namespace suite
