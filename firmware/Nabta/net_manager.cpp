#include "net_manager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "secrets.h"

static const char *BOUNDARY = "----NabtaFormBoundary7MA4YWxkTrZu0gW";
static uint32_t lastRetryAt = 0;

void wifiInit() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // sleep mode adds latency and drops uploads mid-POST
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastRetryAt = millis();
  Serial.printf("[wifi] connecting to %s\n", WIFI_SSID);
}

void wifiUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  const uint32_t now = millis();
  if (now - lastRetryAt < WIFI_RETRY_INTERVAL) {
    return;
  }
  lastRetryAt = now;
  Serial.println("[wifi] link down, reconnecting");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool wifiReady() { return WiFi.status() == WL_CONNECTED; }

// One text field of a multipart/form-data body.
static String formField(const char *name, const String &value) {
  return String("--") + BOUNDARY + "\r\n" +
         "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n" +
         value + "\r\n";
}

static bool parseResponse(const String &payload, InferenceResult &result) {
  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[net] JSON parse failed: %s\n", err.c_str());
    return false;
  }

  const char *health = doc["plant_health"] | "";
  const char *action = doc["action_required"] | "";
  if (health[0] == '\0') {
    Serial.println("[net] response missing plant_health");
    return false;
  }

  strlcpy(result.plant_health, health, sizeof(result.plant_health));
  strlcpy(result.action_required, action, sizeof(result.action_required));
  result.confidence = doc["confidence"] | 0.0f;
  result.ok = true;
  return true;
}

bool uploadFrame(camera_fb_t *fb, float temperature, float humidity,
                 InferenceResult &result) {
  result = InferenceResult{};

  if (fb == nullptr || !wifiReady()) {
    return false;
  }

  const String head = formField("temperature", String(temperature, 1)) +
                      formField("humidity", String(humidity, 1)) +
                      String("--") + BOUNDARY + "\r\n" +
                      "Content-Disposition: form-data; name=\"file\"; "
                      "filename=\"frame.jpg\"\r\n"
                      "Content-Type: image/jpeg\r\n\r\n";
  const String tail = String("\r\n--") + BOUNDARY + "--\r\n";

  const size_t bodyLen = head.length() + fb->len + tail.length();

  // HTTPClient wants one contiguous payload, so the body is staged in PSRAM
  // (~50 kB for an SVGA frame) rather than on the 300 kB internal heap.
  uint8_t *body = (uint8_t *)(psramFound() ? ps_malloc(bodyLen) : malloc(bodyLen));
  if (body == nullptr) {
    Serial.printf("[net] cannot allocate %u byte upload buffer\n", (unsigned)bodyLen);
    return false;
  }

  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), fb->buf, fb->len);
  memcpy(body + head.length() + fb->len, tail.c_str(), tail.length());

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(SERVER_UPLOAD_URL)) {
    Serial.println("[net] http.begin failed (bad SERVER_UPLOAD_URL?)");
    free(body);
    return false;
  }
  http.addHeader("Content-Type", String("multipart/form-data; boundary=") + BOUNDARY);

  Serial.printf("[net] POST %u bytes (image %u)\n", (unsigned)bodyLen, (unsigned)fb->len);
  const int status = http.POST(body, bodyLen);
  free(body);

  bool parsed = false;
  if (status == HTTP_CODE_OK) {
    parsed = parseResponse(http.getString(), result);
  } else {
    Serial.printf("[net] upload failed, HTTP %d (%s)\n", status,
                  http.errorToString(status).c_str());
  }

  http.end();
  return parsed;
}
