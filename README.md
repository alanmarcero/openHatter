# openHatter v3

[Install with OpenVST Manager](https://github.com/alanmarcero/alan-m-OpenVSTManager/releases/latest)

openHatter is a MIDI-tracked open hi-hat synthesizer. Play any MIDI note and the synth follows its pitch and octave directly. The editor shows the received note, MIDI number, velocity, and calculated frequency. There are no pitch or octave selectors.

![openHatter v3 editor](assets/openHatter-v3.png)

Version 3 replaces the metallic voice and editor, and introduces 48 new presets across Progressive, Uplifting, Classic, Tech, Psy, and Club styles. The sound combines an inharmonic modal cymbal bank, shaped noise, stick bounces, transient controls, ADSR, exciter, dynamics, gated space, and reverse swells.

Available as VST3, AU, LV2, and Standalone on macOS, Windows, and Linux.

## MIDI pitch tracking

Every MIDI note-on triggers the sound at the note's equal-tempered frequency:

```text
frequency = 440 × 2 ^ ((MIDI note − 69) / 12)
```

The first modal partial follows that frequency. Upper partials use the selected cymbal mode's inharmonic ratios, and partials above Nyquist are omitted. MIDI note-offs do not shorten the one-shot hat tail. Velocity scales each hit.

The note monitor reports the most recent incoming note. **Audition** plays that note, or MIDI note 60 (C4) before any MIDI note arrives. MIDI CC 7 controls output volume and CC 74 controls air brightness.

## Controls

| Section | Controls |
|---|---|
| Metal Resonator | Cymbal mode, bell focus, resonance, decay, upper-mode sweep and time, level |
| Air Tail | Texture, attack, hold, decay, curve, high-pass, low-pass, stereo width, level |
| Bounce / Groove | Bounce count, spread, decay, humanize, pan, level |
| Envelope / Chick | Amplitude attack, decay, sustain, release; chick type, level, frequency |
| Exciter | Mode, drive, sheen, mix |
| Dynamics | Compressor threshold, ratio, attack, release, makeup, mix; limiter ceiling, release, enable |
| Space / Output | Reverb size, decay, damping, choke, mix; reverse sync and mix; stereo width and output |

The six preset sections can be locked while browsing. A lock keeps that section's values when another preset is selected; its controls remain editable. Lock states are saved with the project.

## Factory presets

All v2 factory programs have been replaced. The 48 new presets are grouped by style:

| Style | Presets |
|---|---|
| Progressive | Glassline, Night Current, Long Exposure, Paper Lantern, Meridian, Soft Focus, Copper Thread, Wide Stillness |
| Uplifting | Skywall, Radiant Spray, Climb Marker, Last Chorus, High Window, Blue Arpeggio, Summit Bloom, Open Sky |
| Classic | Tape Bronze, Warm Press, 909 Chamber, Soft Machine, Polished Rim, Backroom Velvet, Edge of Vinyl, Warm Room |
| Tech | Hard Relay, Grid Slicer, Steel Staccato, Notch Driver, Dry Circuit, Torque Stack, Voltage Bridge, Small Machine |
| Psy | Prism Orbit, Fractal Dust, Jungle Glass, Phase Bloom, Molecular, Starfield, Spiral Ratchet, Deep Sun |
| Club | Neutral Open, Tight Eighths, Wide Hall, Small Room, Felt Brush, Bright Cut, Subtle Air, Reference Plate |

## State and compatibility

The plugin identity, format IDs, parameter IDs, and project parameter state remain compatible with v2 sessions. Legacy root note, octave, fine tune, pitch lock, MIDI mode, and WAV-lock parameter IDs remain for state migration; they do not select the live voice's pitch. Any cached v2 WAV render is discarded. MIDI input always sets the sounding pitch.

## Build

Requires CMake 3.22 or newer and a C++20 compiler. JUCE 8 and Catch2 are fetched by CMake.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Repository layout

| Path | Contents |
|---|---|
| `Source/Core` | C++20 synthesis, effects, and MIDI engine |
| `Source/Plugin` | JUCE parameters, state, presets, editor, and controls |
| `Tests` | Catch2 unit and plugin tests |
| `assets` | Plugin artwork and editor screenshots |
| `docs` | Audio design and screenshot notes |
| `.github/workflows` | Multi-platform build, package, and release workflow |

## License

MIT License. Copyright (c) 2026 Alan Marcero. JUCE is dual-licensed under AGPL-3.0 or a commercial license.
