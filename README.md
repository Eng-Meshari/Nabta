# Nabta

ESP32-CAM edge-to-server plant health monitoring prototype.

The board samples temperature and humidity, photographs the plant on a fixed
interval, POSTs the frame plus the environment readings to an inference server,
and gives acoustic feedback from the verdict that comes back.

## Architecture

```
  ESP32-CAM (AI-Thinker)                       Host machine
  +----------------------------+               +---------------------------+
  |  DHT11  -> GPIO 13         |   Wi-Fi       |  FastAPI  main.py         |
  |  OV2640 -> SVGA JPEG       |  multipart    |    POST /api/analyze      |
  |  Buzzer -> GPIO 2          |  ==========>  |      |                    |
  |                            |               |      +-> uploads/*.jpg    |
  |  Nabta.ino (non-blocking)  |   JSON        |      +-> mock_inference   |
  |   sensors / cam / network  |  <==========  |                           |
  |   buzzer feedback          |               |                           |
  +----------------------------+               +---------------------------+
```

Response contract:

```json
{"status": "success", "plant_health": "healthy", "confidence": 0.95, "action_required": "none"}
```

`action_required == "none"` plays the success chime; anything else
(`"needs_water"`, `"recapture"`, ...) plays the alert tone.

## Bill of materials

| # | Item | Spec | Notes |
|---|---|---|---|
| 1 | ESP32-CAM | AI-Thinker, OV2640, 4 MB external PSRAM | PSRAM is required for SVGA capture |
| 2 | ESP32-CAM-MB | Micro-USB programmer shield | Programming and serial monitor only |
| 3 | DHT11 module | 3-pin breakout with on-board pull-up | Data on GPIO 13 |
| 4 | Passive buzzer module | 3-pin, on-board driver transistor | Signal on GPIO 2 |
| 5 | Power supply | 5V, 2A minimum | Feeds the 5V/GND rail directly |
| 6 | Jumper wires | Female-female | Common ground across all modules |

Wiring, pin justification and power rules: **[docs/WIRING.md](docs/WIRING.md)**.

## Repository layout

```
firmware/          ESP32 sketch and modules
  Nabta.ino        non-blocking orchestrator loop
  config.h         pins, intervals, tone table - tune here
  camera_pins.h    AI-Thinker pin map (fixed by the PCB)
  sensors.*        DHT11 reads, millis()-throttled
  buzzer.*         non-blocking tone pattern player
  cam_manager.*    camera init, capture, frame buffer release
  network.*        Wi-Fi reconnect, multipart upload, JSON parse
  secrets.h.example  copy to secrets.h and fill in
server/            mock inference service
  main.py          FastAPI app, POST /api/analyze
  mock_inference.py  image heuristic standing in for YOLO
  uploads/         received frames, timestamped
docs/WIRING.md     wiring guide
```

## Firmware setup

### 1. Toolchain

Arduino IDE 2.x with **esp32 core 3.x** by Espressif
(Boards Manager URL: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`).

Library Manager dependencies:

- `DHT sensor library` (Adafruit) - pulls in `Adafruit Unified Sensor`
- `ArduinoJson` v7 (Benoit Blanchon)

`esp_camera.h` ships with the core; nothing extra to install for the OV2640.

### 2. Credentials

```bash
cp firmware/secrets.h.example firmware/secrets.h
```

Edit `firmware/secrets.h` with your SSID, password, and the **LAN IP** of the
machine running the server (not `localhost` - the ESP32 resolves it itself).
`secrets.h` is git-ignored.

### 3. Open the sketch

The Arduino IDE requires the sketch folder to match the `.ino` name. Either
rename `firmware/` to `Nabta/`, or open it from the CLI:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32cam firmware/Nabta.ino
```

### 4. Board settings

| Setting | Value |
|---|---|
| Board | AI Thinker ESP32-CAM |
| PSRAM | Enabled |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| Upload Speed | 115200 |
| CPU Frequency | 240 MHz |

Then flash over the ESP32-CAM-MB shield with the external 5V supply connected.

## Server setup

```bash
cd server
python -m venv venv
source venv/bin/activate          # Windows: venv\Scripts\activate
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 8000
```

`--host 0.0.0.0` is not optional: bound to `127.0.0.1` the server is invisible
to the ESP32. Allow port 8000 through the host firewall on first run.

Check the heuristic on its own, without a board:

```bash
python mock_inference.py     # runs the built-in self-check
```

## Prototype validation workflow

1. **Server reachable** - from another device on the LAN, open
   `http://<host-ip>:8000/`. A JSON `{"status": "ok", ...}` means the ESP32 can
   reach it too.
2. **Boot chime** - power the board. A rising three-note chime means the camera
   initialised. An immediate alert tone means `esp_camera_init` failed; check
   the 5V supply first, then the PSRAM setting.
3. **Serial monitor at 115200** - expect, in order:
   ```
   [camera] ready (PSRAM: yes)
   [wifi] connecting to <ssid>
   [env] 24.3 C / 41.0 %RH
   [net] POST 51380 bytes (image 51204)
   [loop] health=healthy confidence=0.99 action=none
   ```
4. **Environment sanity** - breathe on the DHT11; humidity should climb within
   two poll cycles (10 s). Persistent `DHT11 read failed` means a wiring or
   common-ground problem, not a code problem.
5. **Frames landing** - `server/uploads/` fills with timestamped JPEGs, one per
   30 s cycle. Open them: a black or striped frame is a power problem.
6. **Acoustic feedback** - point the camera at something green, then at
   something dull or yellow. The verdict, and therefore the chime, should flip
   between success and alert.

## Known limitations

- `mock_inference.analyze()` is a green-dominance heuristic, not a model. The
  JSON shape is the contract - swap the function body when the real YOLO
  weights land.
- `uploadFrame()` blocks for the HTTP round trip (bounded by `HTTP_TIMEOUT_MS`).
  Everything else in `loop()` is timer-driven and non-blocking.
- No TLS and no auth on the upload endpoint; this is a LAN prototype.
