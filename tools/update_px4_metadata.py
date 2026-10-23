#!/usr/bin/env python3
"""Refresh the bundled PX4 airframe and parameter metadata from artifacts.px4.io.

px4-metadata.yml runs this daily and opens a pull request when either file changes. Both downloads
are retried and validated before either bundled file is replaced, so a truncated download or an
HTML error page never reaches that pull request: the airframe XML must be well-formed, without a
DTD, under an ``airframes`` root, and the parameter catalog must decompress (xz) to a JSON object
with a non-empty ``parameters`` list. The upstream bytes are written unchanged.

PX4's upstream parameters.json only carries the schema ``"version": 1`` marker, not
``parameter_version_major``/``_minor``. As a result ``ParameterMetaData::versionFromMetaDataFile()``
returns a null version for this bundled file, which forces
``FirmwarePlugin::_cachedParameterMetaDataFile()`` to short-circuit to the bundled internal file
and skip the cache lookup entirely (FirmwarePlugin.cc, "Without a known internal version" guard).
That is intentional today: without a real version we cannot safely pick between bundle and cache.
If PX4 should ever benefit from cached metadata, stamp ``parameter_version_major``/``_minor`` here
from the PX4 release tag before writing the file, so both read and write paths have a meaningful
comparison.
"""

from __future__ import annotations

import argparse
import json
import lzma
import xml.parsers.expat
from pathlib import Path

import _bootstrap as _bootstrap
from common.gh_actions import gh_error
from common.io import atomic_write
from common.logging import log_info, log_ok
from common.net import fetch_bytes
from common.paths import REPO_ROOT

PX4_ARTIFACTS = "https://artifacts.px4.io/Firmware/_general"
AIRFRAMES_URL = f"{PX4_ARTIFACTS}/airframes.xml"
PARAMETERS_URL = f"{PX4_ARTIFACTS}/parameters.json.xz"
AIRFRAMES_FILE = Path("src/AutoPilotPlugins/PX4/AirframeFactMetaData.xml")
PARAMETERS_FILE = Path("src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.json")

FETCH_ATTEMPTS = 5
FETCH_TIMEOUT_SECONDS = 120


def _fetch(url: str) -> bytes:
    log_info(f"Downloading {url}")
    return fetch_bytes(url, timeout=FETCH_TIMEOUT_SECONDS, attempts=FETCH_ATTEMPTS)


def _reject_dtd(*_args: object) -> None:
    # Without a DTD there are no entity declarations to expand (XXE, "billion laughs").
    raise ValueError("airframe metadata must not declare a DTD")


def validate_airframes(data: bytes) -> str:
    """Return the airframe XML as text; raise ValueError unless it is well-formed airframe metadata."""
    roots: list[str] = []
    parser = xml.parsers.expat.ParserCreate()
    parser.StartDoctypeDeclHandler = _reject_dtd
    parser.StartElementHandler = lambda name, _attrs: roots.append(name)
    try:
        parser.Parse(data, True)
        text = data.decode("utf-8")
    except (xml.parsers.expat.ExpatError, UnicodeDecodeError) as error:
        raise ValueError(f"airframe metadata is not well-formed UTF-8 XML: {error}") from error
    if roots[:1] != ["airframes"]:
        raise ValueError(f"airframe metadata root is {roots[:1]}, expected ['airframes']")
    return text


def validate_parameters(data: bytes) -> str:
    """Return the decompressed parameter JSON; raise ValueError unless it lists parameters."""
    try:
        text = lzma.decompress(data).decode("utf-8")
        catalog = json.loads(text)
    except (lzma.LZMAError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"parameter metadata is not xz-compressed JSON: {error}") from error
    parameters = catalog.get("parameters") if isinstance(catalog, dict) else None
    if not isinstance(parameters, list) or not parameters:
        raise ValueError("parameter metadata has no parameters list")
    return text


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    try:
        airframes = validate_airframes(_fetch(AIRFRAMES_URL))
        parameters = validate_parameters(_fetch(PARAMETERS_URL))
    except (OSError, RuntimeError, ValueError) as error:
        gh_error(f"PX4 metadata not updated: {error}")
        return 1
    for path, text in ((AIRFRAMES_FILE, airframes), (PARAMETERS_FILE, parameters)):
        atomic_write(REPO_ROOT / path, text)
        log_ok(f"Updated {path.as_posix()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
