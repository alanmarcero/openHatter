# openHatter Preset Ting Level Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use $executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Lower the tuned metallic component across the openHatter factory bank.

**Architecture:** Scale the preset-only `tone_level` value at the preset construction point and lower the fresh-instance default. Keep audio synthesis and manual parameter range unchanged.

**Tech Stack:** C++20, JUCE 8, CMake, Catch2.

**Spec:** `docs/superpowers/specs/2026-11-15-openhatter-preset-ting-design.md`

## Global Constraints

- Keep all 48 preset names, IDs, and non-metal values.
- Apply a 0.25 preset scale and set the default to 0.20; keep the `tone_level` range unchanged.
- Update the project to 3.1.0 and track the tagged CI release.

## Review Focus

- Every factory preset must load a scaled `tone_level` no higher than 0.24.
- A fresh instance must start with `tone_level` at 0.20.
- Preset selection must leave all unrelated parameters and the manual range unchanged.
- The toned-down sound must remain audible and finite in the existing preset render test.
- Release packaging must keep the existing plugin formats and identity.

---

### Task 1: Trim the factory tuned-metal layer

**Files:** `Tests/PluginTests.cpp`, `Source/Plugin/Presets.cpp`, `Source/Plugin/Parameters.cpp`, `CMakeLists.txt`, `README.md`

**Interfaces:** Test through the existing `OpenHatterProcessor` APVTS and `setCurrentProgram()` interfaces.

- [ ] Add a test requiring the fresh-instance tone level to be 0.20 and every factory preset tone level to be positive and no higher than 0.24.
- [ ] Run the focused test and confirm it fails on the current values.
- [ ] Scale the factory preset tone level by 0.25, set the default to 0.20, and update the version and release note.
- [ ] Run the full CTest suite and build VST3, AU, LV2, and Standalone.
- [ ] Review the diff, commit with the personal Git identity, push `main` and `v3.1.0`, then track the workflow to a successful release.
