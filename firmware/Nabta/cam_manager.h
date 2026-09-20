// OV2640 lifecycle: init, capture, and - importantly - frame buffer release.
#pragma once

#include <esp_camera.h>

// Configures the sensor at CAMERA_FRAME_SIZE / CAMERA_JPEG_QUALITY.
// Falls back to a smaller DRAM-only setup when no PSRAM is present.
bool cameraInit();

// Returns nullptr on failure. Every non-null result MUST be handed back to
// releaseFrame() or the driver runs out of frame buffers within a few shots.
camera_fb_t *captureFrame();

void releaseFrame(camera_fb_t *fb);
