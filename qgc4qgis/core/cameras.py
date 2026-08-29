"""Camera catalog specifications and loader for QGC4QGIS."""

import json
from dataclasses import dataclass
from pathlib import Path

from qgc4qgis.core.i18n import tr

CUSTOM_CAMERA_NAME = tr("Custom camera (manual)")


@dataclass
class CameraSpec:
    """Camera specification definition matching QGC CameraMetaData schema."""

    canonicalName: str
    brand: str = ""
    model: str = ""
    sensorWidth: float = 0.0
    sensorHeight: float = 0.0
    imageWidth: int = 0
    imageHeight: int = 0
    focalLength: float = 0.0
    landscape: bool = True
    fixedOrientation: bool = False
    minTriggerInterval: float = 0.0


def load_cameras(json_path: Path | None = None) -> list[CameraSpec]:
    """Load camera specifications catalog sorted by brand/model with synthetic manual entry."""
    if json_path is None:
        json_path = Path(__file__).resolve().parent.parent / "data" / "cameras.json"

    custom_camera = CameraSpec(
        canonicalName=CUSTOM_CAMERA_NAME,
        brand="",
        model="",
        sensorWidth=0.0,
        sensorHeight=0.0,
        imageWidth=0,
        imageHeight=0,
        focalLength=0.0,
        landscape=True,
        fixedOrientation=False,
        minTriggerInterval=0.0,
    )

    if not json_path.exists():
        return [custom_camera]

    with open(json_path, encoding="utf-8") as f:
        data = json.load(f)

    raw_cameras = data.get("cameraMetaData", [])
    catalog: list[CameraSpec] = []
    for item in raw_cameras:
        spec = CameraSpec(
            canonicalName=str(item.get("canonicalName", "")),
            brand=str(item.get("brand", "")),
            model=str(item.get("model", "")),
            sensorWidth=float(item.get("sensorWidth", 0.0)),
            sensorHeight=float(item.get("sensorHeight", 0.0)),
            imageWidth=int(item.get("imageWidth", 0)),
            imageHeight=int(item.get("imageHeight", 0)),
            focalLength=float(item.get("focalLength", 0.0)),
            landscape=bool(item.get("landscape", True)),
            fixedOrientation=bool(item.get("fixedOrientation", False)),
            minTriggerInterval=float(item.get("minTriggerInterval", 0.0)),
        )
        catalog.append(spec)

    catalog.sort(key=lambda c: (c.brand.lower(), c.model.lower(), c.canonicalName.lower()))
    return [custom_camera, *catalog]
