#include "buzzer.h"

#include "config.h"

struct Step {
  uint16_t freq;  // Hz, 0 = silent rest
  uint16_t ms;
};

static const Step BOOT_CHIME[] = {
    {TONE_BOOT_LOW, TONE_BOOT_STEP_MS},
    {TONE_BOOT_MID, TONE_BOOT_STEP_MS},
    {TONE_BOOT_HIGH, TONE_BOOT_STEP_MS * 2},
};

static const Step SUCCESS_CHIME[] = {
    {TONE_SUCCESS_LOW, TONE_SUCCESS_STEP_MS},
    {TONE_SUCCESS_HIGH, TONE_SUCCESS_STEP_MS},
};

// TONE_ALERT_REPEATS beep/rest pairs, written out so the sequence player
// stays a plain array walk.
static const Step ALERT_TONE[] = {
    {TONE_ALERT, TONE_ALERT_ON_MS}, {0, TONE_ALERT_OFF_MS},
    {TONE_ALERT, TONE_ALERT_ON_MS}, {0, TONE_ALERT_OFF_MS},
    {TONE_ALERT, TONE_ALERT_ON_MS}, {0, TONE_ALERT_OFF_MS},
};

static const Step *sequence = nullptr;
static uint8_t sequenceLength = 0;
static uint8_t stepIndex = 0;
static uint32_t stepStartedAt = 0;

static void applyStep() {
  const uint16_t freq = sequence[stepIndex].freq;
  if (freq > 0) {
    tone(PIN_BUZZER, freq);
  } else {
    noTone(PIN_BUZZER);
  }
  stepStartedAt = millis();
}

static void startPattern(const Step *steps, uint8_t length) {
  sequence = steps;
  sequenceLength = length;
  stepIndex = 0;
  applyStep();
}

void buzzerInit() {
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);
}

void playBootChime() {
  startPattern(BOOT_CHIME, sizeof(BOOT_CHIME) / sizeof(BOOT_CHIME[0]));
}

void playSuccessChime() {
  startPattern(SUCCESS_CHIME, sizeof(SUCCESS_CHIME) / sizeof(SUCCESS_CHIME[0]));
}

void playAlertTone() {
  startPattern(ALERT_TONE, sizeof(ALERT_TONE) / sizeof(ALERT_TONE[0]));
}

void buzzerUpdate() {
  if (sequence == nullptr) {
    return;
  }
  if (millis() - stepStartedAt < sequence[stepIndex].ms) {
    return;
  }

  stepIndex++;
  if (stepIndex >= sequenceLength) {
    noTone(PIN_BUZZER);
    sequence = nullptr;
    return;
  }
  applyStep();
}

bool buzzerBusy() { return sequence != nullptr; }
