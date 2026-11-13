# openHatter v2 Design

**Date:** 2026-11-12
**Status:** Spec based on the current v1.2 feature set and the user's request to
rebuild openKicker in the same way.

## Intent

Rebuild openHatter as a clear, tunable open hi-hat instrument for dance music
and trance. Replace the synthesis, effects, state integration, factory bank,
and editor implementations while retaining every current capability and host
contract. Replace all 48 factory programs with new designs, add preset-safe
section locks, and redraw the editor with a layout that keeps labels and values
readable at its default size.

## Current feature inventory

openHatter is a JUCE 8 / C++20 instrument that ships as VST3, AU, LV2, and
Standalone on macOS, Windows, and Linux. V2 retains:

- Four cymbal synthesis modes: 909 Classic, Anjuna Silk, Uplifting FM Bell,
  and Analog 808 Crisp; tuned partials, body filtering, metal level, decay,
  pitch envelope, and chromatic root, octave, and cents tuning.
- Fixed-root or MIDI note-tracking pitch, velocity response, pitch lock, Push It
  audition, and session WAV lock-in.
- Pre-chick stick bounces with count, spacing, level, jitter, decay, and stereo
  spread; Crisp Stick, Pedal Foot, and Laser Tick transients with level and
  frequency controls.
- Four air textures (909 Crisp, Anjuna Silk, Velvet Shimmer, Lo-Fi Crunch),
  attack/hold/decay shaping, curve, high-pass/low-pass filters, level, and
  Mid/Side width.
- Four exciter curves (Tape Warmth, Tube Sheen, Hard Clip, Crystal Exciter),
  compressor, brickwall limiter, gated shimmer reverb, host-synced reverse
  swells, master width, output gain, peak display, and animated Mad Hatter
  stage.
- Preset browsing, Simple and Advanced modes, all existing parameter IDs,
  program behavior, saved state and cached WAV data, and MIDI CC behavior:
  CC 7 output, CC 74 air LPF, CC 120/123 silence, and CC 121 resets CC 7/74.
- Exactly 48 factory programs. Every existing name and setting is retired.

## Goals

- Reimplement the complete synthesis/effects/engine/processor/editor stack while
  keeping the same product and host-facing contract.
- Keep the 48-program bank and replace every old name and preset definition
  with an original trance-focused open-hat sound. Each preset sets all
  presettable parameters so its result does not depend on the preset loaded
  before it.
- Add a one-shot ADSR amplitude envelope: Attack 0.05–20 ms (0.35 ms default),
  Decay 1–600 ms (72 ms), Sustain 0–1 (0.70), and Release 5–800 ms (180 ms).
  The envelope shapes the dry voice within the cymbal/noise tail duration;
  existing tone and air decay controls still define their independent layer
  decays.
- Add independent lock buttons to every editable preset section. While a
  section is locked, factory preset browsing leaves every parameter in that
  section unchanged; manual edits still work.
- Redraw the editor around three pages: **Hat**, **Motion**, and **FX**. Keep
  Simple and Advanced modes, the stage, audition, tuning, all effects, and
  preset navigation. Advanced mode reveals detailed controls without hiding
  any feature.
- Publish as v2.0.0 with an updated README, audio notes, and three native editor
  screenshots showing the new pages.

## Non-goals

- Do not change the plugin name, bundle ID (`com.alanmarcero.openhatter`),
  manufacturer/plugin codes, APVTS root, existing parameter IDs/versions,
  formats, JUCE version, or MIDI-controller meanings.
- Do not remove a synthesis mode, noise texture, transient, articulation,
  tuning option, effect, lock-in behavior, preset workflow, editor mode,
  audition action, or stage animation.
- Do not retain any v1 factory preset name, parameter set, or factory preset
  ordering as active content. Do not add sample or third-party dependencies.

## Approaches

1. Patch the current engine and keep the current editor layout. This is the
   smallest change but does not satisfy the requested complete rebuild.
2. Keep the current and new engines side by side. This preserves a selectable
   legacy sound but duplicates the implementation and keeps old presets alive.
3. Replace the DSP, effects, state integration, presets, and UI while preserving
   the host contract and every documented behavior. This follows the user's
   request to redo openKicker the same way and is the selected approach.

## Audio and preset behavior

The four named cymbal modes remain distinct synthesis choices with six-partial
metallic body, air-noise, and transient contributions. Rebuild each topology
and its stereo/decay behavior rather than wrapping the old voice. The global
ADSR shapes the generated dry hat before the effects bus; it does not shorten
the reverb tail or replace the metal/noise layer decay controls. The WAV-lock
path continues to capture and replay the finished hit, including at a changed
host sample rate.

The new bank contains exactly 48 original trance-focused presets covering
progressive off-beats, uplifting 138–140 material, classic Dutch trance,
tech-trance, psy-trance, and clean club utility. No old factory name or old
factory value set remains. Each preset is a complete, directly authored
parameter definition; program index arithmetic only selects the definition
and does not derive or perturb sound values. Existing session state and cached WAV content still
load; an old saved program index resolves to the new bank after restore.

## Section locks

Locks are saved UI/session state, not host automation parameters. A locked
section protects its owned values from program changes while keeping the
controls editable by hand. Existing `lock_pitch` and `lock_in` remain independent
musical and WAV features.

| Section | Preset-owned settings |
| --- | --- |
| Articulation | Pre-chick count, spread, pan, level, decay, jitter; chick type, level, and frequency |
| Cymbal & Tuning | Cymbal mode, note, octave, fine tune, frequency, Q, metal decay, pitch envelope/decay, level, pitch lock, ADSR |
| Air | Texture type, attack, hold, decay, curve, HPF, LPF, stereo width, and level |
| Exciter | Exciter mode, drive, tilt, and mix |
| Dynamics | Compressor controls, limiter ceiling/release/enabled |
| Space & Output | Reverb, choke, reverse mode/mix, MIDI mode, WAV lock-in, master width, and output gain |

## Editor design

Use the dark ink/steel signal-console visual language introduced in openKicker
v2, with cyan for the primary signal, amber for sustained/body controls, and
coral for transients. Keep the editor at 1100 × 972. Keep the Mad Hatter stage
and its artwork visible on the Hat page; use a separate content area for page
controls so the existing dense faceplate does not force text overlap.

- **Hat:** note keyboard/tuning, mode and partial body, ADSR, chick transient,
  waveform/level display, and the Mad Hatter stage.
- **Motion:** pre-chick articulation and air-tail/noise shaping with the
  section locks for those groups.
- **FX:** exciter, compressor/limiter, reverb/choke, reverse sync, stereo width,
  output, and live peak meter.
- Every page retains preset selector and previous/next navigation. Each
  editable section has an always-visible lock button, a clear locked state,
  and a tooltip. Both view modes use non-overlapping label/control bounds.

## Constraints and release

- Use JUCE 8 and C++20; add no dependencies.
- Preserve the current host-visible parameter IDs and their versions. Add
  versioned IDs for ADSR and section-state data; absent lock state in a legacy
  session defaults to unlocked.
- Preserve all existing CMake targets and the macOS, Windows, and Linux build
  workflow.
- Set CMake/project version and release tag to v2.0.0. Push `main` and the tag
  to `github.com/alanmarcero/openHatter` using the required personal Git identity.

## Success checks

- Existing feature and MIDI contracts remain available; old sessions and WAV
  cache data restore with the same audible values.
- Exactly 48 unique new presets render finite, non-silent audio; no old preset
  names or definitions remain in the factory bank.
- Loading a preset preserves all values in every locked section and updates
  unlocked sections. Lock state survives save/restore.
- All editor pages and both view modes render at 1100 × 972 with no text
  overlap; screenshots show the built editor.
- All plugin formats build, CI tests pass, and the v2.0.0 GitHub Action/release
  completes.
