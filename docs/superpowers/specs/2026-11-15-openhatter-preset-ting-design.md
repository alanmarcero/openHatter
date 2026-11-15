# openHatter Factory Ting Level Design

## Intent

Reduce the tuned metallic “ting” across every factory preset because the current bank plays too loudly.

## Approaches

- Lower master output on each preset. This also reduces noise, stick attack, and effects, so it misses the requested layer.
- Change modal synthesis gain globally. This also changes manual settings and custom patches.
- Scale only the factory tuned-metal level and set the fresh-instance default to the same trim. This preserves other layers and the user control range; use this approach.

## Design

Apply a 0.25 amplitude scale (−12 dB) to `tone_level` in all 48 factory presets, and set the default `tone_level` to 0.20 to match. Keep the parameter range, MIDI behavior, and all other preset values. Update the plugin version to 3.1.0.

## Success Criteria

- All 48 factory preset `tone_level` values are at most 0.24, and a new instance defaults to 0.20.
- Existing preset and audio tests pass; local format builds and GitHub Actions release complete successfully.
