// DHT11 temperature + humidity acquisition, throttled with millis().
#pragma once

#include <Arduino.h>

struct SensorData {
  float temperature;  // degrees Celsius
  float humidity;     // relative humidity, percent
  bool  valid;        // false until the first successful read
};

void sensorsInit();

// Call every loop(). Takes a new sample only once DHT_POLL_INTERVAL has
// elapsed. Returns true on the loop iteration where a fresh sample landed.
bool sensorsUpdate();

// Last known reading. Check .valid before using the numbers.
SensorData sensorsLatest();
