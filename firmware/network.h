// Wi-Fi lifecycle plus the multipart upload / JSON response cycle.
#pragma once

#include <Arduino.h>
#include <esp_camera.h>

// Decoded server verdict. Populated only when ok == true.
struct InferenceResult {
  bool  ok;
  char  plant_health[24];     // e.g. "healthy"
  float confidence;           // 0.0 .. 1.0
  char  action_required[24];  // e.g. "none" / "needs_water"
};

void wifiInit();

// Call every loop(). Kicks off a reconnect attempt when the link drops,
// rate-limited to WIFI_RETRY_INTERVAL. Never blocks.
void wifiUpdate();

bool wifiReady();

// POSTs the JPEG frame plus temperature/humidity as multipart/form-data and
// parses the JSON reply. Blocks for the round trip (bounded by
// HTTP_TIMEOUT_MS). Does NOT take ownership of fb - the caller still releases it.
bool uploadFrame(camera_fb_t *fb, float temperature, float humidity,
                 InferenceResult &result);
