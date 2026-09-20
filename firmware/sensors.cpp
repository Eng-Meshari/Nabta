#include "sensors.h"

#include <DHT.h>

#include "config.h"

static DHT dht(PIN_DHT_DATA, DHT_SENSOR_TYPE);
static SensorData latest = {0.0f, 0.0f, false};
static uint32_t lastPoll = 0;

void sensorsInit() {
  dht.begin();
  // Force the first sample to happen immediately on the first sensorsUpdate().
  lastPoll = millis() - DHT_POLL_INTERVAL;
}

bool sensorsUpdate() {
  const uint32_t now = millis();
  if (now - lastPoll < DHT_POLL_INTERVAL) {
    return false;
  }
  lastPoll = now;

  // The bit-banged DHT11 transaction itself occupies ~25 ms; the millis()
  // gate above keeps that cost off the loop for the other 4975 ms.
  const float h = dht.readHumidity();
  const float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("[sensors] DHT11 read failed, keeping previous value");
    return false;
  }

  latest.temperature = t;
  latest.humidity = h;
  latest.valid = true;
  return true;
}

SensorData sensorsLatest() { return latest; }
