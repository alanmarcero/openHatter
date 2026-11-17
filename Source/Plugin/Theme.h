#pragma once

#include <juce_graphics/juce_graphics.h>

namespace openhatter::theme {

inline const juce::Colour kBackground{ 0xff151923 };
inline const juce::Colour kPanel{ 0xff222835 };
inline const juce::Colour kPanelEdge{ 0xff435063 };
inline const juce::Colour kKnobBody{ 0xff303846 };
inline const juce::Colour kTrack{ 0xff435063 };
inline const juce::Colour kText{ 0xfff4f3ef };
inline const juce::Colour kTextDim{ 0xffbdc7cf };

inline const juce::Colour kAccentBrass{ 0xfff1bd80 };
inline const juce::Colour kAccentSteel{ 0xffbdc7cf };
inline const juce::Colour kAccentAqua{ 0xff8edacc };
inline const juce::Colour kAccentCoral{ 0xffc5b3f4 };
inline const juce::Colour kAccentDim{ 0xffbdc7cf };

// Keep the look-and-feel accents mapped to the current palette.
inline const juce::Colour kAccentCyan = kAccentAqua;
inline const juce::Colour kAccentAmber = kAccentBrass;
inline const juce::Colour kAccentViolet = kAccentCoral;
inline const juce::Colour kAccentNeonGreen = kAccentAqua;
inline const juce::Colour kAccentFlame = kAccentCoral;

} // namespace openhatter::theme
