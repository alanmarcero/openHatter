# openHatter v3 audio notes

## MIDI-tracked modal voice

Each MIDI note-on maps to the equal-tempered frequency `440 × 2^((note - 69) / 12)`. This frequency drives the first partial of a six-mode metallic resonator. The remaining partials use inharmonic ratios selected by the cymbal mode. Their relative level and damping shape the metallic character; partials above Nyquist are omitted to avoid aliasing. The metal decay and upper-mode sweep shape each hit without changing its MIDI-selected base pitch.

An independently shaped noise layer supplies the air tail. It supports white, pink silk, velvet shimmer, and low-fi textures with attack, hold, decay, curve, serial high-pass and low-pass filters, stereo width, and level. Stick bounces can precede the main hit with timing variation and alternating pan. A chick transient adds stick, pedal, or laser contact. The one-shot ADSR shapes the dry hit; the reverb tail continues separately.

## Effects path

The exciter shapes the hit before a parallel compressor, stereo limiter, and four-line feedback reverb. Reverb damping and choke shape its tail. Host transport sets the capture length for reverse beat-end, half-beat, and quarter-beat swells. Master stereo width and output level finish the signal chain.

The reverse cue uses the host beat phase and transport tempo. When transport data is unavailable, the engine uses its 138 BPM fallback. MIDI CC 7 controls output gain, CC 74 changes air brightness, CC 120 and 123 silence the active voice, and CC 121 resets CC 7 and CC 74.

## Preset design

The 48 factory presets cover Progressive, Uplifting, Classic, Tech, Psy, and Club styles. Each preset defines articulation, metal mode and spectrum, air texture and envelope, one-shot ADSR, exciter, dynamics, space, reverse behavior, width, and output. Section locks let a performer keep selected sound-design groups while browsing.

## Useful starting points

- For a short kick-pocket hat, lower Air Decay and Metal Decay, then use a fast ADSR release and moderate Air High Pass.
- For a wide uplifting hat, choose a brighter metal mode, increase Air Width, and add a small amount of room and reverse mix.
- For a darker tech hat, lower Air Low Pass, shorten bounce spread, add exciter drive, and tighten the reverb choke.
- If the transient is soft, reduce ADSR Attack or raise Chick Level before increasing output.
