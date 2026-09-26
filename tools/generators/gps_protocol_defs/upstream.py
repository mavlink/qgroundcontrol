"""Read the pinned pyubx2 and pysbf2 databases into plain data, so validation needs no upstream import."""

from __future__ import annotations

from dataclasses import dataclass
from importlib import metadata
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from collections.abc import Mapping

LICENSE = "BSD-3-Clause"


@dataclass(frozen=True)
class UBXDatabase:
    version: str
    classes: Mapping[int, str]
    messages: Mapping[tuple[int, int], str]
    # Key name -> (key id, pyubx2 type code such as "U001").
    config_keys: Mapping[str, tuple[int, str]]


@dataclass(frozen=True)
class SBFDatabase:
    version: str
    sync: bytes
    block_ids: Mapping[str, int]
    # Block name -> pysbf2 field definitions, in wire order after the 8-byte block header.
    blocks: Mapping[str, Mapping[str, Any]]


class UpstreamError(RuntimeError):
    pass


def _version(package: str) -> str:
    try:
        info = metadata.metadata(package)
    except metadata.PackageNotFoundError as error:
        raise UpstreamError(
            f"{package} is not installed; run with the gps-defs dependency group"
        ) from error
    licenses = info.get_all("License-Expression") or []
    if licenses != [LICENSE]:
        raise UpstreamError(f"{package} declares license {licenses}, not {LICENSE}")
    return info["Version"]


def load_ubx() -> UBXDatabase:
    from pyubx2.ubxtypes_configdb import UBX_CONFIG_DATABASE
    from pyubx2.ubxtypes_core import UBX_CLASSES, UBX_MSGIDS

    return UBXDatabase(
        version=_version("pyubx2"),
        classes={cls[0]: name for cls, name in UBX_CLASSES.items()},
        messages={(key[0], key[1]): name for key, name in UBX_MSGIDS.items() if len(key) == 2},
        config_keys={name: (key_id, code) for name, (key_id, code) in UBX_CONFIG_DATABASE.items()},
    )


def load_sbf() -> SBFDatabase:
    from pysbf2.sbftypes_blocks import SBF_BLOCKS
    from pysbf2.sbftypes_core import SBF_HDR, SBF_MSGIDS

    return SBFDatabase(
        version=_version("pysbf2"),
        sync=SBF_HDR,
        block_ids={name: block_id for block_id, (name, _) in SBF_MSGIDS.items()},
        blocks=SBF_BLOCKS,
    )
