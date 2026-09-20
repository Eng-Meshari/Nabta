// Non-blocking driver for the 3-pin passive buzzer module.
// Patterns are played as a sequence of (frequency, duration) steps advanced
// from buzzerUpdate(); nothing here ever calls delay().
#pragma once

#include <Arduino.h>

void buzzerInit();

void playBootChime();     // rising triad on successful boot
void playSuccessChime();  // short two-note blip on a good server response
void playAlertTone();     // repeated piercing beeps when action is required

// Call every loop() to advance the current pattern.
void buzzerUpdate();

bool buzzerBusy();
