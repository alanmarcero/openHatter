# Native openHatter editor

The 1060 × 866 captures come from the JUCE editor. The Hat page shows a MIDI F♯4 at velocity 104. Its layered cymbal is a bundled SVG animated from the musical clock. The Envelope capture shows a held voice with a 20-second release.

![Hat page](../assets/openHatter-hat.png)

![Detailed hat controls](../assets/openHatter-detail.png)

![Held-note envelope](../assets/openHatter-envelope.png)

![Velocity, roll, choke and resonator settings](../assets/openHatter-performance.png)

![Exciter, dynamics, reverb and master settings](../assets/openHatter-effects.png)

Regenerate all five captures from the repository root with a release build of the plug-in tests.

```sh
OPENHATTER_CAPTURE_DIR="$PWD/assets" build/openhatter_plugin_tests '[plugin][gui]'
```
