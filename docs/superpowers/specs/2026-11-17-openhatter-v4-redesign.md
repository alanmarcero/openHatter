# openHatter v4 Sound and Interface Redesign

## Intent

Make openHatter produce clearly audible, bright open trance hats with a punchy
attack and a smooth noise tail. Keep the metallic layer present but restrained,
so the result does not become a piercing tuned "ting". Make the controls easier
to identify and operate by giving them more room, larger targets, and explicit
labels inspired by openBassSpace's black metal and brass hardware interface.

## Current Evidence

The 48-preset render check only required a 0.15 peak and 0.01 RMS. Some presets
had low transient peaks even when their active RMS was audible. The factory
metal level is scaled to 0.25, which was intentionally introduced to reduce the
tuned layer by 12 dB. The revised audio check requires at least a 0.35 peak and
0.06 active RMS for every factory preset, and was used to retune low-output
attacks without restoring the bright metallic ting.

## Design

- Rework the existing noise, transient, and modal voice so the air and contact
  layers provide a full, clearly audible hat body while the factory metal layer
  remains restrained.
- Keep MIDI pitch and velocity tracking, note-off ADSR, preset locks, effects,
  MIDI CC behavior, plugin parameter IDs, and the 48-program catalog.
- Retune every factory preset around bright trance open-hat roles: tight, bright,
  warm, wide, and long-tail variants. Keep distortion subtle in the factory bank.
- Widen the editor to 1360 pixels and preserve the two existing pages. Use a
  black metal, warm brass, and muted steel palette; increase control and label
  size; replace ambiguous abbreviations with direct action names; make preset
  locks and on/off states obvious.
- Disable macOS and Linux workflow jobs in this zero-star repository. Keep the
  Windows build and a Windows-only tag release path active.

## Success Criteria

- All 48 presets produce finite output, peak at or above 0.35, and active RMS at
  or above 0.06 over the existing MIDI render window, without exceeding the
  existing 1.5 float-peak ceiling.
- Existing synth, MIDI, ADSR, effects, preset, and state tests pass.
- The editor exposes larger, clearly named controls on both pages and renders
  at its new dimensions; updated screenshots show the revised interface.
- The Windows workflow passes on main and the v4.0.0 tag. macOS and Linux jobs
  are skipped, while release creation waits for Windows success.
