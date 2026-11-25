# Changelog

## v4.8.4

- Aligns the suite bar, presets, WAV drag behavior, and editor layout.
- Aligns the JUCE 8 license and hardens CI.

## v4.8.3

- **Note-Snapped EQ:** Snap all three EQ bands to chromatic notes (A-G with sharps) across two-octave ranges (`E1` to `E3`, `B4` to `B6`, `A#7` to `A#9`), matching OpenKicker's EQ design and behavior.

## v4.8.2

- **Parameter Locking:** Add per-parameter locking across every plugin parameter, persisted in session state and preserved across preset loads and randomization.

## v4.8.1

- **UI & Suite Fixes:** UI polish, MIDI Learn for choke and CC, zero-level dimming, EQ collar focus, unit formatting and typed-value entry, per-slot A/B undo, inline preset confirms, and screen-reader titles.

## v4.8.0 — 2026-11-20

Upgrades the hat synth, effects DSP, and key-tracking control:

- **Oversampled metal & modal bronze/shell:** 4x PolyBLEP AnalogMetal ring-mod matrix, ZDF modal BronzeCymbal bank, and energy-storing shell resonators.
- **Filters, saturation & space:** TPT ZDF SVFs, split-band ADAA hiss saturation, Lagrange fractional-delay CombWash with HF damping, and Jot-style FDN reverb with modulated reads and smoothstep gating.
- **Stereo, distortion & echo:** Per-channel xorshift64* noise with L/R detune and orthogonal diffusion, 4x oversampled pre/de-emphasis distortion, and filtered ADAA echo loop.
- **Key Tracking:** New Key Tracking knob in the Hat Synth panel tuning metallic partials, bronze, and body modes to the played MIDI note.
