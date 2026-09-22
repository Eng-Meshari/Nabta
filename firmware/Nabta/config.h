// Central tuning table for the Nabta prototype.
// Everything a bench technician may want to change lives here.
#pragma once

// ---------------------------------------------------------------- peripherals
#define PIN_DHT_DATA            13   // DHT11 data line (free: SD_DATA3, SD unused)
#define PIN_BUZZER              14   // Passive buzzer signal (see docs/WIRING.md)
#define DHT_SENSOR_TYPE       DHT11

// -------------------------------------------------------------------- timings
#define SERIAL_BAUD_RATE     115200
#define DHT_POLL_INTERVAL      5000  // ms between temperature/humidity samples
#define CAPTURE_INTERVAL      30000  // ms between camera capture + upload cycles
#define WIFI_RETRY_INTERVAL    5000  // ms between reconnect attempts
#define HTTP_TIMEOUT_MS       15000  // upload + inference round trip budget

// --------------------------------------------------------------------- camera
#define CAMERA_FRAME_SIZE  FRAMESIZE_SVGA  // 800x600
#define CAMERA_JPEG_QUALITY      12        // 10 (best) .. 63 (worst)
#define CAMERA_FB_COUNT_PSRAM     2
#define CAMERA_FB_COUNT_DRAM      1

// --------------------------------------------------------------- buzzer tones
// Frequencies in Hz, durations in ms. A frequency of 0 means "rest".
#define TONE_BOOT_LOW           880   // A5
#define TONE_BOOT_MID          1175   // D6
#define TONE_BOOT_HIGH         1568   // G6
#define TONE_BOOT_STEP_MS       120

#define TONE_SUCCESS_LOW       1568   // G6
#define TONE_SUCCESS_HIGH      2093   // C7
#define TONE_SUCCESS_STEP_MS     90

#define TONE_ALERT             2637   // E7 - deliberately piercing
#define TONE_ALERT_ON_MS        180
#define TONE_ALERT_OFF_MS       120
#define TONE_ALERT_REPEATS        3

// ------------------------------------------------------------------ behaviour
// Server responses whose "action_required" differs from this trigger the alarm.
#define ACTION_NONE            "none"
