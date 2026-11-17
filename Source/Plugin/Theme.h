#pragma once

#include <juce_graphics/juce_graphics.h>

namespace openhatter::theme {

inline const juce::Colour kBackground{ 0xff111315 };
inline const juce::Colour kPanel{ 0xff1a1d1f };
inline const juce::Colour kPanelEdge{ 0xff4a4e4d };
inline const juce::Colour kKnobBody{ 0xff101213 };
inline const juce::Colour kTrack{ 0xff343938 };
inline const juce::Colour kText{ 0xfff4f0e5 };
inline const juce::Colour kTextDim{ 0xffc0bcae };

inline const juce::Colour kAccentBrass{ 0xffe2b86b };
inline const juce::Colour kAccentSteel{ 0xff9daeb3 };
inline const juce::Colour kAccentAqua{ 0xffa8c4c8 };
inline const juce::Colour kAccentCoral{ 0xffd99161 };
inline const juce::Colour kAccentDim{ 0xff8d9290 };

// Keep the look-and-feel accents mapped to the current palette.
inline const juce::Colour kAccentCyan = kAccentAqua;
inline const juce::Colour kAccentAmber = kAccentBrass;
inline const juce::Colour kAccentViolet = kAccentCoral;
inline const juce::Colour kAccentNeonGreen = kAccentAqua;
inline const juce::Colour kAccentFlame = kAccentCoral;

} // namespace openhatter::theme
