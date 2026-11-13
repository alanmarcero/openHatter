# openHatter v3 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use $subagent-driven-development (recommended) or $executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Rebuild openHatter's synthesis, UI, and preset bank so each incoming MIDI note sets the sound's exact pitch and octave.

**Architecture:** Keep JUCE, the plugin identity, and current format targets. Replace the relative-pitch path with an absolute MIDI-frequency modal voice, connect note status to a new editor, and replace the 48 preset definitions. Keep non-pitch controls, effects, state, and controller behavior where they work with the new voice.

**Tech Stack:** C++20, JUCE 8, CMake, existing Catch2 targets.

**Spec:** `docs/superpowers/specs/2026-11-13-openhatter-v3-design.md`

## Global Constraints

- Keep bundle ID `com.alanmarcero.openhatter`, product name, vendor/plugin codes, VST3/AU/LV2/Standalone targets, and JUCE 8.
- Map every MIDI note 0–127 with `440 * 2^((noteNumber - 69) / 12)`; do not derive pitch from preset roots or offsets.
- Keep the first pitched metal mode at the MIDI frequency. Suppress upper modes above Nyquist.
- Preserve note-on sample offsets, velocity response, MIDI CC 7/74/120/121/123, non-pitch effects, and 48 factory slots.
- Remove pitch selection/lock and WAV lock-in from the user-facing editor.
- Add no dependencies. Update release docs and version to 3.0.0.
- Do not add or run local tests; compile locally and monitor the repository's existing GitHub Actions workflow after pushing.

## Review Focus

- Every MIDI note uses its absolute note number, independent of saved pitch controls and the loaded preset.
- A same-block MIDI event is triggered at its supplied sample offset; velocity remains applied to the hit.
- Higher modes above Nyquist are silent without suppressing an in-band fundamental or adding aliasing.
- Preset browsing updates all sound-defining non-pitch values and leaves locked sections unchanged.
- The MIDI monitor shows the received note number and frequency without suggesting a pitch selector.

---

### Task 1: Build the MIDI-driven modal voice

**Files:** `Source/Core/HatSynth.h`, `Source/Core/HatSynth.cpp`, `Source/Core/HatEngine.h`, `Source/Core/HatEngine.cpp`

**Interfaces:** Add `midiNoteToHz(int noteNumber)`. Add a MIDI-specific voice trigger that accepts the absolute note number. Keep the existing settings-based render helper where needed by current core callers.

- [x] Replace the current metallic voice with an impulse-excited modal resonator, air-noise layer, and stick transient.
- [x] Set its first pitched mode from `midiNoteToHz(noteNumber)`; keep inharmonic upper modes and suppress modes above Nyquist.
- [x] Pass `MidiTrigger.noteNumber` directly through `HatEngine` on every note-on. Keep the event's sample offset and velocity.
- [x] Keep reverse, reverb, exciter, dynamics, width, and CC processing compatible with the new voice. Remove WAV playback from the user-facing processing path.
- [x] Build `openhatter_core` locally with `cmake --build build --target openhatter_core --parallel 4`.

### Task 2: Expose live MIDI input to the processor

**Files:** `Source/Plugin/PluginProcessor.h`, `Source/Plugin/PluginProcessor.cpp`, `Source/Plugin/Parameters.cpp`

**Interfaces:** Add atomic display values for last note number and frequency, plus a received-note flag. The processor updates them from MIDI note-on messages and the editor reads them without accessing audio-thread state directly.

- [x] Update the display state from each received note-on, preserving the existing velocity display and sample-accurate trigger queue.
- [x] Make the audition action retrigger the last MIDI note, defaulting to MIDI note 60 before input arrives.
- [x] Stop restoring or applying a frozen WAV to processor playback; retain shared non-pitch state and section-lock behavior.
- [x] Build the plugin processor target.

### Task 3: Replace the factory bank

**Files:** `Source/Plugin/Presets.cpp`, `Source/Plugin/Presets.h`, `Source/Plugin/PluginProcessor.cpp`

**Interfaces:** Keep `factoryPresets()`, `getNumPrograms()`, `getProgramName()`, and lock-aware `setCurrentProgram()` behavior. Keep 48 complete, directly authored definitions in six groups of eight.

- [x] Replace every preset name and value with a new progressive, uplifting, classic, tech, psy, or club-utility design.
- [x] Ensure no preset value changes the incoming MIDI fundamental.
- [x] Keep the preset application path complete so values do not depend on the previously loaded program.
- [x] Build the plugin after the new definitions are in place.

### Task 4: Redraw the MIDI-first editor

**Files:** `Source/Plugin/PluginEditor.h`, `Source/Plugin/PluginEditor.cpp`, `Source/Plugin/Panels.h`, `Source/Plugin/Panels.cpp`, `Source/Plugin/Theme.h`, `Source/Plugin/Controls.cpp`, `assets/*`, `docs/SCREENSHOTS.md`

**UI tokens:** Foundry `#172626`, slate `#314443`, brass `#D5A957`, signal aqua `#70D6C8`, and transient coral `#EC765F`. Use the default sans face for controls and the default monospaced face for MIDI number/frequency readouts. Keep the 1100 × 972 editor bounds.

**Layout:** Put preset navigation and audition in the header. Make the incoming MIDI note the largest element in a live-input monitor, with MIDI number, frequency, velocity, and waiting/received status beside it. Group controls below into Metal, Air & Hit, Motion, and FX regions. Use the existing Hatter art as a small brand accent.

- [x] Replace the three-page/simple-full interface with the single MIDI-first surface and live note display.
- [x] Remove visible root, octave, cents, pitch-lock, fixed-root/note-tracking, and WAV-lock controls.
- [x] Keep preset navigation, audition, section preset locks, and retained non-pitch sound/effect controls reachable without overlapping labels.
- [x] Capture the built editor for the README and screenshot notes.
- [x] Build the Standalone target for visual inspection.

### Task 5: Update documentation and release version

**Files:** `CMakeLists.txt`, `README.md`, `docs/AUDIO.md`, `docs/SCREENSHOTS.md`

- [x] Update the product description, MIDI note behavior, preset bank, controls, screenshots, and build instructions for v3.
- [x] Set project version to `3.0.0`; keep release tag as `v3.0.0`.
- [x] Check README image links and `git diff --check`.

### Task 6: Build, publish, and track GitHub Actions

**Files:** all implementation and documentation changes above.

- [x] Build all configured local plugin formats and Standalone. Do not run the local test suite.
- [ ] Review the final diff against `main` for scope and whitespace churn.
- [ ] Commit using `user.name=Alan Marcero` and `user.email=221119764+alanmarcero@users.noreply.github.com`.
- [ ] Before pushing, run `git log --format='%h %ae %ce' origin/main..HEAD` and confirm both identities use the noreply address.
- [ ] Push `main` and annotated tag `v3.0.0`; monitor GitHub Actions through all platform builds and the release job.
