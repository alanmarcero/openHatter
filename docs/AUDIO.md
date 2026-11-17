# openHatter synthesis

Each MIDI note-on drives a six-mode metallic resonator at the incoming equal-tempered pitch. Each model supplies inharmonic ratios, spectral weights and damping. Partial spread adjusts the upper frequencies, damping slope sets their decay relative to the fundamental, and stereo spread disperses them across the stereo field. The first resonator keeps the played pitch during a sweep. Partial frequencies at or above Nyquist are omitted.

White, pink silk, velvet shimmer and lo-fi noise feed independent high-pass and low-pass filters. Air attack, hold and decay shape the noise tail in one-shot mode. Stick bounces precede the main hit and can vary in timing and pan. With **Hold notes** selected, metal and air sustain under the amplitude envelope until a matching note-off, including the MIDI channel. Preview always plays a bounded one-shot.

The stick and pedal contacts use individually pitched and weighted sinusoidal partials. Each partial fades from full level at 90% of Nyquist to silence at Nyquist. The laser tick integrates an exponential frequency sweep to land on the selected contact pitch. Its instantaneous frequency controls the same high-frequency fade. The crisp stick's initial impulse lasts one sixth of a millisecond at every sample rate.

The exciter feeds a parallel compressor and limiter. Reverb has separate damping and choke settings. Reverse swells follow the host beat phase and tempo, with a 138 BPM fallback. Master width and level finish the main output. MIDI CC 7 controls output gain, CC 74 changes air brightness, CC 120 and 123 silence voices, and CC 121 resets CC overrides.

Each of the 48 factory hits has an authored metal model, ring time and level, air texture and filter band, and contact pitch and strength. The six families share their room and dynamics settings. Factory room mix ranges from dry to 5%, with reverse off. Performance controls and the user's selected note mode persist when browsing. Existing projects recall their saved parameter values.

At 48 kHz, MIDI note 66 and velocity 100, all 48 factory programs crossed −20 dBFS within 7 ms. Their last samples above −50 dBFS fell between 112 and 247 ms after note-on. Stereo RMS across the first 128 ms ranged from −15.9 to −10.0 dBFS, with sample peaks at the −1 dBFS limiter ceiling.
