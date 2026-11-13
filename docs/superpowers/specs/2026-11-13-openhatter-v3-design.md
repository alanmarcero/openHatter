# openHatter v3 Design

**Date:** 2026-11-13
**Status:** Draft for approval

## Intent

Rebuild openHatter's sound engine, editor, and factory bank around MIDI-first
playing. A MIDI note-on supplies the pitch and octave directly. The editor has
no root-note selector, octave selector, pitch lock, or fixed-root/note-tracking
switch.

## Current behavior

openHatter is a JUCE 8 / C++20 VST3, AU, LV2, and Standalone instrument. The
current editor selects a root note and octave, with an optional fine-tune and
pitch-lock control. Note tracking is optional; when enabled, the engine adds
the incoming MIDI note's interval from that selected root. The editor has Hat,
Motion, and FX pages with Simple and Full Detail modes. The factory bank has
48 programs.

## Goals

- Replace the current MIDI-relative pitch calculation with an absolute MIDI
  note-to-frequency mapping: `440 * 2^((noteNumber - 69) / 12)`.
- Make the first pitched metal resonator mode use that exact frequency for
  every MIDI note number from 0 through 127. Keep upper metallic modes
  inharmonic for a cymbal character; suppress modes above Nyquist.
- Keep note-on sample timing and velocity response. Each note-on retriggers
  the one-shot voice; note-off does not change its decay.
- Replace the synthesis voice with an impulse-excited modal metal body plus a
  separate noise-air layer and stick transient. Rebuild the sound controls so
  presets can shape mode spacing, damping, excitation, noise, and decay without
  changing the MIDI fundamental.
- Replace the editor with one MIDI-first surface. Show the last received note,
  MIDI number, calculated frequency, velocity, and an input-waiting state. Do
  not offer any pitch-selecting or pitch-locking control.
- Replace all 48 factory presets with original sound designs. Keep the count
  and preset navigation. Each program sets every sound-defining value so its
  output does not depend on the previously selected program.
- Retain the existing product identity, plugin formats, non-pitch sound
  controls and effects, section preset locks, velocity response, and MIDI CC
  behavior where those features fit the new engine.

## Non-goals

- Do not change the bundle ID, manufacturer/plugin codes, JUCE version,
  formats, or MIDI CC assignments.
- Do not keep the v2 voice, pitch-relative tracking path, old preset sound
  designs, or WAV lock-in playback. A cached render cannot follow a new MIDI
  pitch.
- Do not add sample libraries, third-party audio dependencies, or a second
  legacy engine.

## Approaches

1. Patch v2's relative-note calculation. This preserves the old engine and UI,
   but does not meet the request for a new engine and editor.
2. Build a chromatic sample bank. This maps notes predictably, but adds a large
   set of audio assets and makes sound changes less flexible than synthesis.
3. Replace the voice with a MIDI-driven modal resonator. The MIDI note number
   sets the exact fundamental; inharmonic modes, noise, and transient layers
   shape the open-hat sound. This is the selected approach because it gives
   direct MIDI pitch control and keeps the instrument fully synthesizable.

## Audio and MIDI behavior

For each MIDI note-on, convert its 0–127 note number to frequency using A4 =
440 Hz and 12-tone equal temperament. Do not calculate an interval from a
preset root. The lowest resonator mode is centered at this frequency. Higher
modes may use preset-defined inharmonic ratios, but may not retune the lowest
mode. Modes above the current sample-rate Nyquist frequency contribute no
audio. Preserve the MIDI event's sample offset within the audio block and the
current velocity-to-level behavior. The voice remains a one-shot percussion
voice; MIDI note-off does not truncate the tail.

The audition action replays the last received MIDI note, or MIDI note 60 before
any external note has arrived. The MIDI display reports the conventional note
name, raw note number, frequency, and velocity so the MIDI number and frequency
remain unambiguous across DAW octave-label conventions.

## Presets and retained controls

Keep 48 factory slots, but replace every name and setting. Use six groups of
eight original presets: progressive, uplifting, classic, tech, psy, and club
utility. Presets can vary resonator brightness/spacing, damping, air spectrum,
transient, articulation, and effects. No preset value may shift the MIDI
fundamental.

Keep the current exciter, dynamics, reverb, reverse swell, output, articulation,
air-layer, ADSR, section-lock, and CC 7/74/120/121/123 behaviors as practical
with the replacement engine. Remove the MIDI mode, pitch selector, fine-tune,
pitch-lock, and WAV lock-in controls from the editor. Retain existing plugin
identity and serialized parameter identifiers where needed for older sessions;
legacy pitch values must not override MIDI input.

## Editor direction

Use a single-screen, MIDI-first layout at the existing 1100 × 972 editor size.
Retain the dark machined-metal character and bronze/cyan lighting as a reference,
but redraw the component hierarchy and control layout. Use the existing Hatter
artwork only as a restrained brand accent.

```text
┌ openHatter ───────────── preset ‹ 48 / 48 › ─────── audition ─ output ┐
│  MIDI INPUT                                                           │
│  C4        60       261.63 Hz       velocity 0.79       ● received    │
├──────────────────────────────────────────────────────────────────────┤
│  METAL BODY            AIR & HIT                   MOTION              │
│  resonance / decay     texture / level             bounce / envelope   │
├──────────────────────────────────────────────────────────────────────┤
│  EXCITER               DYNAMICS                   SPACE                │
│  drive / sheen         comp / limiter             reverb / reverse     │
└──────────────────────────────────────────────────────────────────────┘
```

The MIDI note readout is the main visual anchor. Keep labels in sentence case,
use a clear type scale, and reserve color for signal state and control groups.
Do not retain Simple/Full Detail switching or pitch controls. Preset section
locks remain available on their associated non-pitch groups.

## Compatibility and release

- Preserve `com.alanmarcero.openhatter`, product name, vendor/plugin codes,
  formats, MIDI CC behavior, and existing non-pitch parameter IDs.
- Keep old state loading for shared parameters. Old root/octave/fine values may
  be present in a saved session but are ignored for MIDI-triggered pitch.
- Update the project version to 3.0.0, README/audio notes/screenshots, and
  GitHub release tag `v3.0.0`.
- Push `main` and `v3.0.0` using the personal noreply identity. Track the GitHub
  Actions build and release until completion.

## Success criteria

- MIDI note numbers 0–127 map to their expected equal-tempered frequencies;
  incoming note-on sample offsets trigger at the corresponding sample.
- No UI control or preset changes the MIDI fundamental.
- The new UI shows the actual incoming note, number, frequency, and velocity,
  with no root/octave selector, pitch lock, or WAV lock-in.
- Exactly 48 new presets load independently and retain their sound character
  while following every incoming MIDI pitch.
- All existing plugin formats build, the GitHub Actions workflow completes,
  and the v3.0.0 release contains the platform packages.
