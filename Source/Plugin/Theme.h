#pragma once

#include <juce_graphics/juce_graphics.h>

namespace openhatter::theme {

inline const juce::Colour kBackground{ 0xff0d1317 };
inline const juce::Colour kPanel{ 0xff152026 };
inline const juce::Colour kPanelEdge{ 0xff283842 };
inline const juce::Colour kKnobBody{ 0xff1b272d };
inline const juce::Colour kTrack{ 0xff283842 };
inline const juce::Colour kText{ 0xffe6f0f5 };
inline const juce::Colour kTextDim{ 0xff869ca8 };

inline const juce::Colour kAccentBrass{ 0xffffbe3b };
inline const juce::Colour kAccentSteel{ 0xff869ca8 };
inline const juce::Colour kAccentAqua{ 0xff48f0c8 };
inline const juce::Colour kAccentCoral{ 0xff5ce1ff };
inline const juce::Colour kAccentDim{ 0xff869ca8 };

// Disabled controls draw at this opacity.
inline constexpr float kDimAlpha = 0.4f;
// Inactive controls stay clearly dimmed while keeping stored readouts readable.
inline constexpr float kInactiveControlAlpha = 0.48f;

// Keep the look-and-feel accents mapped to the current palette.
inline const juce::Colour kAccentCyan = kAccentAqua;
inline const juce::Colour kAccentAmber = kAccentBrass;
inline const juce::Colour kAccentViolet = kAccentCoral;
inline const juce::Colour kAccentNeonGreen = kAccentAqua;
inline const juce::Colour kAccentFlame = kAccentCoral;

} // namespace openhatter::theme
