#pragma once

#include <juce_graphics/juce_graphics.h>

namespace openhatter::theme {

inline const juce::Colour kBackground{ 0xff101716 };
inline const juce::Colour kPanel{ 0xff1a2421 };
inline const juce::Colour kPanelEdge{ 0xff34433d };
inline const juce::Colour kKnobBody{ 0xff202d28 };
inline const juce::Colour kTrack{ 0xff3d524a };
inline const juce::Colour kText{ 0xfff3f1e8 };
inline const juce::Colour kTextDim{ 0xffa8b1a7 };

inline const juce::Colour kAccentBrass{ 0xffd7ae68 };
inline const juce::Colour kAccentAqua{ 0xff77cfc0 };
inline const juce::Colour kAccentCoral{ 0xffe88970 };
inline const juce::Colour kAccentDim{ 0xff53645c };

// Keep the look-and-feel accents mapped to the current palette.
inline const juce::Colour kAccentCyan = kAccentAqua;
inline const juce::Colour kAccentAmber = kAccentBrass;
inline const juce::Colour kAccentViolet = kAccentCoral;
inline const juce::Colour kAccentNeonGreen = kAccentAqua;
inline const juce::Colour kAccentFlame = kAccentCoral;

} // namespace openhatter::theme
