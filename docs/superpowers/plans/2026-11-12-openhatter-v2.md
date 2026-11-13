# openHatter v2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use $subagent-driven-development (recommended) or $executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild openHatter v2 with a new 48-preset trance bank, lockable sections, full ADSR, complete DSP/UI replacement, and no feature loss.

**Architecture:** Keep JUCE, plugin identity, host parameter IDs, MIDI/state contracts, and formats. Replace the internal voice, effect, engine, preset, state, and editor implementations; apply locks in the single factory-program path and save lock state in the session tree.

**Tech Stack:** C++20, JUCE 8, CMake, existing Catch2 suite.

**Spec:** `docs/superpowers/specs/2026-11-12-openhatter-v2-design.md`

## Global Constraints

- Preserve `com.alanmarcero.openhatter`, every current parameter ID/version, MIDI CC meanings, plugin formats, CMake targets, and state/WAV restore behavior.
- Replace all 48 old factory presets with exactly 48 new trance-focused presets.
- Add section-lock UI/session state without making locked controls uneditable.
- Keep the 1100 × 972 default editor bounds and expose all controls in Simple and Advanced modes.
- Use JUCE 8 and C++20; add no dependencies.
- Publish v2.0.0 to `main` and tag `v2.0.0` with the personal noreply Git identity.

## Review Focus

- Locked values survive preset changes across the reset-to-default path; unlocked values update from the selected preset.
- The global ADSR reaches silence at the dry voice tail boundary without cutting off the effects tail.
- WAV lock-in restores and renders at the current host sample rate.
- Internal/external MIDI note and CC handling remains channel-omni and finite at low/high sample rates.
- All controls remain visible and text does not overlap at the fixed editor size on each page and mode.

---

### Task 1: Replace the hat voice and add ADSR

**Files:** `Source/Core/HatSynth.h`, `Source/Core/HatSynth.cpp`, `Tests/HatSynthTests.cpp`

**Interfaces:** Keep existing public note/tuning and voice-render interfaces; add ADSR values to the voice settings using the spec ranges. Preserve the four cymbal modes, six partials, noise contribution, transient types, and deterministic state.

- [x] Rebuild the four metallic modes and shared dry-hit envelope; preserve tone/noise layer decays and tail bounds.
- [x] Keep stereo phase state independent per channel and clamp invalid/extreme settings to finite output.
- [x] Update existing synth tests only where expected preset-independent output or new ADSR defaults changed.
- [x] Build the core target.

### Task 2: Replace effects and engine orchestration

**Files:** `Source/Core/Effects.h`, `Source/Core/Effects.cpp`, `Source/Core/HatEngine.h`, `Source/Core/HatEngine.cpp`, related existing effect/engine tests.

**Interfaces:** Preserve the current engine/effect settings structs and public process calls; keep host transport, reverse sync modes, compressor, limiter, reverb, choke, width, and MIDI controller behavior.

- [x] Reimplement the exciter, compressor/limiter, reverberation, choke gate, and reverse swell chain with smoothed parameter changes and finite-state recovery.
- [x] Preserve all CC mappings and reset semantics from the README.
- [x] Update only existing assertions made stale by the implementation change.
- [x] Build the engine target.

### Task 3: Add lock-aware presets, parameters, and session state

**Files:** `Source/Plugin/Parameters.h/.cpp`, `Source/Plugin/Presets.h/.cpp`, `Source/Plugin/PluginProcessor.h/.cpp`, `Tests/PluginTests.cpp`.

**Interfaces:** Preserve old APVTS IDs/versions and processor entry points. Add `amp_attack`, `amp_decay`, `amp_sustain`, and `amp_release`; add six non-automatable lock flags to saved session state using the spec's ownership table.

- [x] Replace the factory bank with exactly 48 uniquely named, directly authored new trance hats; set every preset-owned field explicitly in each definition (no index-based value formulas).
- [x] Route processor and editor program changes through one lock-aware preset application path; snapshot all values owned by locked sections before any default reset.
- [x] Preserve old parameter/WAV restore behavior; missing lock data loads as unlocked; retired factory indices select the new bank.
- [x] Update existing catalog assertions and add lock/session assertions to the existing plugin test area.
- [x] Build the plugin processor target.

### Task 4: Reimplement the editor and capture all pages

**Files:** `Source/Plugin/Theme.h`, `Source/Plugin/EdmLookAndFeel.h/.cpp`, `Source/Plugin/Controls.h/.cpp`, `Source/Plugin/Panels.h/.cpp`, `Source/Plugin/PluginEditor.h/.cpp`, `Tests/PluginTests.cpp`, `assets/*`.

**Interfaces:** Keep `OpenHatterEditor`, preset navigation, Simple/Advanced switching, audition, lock-pitch, lock-WAV, all parameters, and stage animation. Use the spec's Hat/Motion/FX pages and six section locks.

- [x] Rebuild the header, page navigation, panels, section bounds, controls, tooltips, and visible lock states in the openKicker v2 visual language.
- [x] Keep all controls reachable in both view modes and all text bounds disjoint at 1100 × 972.
- [x] Capture built-editor screenshots for Hat, Motion, and FX, and link them in the screenshot document and README.
- [x] Update the existing editor screenshot test's capture setup and capture all six views.
- [x] Build the Standalone target and inspect all screenshots for overlaps or truncation.

### Task 5: Update release documentation and version

**Files:** `CMakeLists.txt`, `README.md`, `docs/AUDIO.md`, `docs/SCREENSHOTS.md`, screenshot assets.

- [x] Set the project version to 2.0.0 and document the replacement bank, engine, ADSR, section locks, all retained controls, formats, build steps, and screenshots.
- [x] Verify every image link resolves and `git diff --check` is clean.

### Task 6: Build, publish, and track CI

- [x] Build AU, VST3, LV2, and Standalone locally.
- [x] Review parameter ownership, preset completeness/count, saved-state compatibility, UI page bounds, and changed-file scope.
- [ ] Commit with personal Git identity. Before each push, run `git log --format='%h %ae %ce' origin/main..HEAD` and confirm all emails are `221119764+alanmarcero@users.noreply.github.com`.
- [ ] Push `main` and annotated tag `v2.0.0`; track the tagged GitHub Action through package and release completion.
