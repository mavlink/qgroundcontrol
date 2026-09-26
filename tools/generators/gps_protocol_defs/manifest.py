"""Typed view of manifest.toml: the protocol definitions QGC uses, with the values it uses today."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

from common.io import read_toml

from generators.common.validation import clamped_repr, reject_unknown_keys, require_dict

MANIFEST_PATH = Path(__file__).with_name("manifest.toml")


@dataclass(frozen=True)
class UBXMessage:
    name: str
    cls: int
    id: int


@dataclass(frozen=True)
class UBXKey:
    name: str
    id: int
    type: str
    # The upstream type a deliberate disagreement with QGC's type was reviewed against.
    upstream_type: str | None = None


@dataclass(frozen=True)
class MsgOutFamily:
    name: str
    i2c_id: int


@dataclass(frozen=True)
class SBFBits:
    name: str
    accessor: str
    mask: int


@dataclass(frozen=True)
class SBFField:
    name: str
    member: str
    type: str
    offset: int
    bits: tuple[SBFBits, ...] = ()


@dataclass(frozen=True)
class SBFBlock:
    name: str
    id: int
    fields: tuple[SBFField, ...]


@dataclass(frozen=True)
class Manifest:
    messages: tuple[UBXMessage, ...]
    keys: tuple[UBXKey, ...]
    msgout_ports: tuple[str, ...]
    msgout: tuple[MsgOutFamily, ...]
    blocks: tuple[SBFBlock, ...]


def _int(value: object, context: str, source: object) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(
            f"{source}: {context} must be a non-negative integer, got {clamped_repr(value)}"
        )
    return value


def _str(value: object, context: str, source: object) -> str:
    if not isinstance(value, str) or not value:
        raise ValueError(
            f"{source}: {context} must be a non-empty string, got {clamped_repr(value)}"
        )
    return value


def _table(data: dict[str, Any], key: str, context: str, source: object) -> dict[str, Any]:
    return require_dict(data.get(key, {}), f"{context}.{key}", source)


def _messages(ubx: dict[str, Any], source: object) -> tuple[UBXMessage, ...]:
    messages = []
    for name, value in _table(ubx, "messages", "ubx", source).items():
        if not isinstance(value, list) or len(value) != 2:
            raise ValueError(
                f"{source}: ubx.messages.{name} must be [class, id], got {clamped_repr(value)}"
            )
        cls, id_ = (_int(part, f"ubx.messages.{name}", source) for part in value)
        messages.append(UBXMessage(name, cls, id_))
    return tuple(messages)


def _keys(ubx: dict[str, Any], source: object) -> tuple[UBXKey, ...]:
    keys = []
    for name, value in _table(ubx, "keys", "ubx", source).items():
        context = f"ubx.keys.{name}"
        entry = require_dict(value, context, source)
        reject_unknown_keys(entry, frozenset({"id", "type", "upstream-type"}), context, source)
        upstream_type = entry.get("upstream-type")
        keys.append(
            UBXKey(
                name,
                _int(entry.get("id"), f"{context}.id", source),
                _str(entry.get("type"), f"{context}.type", source),
                None
                if upstream_type is None
                else _str(upstream_type, f"{context}.upstream-type", source),
            )
        )
    return tuple(keys)


def _block(name: str, value: object, source: object) -> SBFBlock:
    context = f"sbf.blocks.{name}"
    entry = require_dict(value, context, source)
    reject_unknown_keys(entry, frozenset({"id", "fields", "bits"}), context, source)
    bits = _table(entry, "bits", context, source)
    fields = []
    for field_name, field_value in _table(entry, "fields", context, source).items():
        field_context = f"{context}.fields.{field_name}"
        field = require_dict(field_value, field_context, source)
        reject_unknown_keys(field, frozenset({"member", "type", "offset"}), field_context, source)
        field_bits = []
        for bit_name, bit_value in require_dict(
            bits.pop(field_name, {}), f"{context}.bits.{field_name}", source
        ).items():
            bit_context = f"{context}.bits.{field_name}.{bit_name}"
            bit = require_dict(bit_value, bit_context, source)
            reject_unknown_keys(bit, frozenset({"accessor", "mask"}), bit_context, source)
            field_bits.append(
                SBFBits(
                    bit_name,
                    _str(bit.get("accessor"), f"{bit_context}.accessor", source),
                    _int(bit.get("mask"), f"{bit_context}.mask", source),
                )
            )
        fields.append(
            SBFField(
                field_name,
                _str(field.get("member"), f"{field_context}.member", source),
                _str(field.get("type"), f"{field_context}.type", source),
                _int(field.get("offset"), f"{field_context}.offset", source),
                tuple(field_bits),
            )
        )
    if bits:
        raise ValueError(
            f"{source}: {context}.bits names unlisted field(s): {', '.join(sorted(bits))}"
        )
    return SBFBlock(name, _int(entry.get("id"), f"{context}.id", source), tuple(fields))


def parse_manifest(data: dict[str, Any], source: object = MANIFEST_PATH) -> Manifest:
    """Validate the manifest's shape; values are checked against upstream by definitions.resolve()."""
    reject_unknown_keys(data, frozenset({"ubx", "sbf"}), "manifest", source)
    ubx = _table(data, "ubx", "manifest", source)
    reject_unknown_keys(
        ubx, frozenset({"msgout-ports", "messages", "keys", "msgout"}), "ubx", source
    )
    sbf = _table(data, "sbf", "manifest", source)
    reject_unknown_keys(sbf, frozenset({"blocks"}), "sbf", source)

    ports = ubx.get("msgout-ports", [])
    if not isinstance(ports, list):
        raise ValueError(f"{source}: ubx.msgout-ports must be a list, got {clamped_repr(ports)}")
    msgout = tuple(
        MsgOutFamily(name, _int(value, f"ubx.msgout.{name}", source))
        for name, value in _table(ubx, "msgout", "ubx", source).items()
    )
    return Manifest(
        messages=_messages(ubx, source),
        keys=_keys(ubx, source),
        msgout_ports=tuple(_str(port, "ubx.msgout-ports entry", source) for port in ports),
        msgout=msgout,
        blocks=tuple(
            _block(name, value, source)
            for name, value in _table(sbf, "blocks", "sbf", source).items()
        ),
    )


def load_manifest(path: Path = MANIFEST_PATH) -> Manifest:
    return parse_manifest(read_toml(path), path)
