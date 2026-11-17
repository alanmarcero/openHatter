# openHatter v4 Redesign Implementation Plan

> **For agentic workers:** Native implementation in this session; each task ends with a focused verification before release.

**Goal:** Rebuild openHatter's audible balance and presets, make the interface clearer, and release v4.0.0.

**Architecture:** Keep the current JUCE plugin, processor, parameter IDs, and host/MIDI paths. Rework the voice and factory preset balance around an audible broadband open-hat body with controlled metallic partials, then widen and restyle the existing editor.

**Tech Stack:** C++20, JUCE 8, CMake, Catch2, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-11-17-openhatter-v4-redesign.md`

## Global Constraints

- Keep MIDI pitch, velocity, ADSR, effects, state, and automation behavior.
- Keep all 48 factory slots and existing plugin parameter IDs.
- Keep the tuned metal layer restrained in factory presets.
- Keep Windows builds enabled and make macOS/Linux workflow jobs reversible.
- Use the personal Git identity for commits and tags.

## Review Focus

- Lowest-output preset must exceed the minimum peak and RMS without clipping.
- Low and high MIDI notes must remain finite and audible.
- Section locks, MIDI ADSR, host roll, and six resonator controls remain usable.
- Both editor pages fit at the new size with readable text and labeled controls.
- A skipped macOS/Linux job must not prevent a successful Windows-only release.

---

### Task 1: Rebalance the synth voice and factory presets

**Files:** `Source/Core/HatSynth.cpp`, `Source/Core/HatSynth.h`, `Source/Plugin/Presets.cpp`, `Tests/HatSynthTests.cpp`, `Tests/PluginTests.cpp`

- [x] Use the failing 48-preset audio assertion as the baseline.
- [x] Improve the broadband transient/body contribution while preserving the reduced metal layer and existing parameter semantics.
- [x] Normalize the air level across all 48 presets and retune low-output compressor profiles while preserving their style variations.
- [x] Run synth, preset-audio, MIDI, ADSR, and effects tests.

### Task 2: Make both editor pages readable and direct

**Files:** `Source/Plugin/PluginEditor.cpp`, `Source/Plugin/PluginEditor.h`, `Source/Plugin/Panels.cpp`, `Source/Plugin/Controls.cpp`, `Source/Plugin/EdmLookAndFeel.cpp`, `Source/Plugin/Theme.h`, `Tests/PluginTests.cpp`

- [x] Add editor assertions for the wider window.
- [x] Expand the editor to 1360 pixels and enlarge header, lock, and toggle targets.
- [x] Apply the black-metal and brass palette, improve label hierarchy, and use plain names for actions.
- [x] Render both pages and inspect the regenerated screenshots.

### Task 3: Package and release v4.0.0 on Windows

**Files:** `.github/workflows/build.yml`, `CMakeLists.txt`, `README.md`, `docs/AUDIO.md`, `docs/SCREENSHOTS.md`, factory screenshots.

- [x] Keep macOS/Linux build jobs disabled and permit tag release after Windows succeeds while other jobs are skipped.
- [x] Update version and product documentation to v4.0.0.
- [x] Build AU, VST3, LV2, and Standalone locally; run the full CTest suite.
- [ ] Check the diff, commit with the personal identity, audit commit emails, push `main` and `v4.0.0`, and monitor Windows release jobs.
