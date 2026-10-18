"""Height conventions of the published tiles, shared by the encoder and the metadata (stdlib only)."""

from __future__ import annotations

SEA_LEVEL = 0.0
# Stored height resolution. The vehicle terrain protocol rounds to whole meters again, so this
# shifts ~4% of TERRAIN_DATA heights by one meter (never more, no bias) while removing the
# sub-decimeter noise that makes up most of a tile's compressed size; see README.md.
HEIGHT_STEP = 0.25
