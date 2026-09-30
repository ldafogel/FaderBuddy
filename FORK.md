# Configurable haptics fork

This public fork tracks `scottbez1/FaderBuddy`; the custom implementation is on
`codex/configurable-haptics`. Upstream base: `3d44d7d`. License remains Apache-2.0.
This is experimental firmware, not a claim of validated motor feel.

## Build and validate

Use PlatformIO Core 6.1.19 (megaAVR platform pinned to 1.10.0):

```sh
firmware/test/haptics/run.sh
pio run -e fb_app_only
python firmware/tools/export_app_image.py --no-build --output build/faderbuddy-haptics-1.128.bin
```

The bootloader-compatible image starts at `0x600`, retains the metadata footer
at `0x3ffe`, and does not change the bootloader, fuses, or EEPROM calibration
layout. Application version is **1.128**. Feature-detect `LFH\x01`, not a version
comparison: this extension is independent of future upstream versions.

Flash is nearly full. To fit, this build omits **double-tap recognition** (nonce
remains zero), retains capacitive touch sensing, and uses smaller integer legacy
haptic calculations. Legacy detent force quantization can differ by one ADC
count; calibration uses exact 199/200 endpoint weighting instead of floats.
Ordinary targets, speed, calibration, layers and native haptic modes remain.
The standalone legacy build retains double-tap recognition; it starts at zero
and must **not** be used as an I2C application update.

The tests exercise the production spring controller, including force polarity,
limits, damping, detent choice/hysteresis, slew, endpoint protection and lease
expiry across clock wrap. An illustrative friction/inertia simulation checks
return toward center. These tests do not establish actual stability, temperature,
return precision, or the achieved 2 kHz cadence on hardware.

## Extension protocol

All fields are bytes. Protocol v5 registers stay compatible.

| Register | Operation |
|---|---|
| `0x30` | Read four bytes: `4c 46 48 01` (`LFH`, extension version 1). |
| `0x31` | Write register plus exactly 19 configuration bytes below. |
| `0x32` | Read one byte: 1 = active lease, 0 = inactive. Write `[0x32,1]` to renew an existing lease. |

Configuration payload:

| Index | Meaning |
|---|---|
| 0–2 | Version 1, active layer 0–7, detent count 0–10 (0 disables). |
| 3 | Spring stiffness, PWM per normalized travel unit /16; nonzero when enabled. |
| 4 | Velocity damping, PWM per normalized unit/second /256. |
| 5 | Maximum PWM, 1–254 when enabled. |
| 6 | Dead zone, 0–32 travel counts. |
| 7 | Capture radius in travel counts; 0 attracts over full travel. |
| 8 | Maximum PWM change per control tick, 1–255 when enabled (nominal tick 500 µs). |
| 9–18 | Up to ten strictly increasing positions, 0–255 calibrated travel. Unused bytes ignored. |

Internally full travel is 1020 normalized units. Thus position/radius/dead-zone
bytes represent four units each. Force is proportional to error outside a soft
dead zone, minus velocity damping, then force- and slew-limited. No integral
windup or minimum-duty kick. The bridge uses slow-decay PWM. Detent selection
has two travel counts of boundary hysteresis.

Configuration is volatile and accepted only in Active/Idle on the active layer,
with native haptics set to zero and no deferred layer change. Clear native
haptics first so lease expiry cannot silently resume an older force field.
It overrides native haptics and remains active while touched **and** released.
Renew every 100 ms; after 500 ms without renewal it coasts. Renewal cannot revive
an expired lease: reconfiguration is explicit. Targets, layer changes, legacy
haptic writes, fault clears, calibration and bootloader entry cancel the custom
session. These commands win if received in the same main-loop batch as a custom
configuration. Separate legacy-clear and custom-config writes by a later poll.
Errors/calibration never run the spring. Invalid packets are ignored without
refreshing the lease. Read status on a later poll; an I2C ACK is not activation.

## Bench and updates

First isolate show-control trigger output, then update and exercise one board.
Verify bootloader availability and board identity before choosing I2C versus
UPDI. Use the application image with an I2C updater; ordinary ESP32 OTA does not
update the ATtiny. Preserve calibration and have the upstream image available.
Do not use the factory chip-erase/fuse-changing upload for an application update.

For upstream changes:

```sh
git fetch upstream
git switch codex/configurable-haptics
git merge upstream/main
```

Re-run tests, build and inspect the final flash layout before publishing a new
image. Merge upstream source into this branch; installing a stock binary would
remove the custom haptics extension.
