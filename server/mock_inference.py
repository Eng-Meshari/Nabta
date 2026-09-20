"""Mock plant-health inference.

Stands in for the real YOLO model until it exists. The verdict is derived from
cheap image statistics so the firmware sees plausible, image-dependent answers
instead of a constant: a leaf that is green and well exposed reads "healthy",
a dull or yellowing one reads "needs_water".
"""

from __future__ import annotations

import io

from PIL import Image, ImageStat

# ponytail: green-dominance heuristic, swap the body of analyze() for a real
# YOLO call when the model lands - the return shape is the contract.
GREEN_DOMINANCE_THRESHOLD = 1.15  # mean(G) / mean(R) above this reads healthy
MIN_PLAUSIBLE_BYTES = 2048  # smaller than this is a truncated / failed capture


def analyze(image_bytes: bytes) -> dict:
    """Return the detection payload for one JPEG frame."""
    if len(image_bytes) < MIN_PLAUSIBLE_BYTES:
        return {
            "status": "error",
            "plant_health": "unknown",
            "confidence": 0.0,
            "action_required": "recapture",
        }

    try:
        image = Image.open(io.BytesIO(image_bytes))
        image.load()
    except Exception:
        return {
            "status": "error",
            "plant_health": "unknown",
            "confidence": 0.0,
            "action_required": "recapture",
        }

    width, height = image.size
    red, green, _blue = ImageStat.Stat(image.convert("RGB")).mean
    # Green vs red separates a healthy leaf from a yellowing one; blue is
    # mostly background/sky here and only adds noise.
    dominance = green / max(red, 1.0)

    healthy = dominance >= GREEN_DOMINANCE_THRESHOLD
    # Confidence grows with how far the frame sits from the decision boundary,
    # and with resolution (a bigger frame gives the real model more to work with).
    margin = min(abs(dominance - GREEN_DOMINANCE_THRESHOLD) / 0.30, 1.0)
    resolution_bonus = 0.05 if width * height >= 640 * 480 else 0.0
    confidence = round(min(0.60 + 0.35 * margin + resolution_bonus, 0.99), 2)

    return {
        "status": "success",
        "plant_health": "healthy" if healthy else "stressed",
        "confidence": confidence,
        "action_required": "none" if healthy else "needs_water",
    }


def _jpeg(color: tuple[int, int, int], size: tuple[int, int] = (800, 600)) -> bytes:
    buf = io.BytesIO()
    Image.new("RGB", size, color).save(buf, format="JPEG")
    return buf.getvalue()


if __name__ == "__main__":
    healthy = analyze(_jpeg((40, 160, 50)))
    assert healthy["plant_health"] == "healthy", healthy
    assert healthy["action_required"] == "none", healthy

    stressed = analyze(_jpeg((180, 160, 40)))
    assert stressed["plant_health"] == "stressed", stressed
    assert stressed["action_required"] == "needs_water", stressed

    broken = analyze(b"not-a-jpeg")
    assert broken["status"] == "error", broken

    for payload in (healthy, stressed, broken):
        assert 0.0 <= payload["confidence"] <= 1.0, payload
        assert set(payload) == {
            "status",
            "plant_health",
            "confidence",
            "action_required",
        }, payload

    print("mock_inference self-check passed")
    print(healthy)
    print(stressed)
