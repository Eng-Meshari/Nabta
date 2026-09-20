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
| Passive buzzer | I/O (S) | **GPIO 2** | PWM signal from `tone()`. See §3 before changing this. |
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

What is left on the header is **GPIO 2, 4, 12, 13, 14, 15, 16** - and 4, 12,
14, 15 all carry an SD-card function or a boot constraint. Since this prototype
never mounts the SD card, **GPIO 2 and GPIO 13** are the two cleanest pins
available, which is exactly what Nabta uses.

## 3. GPIO 2 and the strapping pins

ESP32 samples four pins at reset to decide how to boot. GPIO 2 is one of them:

| Pin | Boot role | Constraint |
|---|---|---|
| GPIO 0 | Boot mode select | Low = flash download, high/float = normal boot. Used by XCLK here - never load it. |
| GPIO 2 | Must be **low or floating** at reset | Safe for outputs that idle low. |
| GPIO 12 (MTDI) | Selects flash voltage | Must be **low** at reset; a high here can brick the flash interface. Avoid. |
| GPIO 15 (MTDO) | Silences boot log when low | Usable, but noisier to debug. |

GPIO 2 works for the buzzer because a 3-pin passive buzzer module idles its
signal input low through the driver transistor's base resistor. The firmware
also leaves the pin silent (`noTone()`) until `setup()` finishes, so nothing
pulls it high during the reset window.

**If the board refuses to enter flash mode**, disconnect the buzzer signal wire
before flashing and reconnect it afterwards - something on that module is
holding GPIO 2 high at reset.

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
        |   GPIO2  <-----I/O --------------------+
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
4. If uploads still fail, temporarily unplug the buzzer signal wire (see §3).
