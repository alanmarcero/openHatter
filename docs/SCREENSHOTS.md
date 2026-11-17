# openHatter v4.0 screenshots

[The Sound Design page](../assets/openHatter-v4.png) is captured from the native JUCE editor snapshot at its default 1360 × 972 size. The MIDI monitor is idle because no MIDI keyboard was connected during capture. The screenshot shows the optional Hold MIDI Notes (ADSR) control in its default off state.

[The Performance page](../assets/openHatter-v4-performance.png) is captured from the native JUCE editor snapshot in `OpenHatterEditor renders every page in both control modes`. Regenerate it after a UI update with `OPENHATTER_PERFORMANCE_CAPTURE=assets/openHatter-v4-performance.png build/openhatter_plugin_tests "[plugin][gui]"`. It shows the layer velocity curves, resonator controls, MIDI choke settings, and an enabled host-synced 1/32 roll.

Regenerate the Sound Design page with `OPENHATTER_SOUND_CAPTURE=assets/openHatter-v4.png build/openhatter_plugin_tests "[plugin][gui]"`.

The Sound Design page includes the metal partial spread, damping slope, and stereo spread controls alongside the larger preset and section lock buttons.
