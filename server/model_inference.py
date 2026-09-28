"""YOLOv8 plant-health inference.

Runs the trained detector (best.pt, classes: healthy / stressed) on one
ESP32-CAM frame and collapses its boxes into the payload the firmware expects.
Every analyzed frame is also saved to uploads/annotated_<timestamp>.jpg with
the boxes drawn on it, for visual inspection.
"""

from __future__ import annotations

import io
from datetime import datetime
from pathlib import Path

from PIL import Image
from ultralytics import YOLO

SERVER_DIR = Path(__file__).parent
UPLOAD_DIR = SERVER_DIR / "uploads"
MODEL = YOLO(SERVER_DIR / "best.pt")

# Tuning knobs: the ESP32-CAM is noisy, adjust these against real frames.
MIN_DETECTION_CONF = 0.25  # boxes below this are ignored entirely
STRESS_ALERT_CONF = 0.60  # a single stressed box this confident triggers watering

ERROR = {
    "status": "error",
    "plant_health": "unknown",
    "confidence": 0.0,
    "action_required": "recapture",
}


def analyze(image_bytes: bytes) -> dict:
    """Return the detection payload for one JPEG frame."""
    try:
        # convert() forces a full decode, so a truncated capture fails here.
        image = Image.open(io.BytesIO(image_bytes)).convert("RGB")
        result = MODEL.predict(image, conf=MIN_DETECTION_CONF, verbose=False)[0]
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        result.save(filename=str(UPLOAD_DIR / f"annotated_{stamp}.jpg"))
    except Exception as exc:
        print(f"inference failed: {exc!r}")
        return dict(ERROR)

    labels = [result.names[int(c)] for c in result.boxes.cls]
    return verdict(labels, result.boxes.conf.tolist())


def verdict(labels: list[str], confs: list[float]) -> dict:
    """Collapse per-box detections into one plant-health verdict."""
    if not labels:
        return dict(ERROR)  # no plant in frame

    stressed = [c for label, c in zip(labels, confs) if label == "stressed"]
    healthy = [c for label, c in zip(labels, confs) if label == "healthy"]

    if len(stressed) > len(healthy) or max(stressed, default=0.0) >= STRESS_ALERT_CONF:
        return {
            "status": "success",
            "plant_health": "stressed",
            "confidence": round(max(stressed), 2),
            "action_required": "needs_water",
        }
    # Reaching here means healthy boxes are at least as many as stressed ones,
    # so the list is never empty.
    return {
        "status": "success",
        "plant_health": "healthy",
        "confidence": round(max(healthy), 2),
        "action_required": "none",
    }


if __name__ == "__main__":
    assert verdict([], [])["status"] == "error"
    assert verdict(["healthy"], [0.9])["plant_health"] == "healthy"
    assert verdict(["stressed", "stressed", "healthy"], [0.3, 0.4, 0.9]) == {
        "status": "success",
        "plant_health": "stressed",
        "confidence": 0.4,
        "action_required": "needs_water",
    }
    # Outnumbered, but confident enough to trigger watering on its own.
    assert verdict(["healthy", "healthy", "stressed"], [0.8, 0.7, 0.65])["plant_health"] == "stressed"
    # Tie with a weak stressed box stays healthy.
    assert verdict(["healthy", "stressed"], [0.5, 0.4])["plant_health"] == "healthy"
    assert analyze(b"not-a-jpeg") == ERROR
    print("model_inference self-check passed")
