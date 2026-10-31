# Sweep

ZMK user config for a Ferris Sweep (`cradio` shield) on nice!nano v2 boards.
A third nice!nano v2 is the dongle: it is the split central. It talks to the
computer over USB only. Both halves are peripherals. They connect to the
dongle over Bluetooth and do not connect to the computer.

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
5. On the computer, remove the old "Cradio" Bluetooth device if it is still
   listed. The dongle does not advertise as a Bluetooth keyboard. It is the
   keyboard as soon as the USB cable is plugged in.

A keymap-only change needs a new `cradio_dongle.uf2` on the dongle. The halves
do not run the keymap. Flash a half again only when its own config changes
(sleep, debounce, or the peripheral role).

## Battery

The dongle fetches both halves. The computer is on USB, and USB HID does not
carry battery levels, so the OS battery icon will not show them.

Hold the two layer thumbs together (nav and sym, which turns on the number
layer) and tap the left space thumb. The dongle types the levels at the
cursor, for example `0 85% 1 72%`. `0` is the first half that paired to the
dongle and `1` is the second. That order is not left and right. The
characters follow the computer's keyboard layout, same as the number keys.
A display or LED on the dongle is not part of this firmware.
