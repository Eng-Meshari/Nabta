#include "cam_manager.h"

#include <Arduino.h>

#include "camera_pins.h"
#include "config.h"

bool cameraInit() {
  camera_config_t cfg = {};

  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM;
  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM;
  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM;
  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM;
  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk = XCLK_GPIO_NUM;
  cfg.pin_pclk = PCLK_GPIO_NUM;
  cfg.pin_vsync = VSYNC_GPIO_NUM;
  cfg.pin_href = HREF_GPIO_NUM;
  cfg.pin_sccb_sda = SIOD_GPIO_NUM;
  cfg.pin_sccb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn = PWDN_GPIO_NUM;
  cfg.pin_reset = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;

  // With PSRAM we can afford SVGA plus a second buffer, which lets the driver
  // keep grabbing while we are busy uploading the previous frame.
  if (psramFound()) {
    cfg.frame_size = CAMERA_FRAME_SIZE;
    cfg.jpeg_quality = CAMERA_JPEG_QUALITY;
    cfg.fb_count = CAMERA_FB_COUNT_PSRAM;
    cfg.fb_location = CAMERA_FB_IN_PSRAM;
    cfg.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // No PSRAM: internal DRAM cannot hold an SVGA JPEG reliably.
    Serial.println("[camera] no PSRAM detected, falling back to SVGA/DRAM single buffer");
    cfg.frame_size = FRAMESIZE_SVGA;
    cfg.jpeg_quality = 15;
    cfg.fb_count = CAMERA_FB_COUNT_DRAM;
    cfg.fb_location = CAMERA_FB_IN_DRAM;
    cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  const esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("[camera] esp_camera_init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    // OV2640 modules on the AI-Thinker board are mounted upside down.
    sensor->set_vflip(sensor, 1);
    sensor->set_hmirror(sensor, 1);
  }

  Serial.printf("[camera] ready (PSRAM: %s)\n", psramFound() ? "yes" : "no");
  return true;
}

camera_fb_t *captureFrame() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb == nullptr) {
    Serial.println("[camera] capture failed");
    return nullptr;
  }
  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("[camera] unexpected pixel format, dropping frame");
    esp_camera_fb_return(fb);
    return nullptr;
  }
  return fb;
}

void releaseFrame(camera_fb_t *fb) {
  if (fb != nullptr) {
    esp_camera_fb_return(fb);
  }
}
