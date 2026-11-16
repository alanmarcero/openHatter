# Commercial feature research: openHatter

## Reference

Sugar Bytes' official DrumComputer manual describes multiple synthesis engines per voice, a multimode filter, modulation generators, and engine-level sequencing controls. Its official product page highlights per-sound randomization profiles and sound variation.

- [DrumComputer manual](https://downloads.sugar-bytes.de/manuals/DrumComputer.pdf)
- [DrumComputer product page](https://sugar-bytes.de/en/drumcomputer-ipad)

These are sound-design references only. openHatter keeps its own modal cymbal synthesis and preset content.

## Capability gaps and implemented changes

1. **Metal-partial spread.** The existing six resonators could be tuned separately, but had no macro spread. Added an automatable ±24-semitone spread across upper partials; zero preserves the prior voicing.
2. **High-mode damping slope.** The resonators used a fixed relative damping profile. Added an automatable slope that shortens or extends upper-mode decay relative to the fundamental.
3. **Per-partial stereo spread.** The modal bank used a fixed phase offset. Added an automatable 0–200% control that scales stereo dispersion across upper partials while keeping the fundamental centered.

## Verification

- Release AU, VST3, LV2, and Standalone targets built successfully.
- CTest passed: **55/55**.
- Refreshed and inspected assets/openHatter-hat-advanced.png and assets/openHatter-performance.png. The advanced Sound Design capture shows the partial spread, damping slope, and stereo controls; the controls fit their section.
