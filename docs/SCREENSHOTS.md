# openHatter v3.3 screenshots

[The Sound Design page](../assets/openHatter-v3.png) is captured from the native JUCE editor snapshot at its default 1100 × 972 size. The MIDI monitor is idle because no MIDI keyboard was connected during capture. The screenshot shows the optional Full ADSR control in its default off state.

[The Performance page](../assets/openHatter-performance.png) is captured from the native JUCE editor snapshot in `OpenHatterEditor renders every page in both control modes`. Regenerate it after a UI update with `OPENHATTER_PERFORMANCE_CAPTURE=assets/openHatter-performance.png build-task/openhatter_plugin_tests "[plugin][gui]"`. It shows the layer velocity curves, resonator controls, MIDI choke settings, and an enabled host-synced 1/32 roll.

Regenerate the Sound Design page with `OPENHATTER_SOUND_CAPTURE=assets/openHatter-v3.png build-task/openhatter_plugin_tests "[plugin][gui]"`.

The [advanced Sound Design page](../assets/openHatter-hat-advanced.png) shows the new metal partial spread, damping slope, and stereo spread controls.
