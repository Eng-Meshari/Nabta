# Nabta - Wiring Guide

Target board: **ESP32-CAM (AI-Thinker)** with OV2640 and external PSRAM,
flashed through an **ESP32-CAM-MB** micro-USB shield.

## 1. Connection table

| Module | Module pin | ESP32-CAM pin | Notes |
|---|---|---|---|
| DHT11 (3-pin breakout) | VCC | 5V | The breakout's regulator is happy at 3.3V too; 5V is more tolerant of long leads. |
| DHT11 | DATA | **GPIO 13** | Breakout boards already carry the 10 kΩ pull-up. Add one to 3V3 if you use a bare 4-pin sensor. |
| DHT11 | GND | GND | Shared ground rail. |
| Passive buzzer (3-pin) | VCC | 5V | Module has its own driver transistor; do not drive a bare buzzer from a GPIO. |
| Passive buzzer | I/O (S) | **GPIO 14** | PWM signal from `tone()`. See §3 before changing this. |
| Passive buzzer | GND | GND | Shared ground rail. |
| External PSU | +5V | 5V | 5V / 2A minimum. |
| External PSU | GND | GND | Must be common with the ESP32-CAM-MB ground. |

## 2. Why so few pins are actually free

The OV2640 occupies most of the module's I/O. These are hard-wired on the PCB
and cannot be reused:

| Function | GPIO |
|---|---|
| Camera data D0..D7 | 5, 18, 19, 21, 34, 35, 36, 39 |
| Camera clocks / sync | 0 (XCLK), 22 (PCLK), 23 (HREF), 25 (VSYNC) |
| Camera SCCB | 26 (SDA), 27 (SCL) |
| Camera power down | 32 |
| On-board flash LED | 4 |
| UART programming | 1 (TX), 3 (RX) |

What is left on the header is **GPIO 2, 4, 12, 13, 14, 15, 16** - and every
one of 2, 4, 12, 13, 14, 15 doubles as a microSD line. Since this prototype
never mounts the SD card, the deciding factors are boot straps and on-board
loads: GPIO 2, 12 and 15 are strapping pins and GPIO 4 drives the flash LED.
That leaves **GPIO 13 and GPIO 14** as the two cleanest pins, which is exactly
what Nabta uses.

## 3. Why GPIO 14 for the buzzer

The buzzer originally sat on GPIO 2 and produced an audible hiss and clicks,
loudest while the Wi-Fi radio was transmitting. It was moved to GPIO 14 for
four reasons.

**1. GPIO 14 is not a strapping pin.** ESP32 samples four pins at reset to
decide how to boot:

| Pin | Boot role | Constraint |
|---|---|---|
| GPIO 0 | Boot mode select | Low = flash download, high/float = normal boot. Used by XCLK here - never load it. |
| GPIO 2 | Must be **low or floating** at reset | A buzzer module that pulls its input high blocks flash mode. |
| GPIO 12 (MTDI) | Selects flash voltage | Must be **low** at reset; a high here can brick the flash interface. Avoid. |
| GPIO 15 (MTDO) | Silences boot log when low | Usable, but noisier to debug. |

On GPIO 2, a module that held its signal input high at reset stopped the board
entering flash mode. GPIO 14 has no boot role, so the buzzer can no longer
interfere with flashing.

**2. No shared on-board LED.** On DevKit-style ESP32 boards GPIO 2 drives the
on-board blue status LED, so a buzzer on that pin shares the line with the LED
load and its current. The AI-Thinker ESP32-CAM has no LED on GPIO 2 (its LEDs
are the white flash on GPIO 4 and the red status LED on GPIO 33). Keeping the
buzzer off GPIO 2 means the wiring stays valid if the firmware is moved to a
board that does have one.

**3. No SD pull-up on the line.** GPIO 2 is the microSD `DATA0` line. The SD
specification requires pull-ups on the `CMD` and `DATA` lines, so boards with
an SD slot typically fit one there. When the pin is released, that pull-up
drags the buzzer input toward 3V3 and half-biases the module's driver
transistor. GPIO 14 is the SD `CLK` line, which needs no pull-up. Trade-off:
mounting the SD card later (in 1-bit or 4-bit mode) will clash with the buzzer
on GPIO 14.

**4. Immune to Wi-Fi RF bursts.** The radio draws ~250 mA bursts while
transmitting, and those couple into any high-impedance trace. A signal pin
that floats, or is held only by a pull-up, picks up the bursts, and the
buzzer's driver transistor makes them audible. `buzzerInit()` now configures
the pin as a push-pull `OUTPUT` driven `LOW` (plus `noTone()`), so between
tones the line is low-impedance and RF pickup cannot bias the transistor.

GPIO 14 is also JTAG `MTMS` and may emit a short burst during the ROM boot
phase, before `setup()` runs. A brief click at reset is expected and harmless.

GPIO 13 has no strapping role at all. It is `HS2_DATA3` for the SD card and
`MTCK` for JTAG, neither of which this prototype uses, so the DHT11 owns it
outright.

## 4. Power and ground rules

- **The USB port on the ESP32-CAM-MB cannot power this build.** The OV2640 draws
  180-300 mA during capture and the Wi-Fi radio adds ~250 mA bursts; USB
  brownouts show up as `Brownout detector was triggered` or `camera init failed`
  in the serial log. Feed the 5V rail from a dedicated **5V / 2A** supply.
- **One ground, shared by everything.** The PSU ground, the ESP32-CAM ground,
  the DHT11 ground, and the buzzer ground must all meet. A DHT11 referenced to a
  different ground returns NaN readings; a buzzer referenced elsewhere either
  stays silent or injects noise into the camera's SCCB lines.
- Keep the 5V run to the camera board short and thick. Voltage sag on a thin
  breadboard jumper is the single most common cause of intermittent capture
  failures on this module.
- Add a 470-1000 µF electrolytic across the 5V/GND rail near the board if
  captures fail only when Wi-Fi transmits.

## 5. Schematic sketch

```
              +5V 2A PSU
                 |    |
        +--------+    +---------+---------------+
        |                       |               |
     [5V] ESP32-CAM          [VCC] DHT11     [VCC] Buzzer
        |   GPIO13 <-----DATA----+               |
        |   GPIO14 <-----I/O --------------------+
        |                       |               |
     [GND]--------------------[GND]-----------[GND]   <-- common ground
        |
     ESP32-CAM-MB (USB, programming + serial monitor only)
```

## 6. Flashing checklist

1. Seat the ESP32-CAM on the ESP32-CAM-MB shield (camera connector facing away
   from the USB port).
2. Keep the external 5V supply connected during flashing; the shield's USB
   supplies data and serial, not the current budget.
3. Press **RST** on the shield if the upload does not start - the MB shield
   auto-toggles boot mode, but a marginal supply can miss the handshake.
4. The buzzer on GPIO 14 has no boot role (see §3), so if uploads still fail,
   look at the supply and the shield, not the peripherals.
