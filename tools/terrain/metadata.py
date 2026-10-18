"""TileJSON and attribution published next to the tiles of each prefix."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING, NamedTuple

from .heights import HEIGHT_STEP, SEA_LEVEL
from .tilemath import MAX_LATITUDE, MAX_ZOOM

if TYPE_CHECKING:
    from pathlib import Path

TILEJSON_NAME = "tilejson.json"
ATTRIBUTION_NAME = "ATTRIBUTION.txt"


class MetadataFile(NamedTuple):
    path: Path
    content_type: str


NOTICE_SEVERITIES = ("info", "critical")


class Notice(NamedTuple):
    """Why the data version changed; QGC shows it when it discards its cached tiles."""

    severity: str
    text: str

    def validate(self) -> None:
        if self.severity not in NOTICE_SEVERITIES:
            raise ValueError(f"Notice severity must be one of {', '.join(NOTICE_SEVERITIES)}")
        if not self.text.strip():
            raise ValueError("Notice text must not be empty")


ATTRIBUTION_NOTICES = (
    "Produced using Copernicus WorldDEM-30 © DLR e.V. 2010-2014 and © Airbus Defence and Space "
    "GmbH 2014-2018 provided under COPERNICUS by the European Union and ESA; all rights reserved.",
    "Produced using Copernicus WorldDEM-90 © DLR e.V. 2010-2014 and © Airbus Defence and Space "
    "GmbH 2014-2018 provided under COPERNICUS by the European Union and ESA; all rights reserved.",
)


def tilejson(
    public_url: str, prefix: str | None, data_version: str, notice: Notice | None = None
) -> dict[str, object]:
    """TileJSON 3.0.0 plus the elevation conventions consumers need (extra keys are allowed).

    ``version`` is the data version: clients key their tile caches on it, so it changes
    whenever published tiles change. ``notice`` tells users why.
    """
    base = f"{public_url}/{prefix}" if prefix else public_url
    doc: dict[str, object] = {
        "tilejson": "3.0.0",
        "name": "QGroundControl terrain",
        "version": data_version,
        "description": (
            "Copernicus DEM GLO-30 (GLO-90 where GLO-30 is unavailable) as terrarium-encoded "
            "Web Mercator tiles"
        ),
        "attribution": " ".join(ATTRIBUTION_NOTICES),
        "scheme": "xyz",
        "tiles": [f"{base}/terrarium/{{z}}/{{x}}/{{y}}.png"],
        "minzoom": 0,
        "maxzoom": MAX_ZOOM,
        "bounds": [-180.0, -MAX_LATITUDE, 180.0, MAX_LATITUDE],
        "encoding": "terrarium",
        "height_step_m": HEIGHT_STEP,
        "vertical_datum": "EGM2008",
        "surface": "DSM",
        "sea_level_m": SEA_LEVEL,
    }
    if notice is not None:
        notice.validate()
        doc["notice"] = {"severity": notice.severity, "text": notice.text}
    return doc


def write_metadata(
    out: Path,
    public_url: str,
    prefix: str | None,
    data_version: str,
    notice: Notice | None = None,
) -> list[MetadataFile]:
    """Write the metadata files for *prefix* into *out*; returns them with their content types."""
    tilejson_path = out / TILEJSON_NAME
    tilejson_path.write_text(
        json.dumps(tilejson(public_url, prefix, data_version, notice), indent=2, ensure_ascii=False)
        + "\n",
        encoding="utf-8",
    )
    attribution_path = out / ATTRIBUTION_NAME
    attribution_path.write_text("\n\n".join(ATTRIBUTION_NOTICES) + "\n", encoding="utf-8")
    # tilejson.json last: uploading in this order publishes the new version last.
    return [
        MetadataFile(attribution_path, "text/plain; charset=utf-8"),
        MetadataFile(tilejson_path, "application/json"),
    ]
