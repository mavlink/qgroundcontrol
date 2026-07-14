"""Validated loader for the shared QGC Docker variant manifest."""

from __future__ import annotations

import json
from pathlib import Path
from typing import TypedDict, cast

VARIANTS_JSON = Path(__file__).resolve().parent / "variants.json"


class DockerVariant(TypedDict):
    id: str
    ci_variant: str
    platform: str
    security_category: str
    selector: str
    target: str
    image: str
    fuse: bool
    artifact_pattern: str
    package_pattern: str
    build_args: dict[str, str]


_STRING_FIELDS = (
    "id",
    "ci_variant",
    "platform",
    "security_category",
    "selector",
    "target",
    "image",
    "artifact_pattern",
    "package_pattern",
)
_REQUIRED_FIELDS = {*_STRING_FIELDS, "fuse", "build_args"}
_SELECTORS = {"linux", "android"}


def load_variants(path: Path = VARIANTS_JSON) -> list[DockerVariant]:
    """Load Docker variants and reject malformed or ambiguous definitions."""
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or not isinstance(payload.get("variants"), list):
        raise ValueError(f"{path} must contain a top-level 'variants' list")

    variants: list[DockerVariant] = []
    seen_ids: set[str] = set()
    seen_security_categories: set[str] = set()
    for index, value in enumerate(payload["variants"]):
        context = f"{path}: variant {index}"
        if not isinstance(value, dict):
            raise ValueError(f"{context} must be an object")
        missing = sorted(_REQUIRED_FIELDS - value.keys())
        if missing:
            raise ValueError(f"{context} is missing required fields: {', '.join(missing)}")
        for field in _STRING_FIELDS:
            if not isinstance(value[field], str):
                raise ValueError(f"{context}.{field} must be a string")
        if not isinstance(value["fuse"], bool):
            raise ValueError(f"{context}.fuse must be a boolean")
        build_args = value["build_args"]
        if not isinstance(build_args, dict) or not all(
            isinstance(key, str) and isinstance(item, str) for key, item in build_args.items()
        ):
            raise ValueError(f"{context}.build_args must map strings to strings")
        if value["selector"] not in _SELECTORS:
            raise ValueError(f"{context}.selector must be one of: {', '.join(sorted(_SELECTORS))}")
        variant_id = value["id"]
        if variant_id in seen_ids:
            raise ValueError(f"{context}.id duplicates {variant_id!r}")
        seen_ids.add(variant_id)
        security_category = value["security_category"]
        if security_category in seen_security_categories:
            raise ValueError(f"{context}.security_category duplicates {security_category!r}")
        seen_security_categories.add(security_category)
        variants.append(cast("DockerVariant", value))
    return variants
