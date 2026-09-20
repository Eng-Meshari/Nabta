"""Nabta mock inference server.

Receives multipart uploads from the ESP32-CAM, archives the frame for visual
verification, and answers with the payload the firmware expects.

Run with:  uvicorn main:app --host 0.0.0.0 --port 8000
"""

from __future__ import annotations

from datetime import datetime
from pathlib import Path

from fastapi import FastAPI, Form, UploadFile

from mock_inference import analyze

UPLOAD_DIR = Path(__file__).parent / "uploads"
UPLOAD_DIR.mkdir(parents=True, exist_ok=True)

app = FastAPI(title="Nabta Mock Inference Server")


@app.get("/")
def health() -> dict:
    """Liveness probe - handy for confirming the ESP32 can reach this host."""
    return {"status": "ok", "uploads": len(list(UPLOAD_DIR.glob("*.jpg")))}


@app.post("/api/analyze")
async def analyze_frame(
    file: UploadFile,
    temperature: float = Form(...),
    humidity: float = Form(...),
) -> dict:
    image_bytes = await file.read()

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    saved_as = UPLOAD_DIR / f"{stamp}.jpg"
    saved_as.write_bytes(image_bytes)

    result = analyze(image_bytes)
    print(
        f"[{stamp}] {len(image_bytes)} bytes | {temperature:.1f} C | "
        f"{humidity:.1f} %RH -> {result['plant_health']} "
        f"({result['confidence']:.2f}) / {result['action_required']}"
    )

    # Echo the environment back so the saved frames can be correlated later.
    return {**result, "temperature": temperature, "humidity": humidity}
