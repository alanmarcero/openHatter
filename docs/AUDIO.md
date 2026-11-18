# openHatter synthesis

Each hit is an unpitched hat. Eight hiss modes feed the air filters: resonant 909 noise, softened silk noise, a noisy velvet shimmer, bit-reduced crunch, six band-limited squares ring-modulated at the 808 metal ratios for the attack and then noise, a bronze wash whose sine partials die in a few dozen milliseconds unless Hiss Density is high enough for a ride, bright sizzle, and a short comb wash. A closing lowpass takes the top off through the tail. The air layer does not follow the played MIDI note.

The bronze shell is four inharmonic bandpasses from about 900 Hz upward, plus a short knock in that same range. It is mixed under the air and does not add a low tom tone. The stick, pedal and laser contacts are short partials with a band-limited noise tick. Partials fade out before Nyquist. The laser strike is a downward pitch sweep onto the selected contact pitch.

One-shot tails follow the air attack, hold, decay and curve. Negative curves are exponential and front-loaded, so a short decay reads as a closed hat and a long decay as an open hat or ride. With **Hold notes** selected, the amplitude envelope sustains until a matching note-off on the same MIDI channel. Preview always plays a bounded one-shot.

The exciter, compressor, limiter, EQ and a short gated reverb sit on the engine output. Factory presets are gain-matched so the limiter is a ceiling rather than the sound. MIDI CC 7 controls output gain, CC 74 changes air brightness, CC 120 and 123 silence voices, and CC 121 resets CC overrides.

Factory programs keep their parameter IDs. Sessions saved before the 4.6 bank load their stored values and are labelled **Custom sound** until a factory hat is chosen.
