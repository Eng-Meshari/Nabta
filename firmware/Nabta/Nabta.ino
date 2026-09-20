/*
 * Nabta - ESP32-CAM edge-to-server plant health monitor.
 *
 * loop() owns three independent millis() timers and never blocks:
 *   - sensorsUpdate()  polls the DHT11 every DHT_POLL_INTERVAL
 *   - captureAndUpload() runs every CAPTURE_INTERVAL
 *   - buzzerUpdate()   advances whatever tone pattern is playing
 *
 * The only bounded blocking call is the HTTP round trip inside uploadFrame().
 */

#include "buzzer.h"
#include "cam_manager.h"
#include "config.h"
#include "net_manager.h"
#include "sensors.h"

static uint32_t lastCaptureAt = 0;
static bool cameraReady = false;

static void captureAndUpload() {
  const SensorData env = sensorsLatest();
  if (!env.valid) {
    Serial.println("[loop] skipping upload, no valid DHT11 reading yet");
    return;
  }

  camera_fb_t *fb = captureFrame();
  if (fb == nullptr) {
    return;
  }

  InferenceResult result;
  const bool ok = uploadFrame(fb, env.temperature, env.humidity, result);

  // Released immediately after the POST so the driver always has a spare
  // buffer for the next cycle, whatever the server answered.
  releaseFrame(fb);

  if (!ok) {
    return;
  }

  Serial.printf("[loop] health=%s confidence=%.2f action=%s\n",
                result.plant_health, result.confidence, result.action_required);

  if (strcmp(result.action_required, ACTION_NONE) == 0) {
    playSuccessChime();
  } else {
    playAlertTone();
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  buzzerInit();
  sensorsInit();
  cameraReady = cameraInit();
  wifiInit();

  if (cameraReady) {
    playBootChime();
  } else {
    playAlertTone();  // audible signal that the camera never came up
  }

  // Offset the first capture so the DHT11 has produced a reading by then.
  lastCaptureAt = millis() - CAPTURE_INTERVAL + DHT_POLL_INTERVAL;
}

void loop() {
  buzzerUpdate();
  wifiUpdate();

  if (sensorsUpdate()) {
    const SensorData env = sensorsLatest();
    Serial.printf("[env] %.1f C / %.1f %%RH\n", env.temperature, env.humidity);
  }

  if (cameraReady && wifiReady() && millis() - lastCaptureAt >= CAPTURE_INTERVAL) {
    lastCaptureAt = millis();
    captureAndUpload();
  }
}
