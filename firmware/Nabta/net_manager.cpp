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
  static bool announced = false;
  if (WiFi.status() == WL_CONNECTED) {
    if (!announced) {
      announced = true;
      // An IP outside the server's subnet or a weak RSSI (below about -75 dBm
      // on the ESP32-CAM PCB antenna) explains most failed uploads.
      Serial.printf("[wifi] connected, ip=%s gateway=%s rssi=%d dBm\n",
                    WiFi.localIP().toString().c_str(),
                    WiFi.gatewayIP().toString().c_str(), WiFi.RSSI());
    }
    return;
  }
  announced = false;
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

// Cheap GET against the server's health route ("/") before the large POST, so
// an unreachable server is reported on its own instead of as a failed upload.
static bool pingServer() {
  const String uploadUrl = SERVER_UPLOAD_URL;
  // Keep scheme + host + port, i.e. everything before the first path slash.
  const String healthUrl = uploadUrl.substring(0, uploadUrl.indexOf('/', strlen("http://"))) + "/";

  HTTPClient http;
  http.setConnectTimeout(HTTP_PING_TIMEOUT_MS);
  http.setTimeout(HTTP_PING_TIMEOUT_MS);
  if (!http.begin(healthUrl)) {
    Serial.printf("[net] ping: http.begin failed for %s\n", healthUrl.c_str());
    return false;
  }

  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[net] ping %s failed, HTTP %d %s (rssi=%d dBm)\n", healthUrl.c_str(),
                  httpCode, http.errorToString(httpCode).c_str(), WiFi.RSSI());
  }
  http.end();
  return httpCode == HTTP_CODE_OK;
}

bool uploadFrame(camera_fb_t *fb, float temperature, float humidity,
                 InferenceResult &result) {
  result = InferenceResult{};

  if (fb == nullptr || !wifiReady() || !pingServer()) {
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
  const int httpCode = http.POST(body, bodyLen);
  free(body);

  // Negative codes are transport errors (no HTTP response), see the table in
  // HTTPClient.h: -1 connect failed, -3 payload send failed, -11 read timeout.
  const String response = http.getString();
  Serial.printf("[net] HTTP response code: %d\n", httpCode);
  Serial.printf("[net] HTTP response body: %s\n", response.c_str());
  if (httpCode < 0) {
    Serial.printf("[net] transport error: %s\n", http.errorToString(httpCode).c_str());
  }

  const bool parsed = httpCode == HTTP_CODE_OK && parseResponse(response, result);
  http.end();
  return parsed;
}
