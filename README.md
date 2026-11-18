# openHatter v4.2.0

[Install with OpenVST Manager](https://github.com/alanmarcero/alan-m-OpenVSTManager/releases/latest)

openHatter is a pitched hi-hat synthesizer. Play a note to tune six metal resonators, filtered air and a stick or pedal strike together. Use one-shots for sequenced offbeats, or hold a key to sustain the cymbal as a synth voice.

![Native openHatter editor showing the selected hat, metal and air decay, MIDI pitch and controls for the three layers](assets/openHatter-hat.png)

## Find a hat

Choose a **Factory preset** or step through the bank with **Previous** and **Next**. **Preview hat** plays a one-shot at the last incoming pitch, or C4 before the first note. The 48 new sounds span six families of eight.

| Family | Try | What to listen for |
| --- | --- | --- |
| Progressive | Silk Offbeat | A soft metal ring under a wider noise tail |
| Uplifting | Long Arc | Bright, open air between kick hits |
| Classic | Pressed Bronze | A stick strike and rounded 909-style body |
| Tech | Dry Contact | A narrow, short hat with a defined attack |
| Psy | Twin Pulses | Two close strikes with a pitched laser tick |
| Club | Main Floor | An even offbeat with a firm leading edge |

## Shape the hit

On **Hat**, set **Metal body** decay, bell focus and level to place the pitched ring. Set **Air tail** decay and low-pass cutoff for the length and brightness of the hiss. **Stick & bounce** sets the contact type, strike count, frequency and level. **Detailed controls** opens the pitch sweep, resonator spread and damping, air attack and hold, filters, and bounce timing.

The animated cymbal opens on the offbeat and follows the host through tempo changes and seeks. It rests when transport stops and uses a 138 BPM clock in Standalone. The two decay bars compare the current metal and air settings in milliseconds. The MIDI monitor shows the last played note, frequency and velocity. The factory hats use 72–158 ms metal decay and 113–242 ms air decay. Each section's **Lock** keeps its settings when browsing. The metal lock also protects the amplitude envelope. You can edit a locked control and save the lock state with your project.

## Play sustained notes

Open **Envelope** and select **Hold notes**. Attack brings the metal and air in, decay settles them at the sustain level, and a matching note-off on the same MIDI channel starts release. Release runs from zero to 20 seconds. Select **One-shot** to let the sound finish on its own. The four envelope controls also shape the one-shot within its natural tail. The selected play mode stays active when you browse presets.

![Native Envelope page in Hold notes mode with editable attack, decay, sustain and release](assets/openHatter-envelope.png)

## Performance and effects

On **Performance**, set separate velocity curves for metal, air, stick and reverb. Tune, mute or solo any of the six resonators. A note-off on the choke note (42 by default) or a high value on the choke CC (4) closes a ringing hat. CC 7 controls output volume and CC 74 controls air brightness. **Roll on** repeats the last hit at host-synced eighth, sixteenth or thirty-second notes. Set swing and a fourth-hit accent. Rolls pause with the host and end on a choke.

On **Effects**, shape the exciter, parallel compressor, limiter and reverb, add a synced reverse swell, then set master width and gain. Four optional stereo buses carry metal, air, stick and reverb for separate mixing. They include output gain and bypass shared effects and master width.

[See the detailed Hat, Performance and Effects pages.](docs/SCREENSHOTS.md)

## Changes in 4.1

The stick and pedal oscillators now fade individual overtones as they approach Nyquist. The laser strike follows a continuous downward pitch sweep and fades when it reaches the upper frequency limit. These changes also apply to stick bounces and held-note attacks. Earlier sessions retain their parameter values, MIDI behavior and section locks. The preset browser labels sounds from earlier banks **Custom sound** until you pick a new preset.

## Formats and license

The project builds VST3, LV2 and Standalone on macOS, Windows and Linux, plus AU on macOS. See the [latest release](https://github.com/alanmarcero/openHatter/releases/latest) for packaged downloads.

MIT License. See [LICENSE](LICENSE).
