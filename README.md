# Sweep

ZMK user config for a Ferris Sweep (`cradio` shield) on nice!nano v2 boards.
A third nice!nano v2 is the dongle: it is the split central and talks to the
computer over USB and Bluetooth. Both halves are peripherals. They pair to the
dongle and do not pair to the computer.

The keymap, layers, and behaviors are `config/cradio.keymap`, shared by the
dongle and the halves. GitHub Actions builds four firmware files from
`build.yaml`: `cradio_dongle`, `cradio_left`, `cradio_right`, and
`settings_reset`.

`nice_nano//zmk` in `build.yaml` is the nice!nano v2. Current ZMK renamed the
old `nice_nano_v2` board id; revision 2.0.0 is the default.

## Flashing

Download the `firmware` artifact from the GitHub Actions run. Put a nice!nano
in the bootloader by double-tapping its reset button. It shows up as a USB
drive. Copy the matching `.uf2` onto that drive.

| File | Board |
| --- | --- |
| `cradio_dongle.uf2` | Dongle (the nice!nano that plugs into the computer) |
| `cradio_left.uf2` | Left half |
| `cradio_right.uf2` | Right half |
| `settings_reset.uf2` | Any of the three, only to clear bonds |

### First flash, and any time the central changes

The halves still have bonds from when the left half was the central. Clear
all three boards before flashing the new firmware. The reset image does not
advertise, so flash it to every board before flashing the real firmware.

1. Put the dongle, the left half, and the right half into the bootloader.
2. Copy `settings_reset.uf2` to each of the three. Finish all three resets
   before continuing.
3. Copy `cradio_dongle.uf2` to the dongle, `cradio_left.uf2` to the left half,
   and `cradio_right.uf2` to the right half.
4. Plug the dongle into USB, then power both halves. They pair to the dongle
   on their own. If a half does not connect, reset that half and the dongle
   at the same time (reset button, or power them off and on together).
5. On the computer, remove the old "Cradio" Bluetooth device and pair again
   if you use Bluetooth. Over USB, the dongle is the keyboard as soon as it
   is plugged in.

A keymap-only change needs a new `cradio_dongle.uf2` on the dongle. The halves
do not run the keymap. Flash a half again only when its own config changes
(sleep, debounce, or the peripheral role).

## Battery

The dongle fetches both halves and publishes them on a second Bluetooth
Battery Service, as `Peripheral 0` and `Peripheral 1` (pairing order, not
left/right). USB HID does not carry those levels. Over Bluetooth, the system
battery icon shows the dongle's own reading. Reading both halves takes a tool
that opens the extra Battery Service. A display or LED on the dongle is not
part of this firmware.
