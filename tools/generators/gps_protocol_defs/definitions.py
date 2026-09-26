"""Resolve the manifest against the upstream databases into the definitions the headers are rendered from."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from .manifest import Manifest, SBFBlock, SBFField, UBXKey
    from .upstream import SBFDatabase, UBXDatabase

CPP_TYPES = {
    ("uint", 1): "uint8_t",
    ("uint", 2): "uint16_t",
    ("uint", 4): "uint32_t",
    ("uint", 8): "uint64_t",
    ("int", 1): "int8_t",
    ("int", 2): "int16_t",
    ("int", 4): "int32_t",
    ("int", 8): "int64_t",
    ("float", 4): "float",
    ("float", 8): "double",
}

# pyubx2 stores enums (E), bitfields (X) and booleans (L) as unsigned integers.
UBX_KINDS = {"U": "uint", "E": "uint", "X": "uint", "L": "uint", "I": "int", "R": "float"}
SBF_KINDS = {"U": "uint", "X": "uint", "I": "int", "F": "float"}

SBF_HEADER_SIZE = 8
MSGOUT_BASE_PORT = "I2C"

_FIXED_CODE = re.compile(r"[A-UW-Z]\d{3}")
_CONSTANT = re.compile(r"[A-Z][A-Z0-9_]*")
_MEMBER = re.compile(r"[a-z][A-Za-z0-9]*")
_TYPE_NAME = re.compile(r"[A-Z][A-Za-z0-9]*")


class DefinitionError(Exception):
    """Every disagreement between the manifest and upstream, reported together."""

    def __init__(self, problems: list[str]) -> None:
        super().__init__("\n".join(problems))
        self.problems = problems


@dataclass(frozen=True)
class ClassDef:
    constant: str
    value: int


@dataclass(frozen=True)
class MessageDef:
    constant: str
    class_constant: str
    id: int


@dataclass(frozen=True)
class KeyDef:
    constant: str
    id: int
    type: str
    upstream_type: str
    # QGC keeps its own type against upstream, as the manifest acknowledges.
    kept: bool = False


@dataclass(frozen=True)
class PortDef:
    name: str
    offset: int


@dataclass(frozen=True)
class MsgOutDef:
    constant: str
    i2c_id: int
    port_ids: tuple[tuple[str, int], ...]


@dataclass(frozen=True)
class BitsDef:
    accessor: str
    type: str
    mask: int
    shift: int


@dataclass(frozen=True)
class FieldDef:
    member: str
    type: str
    offset: int
    bits: tuple[BitsDef, ...]


@dataclass(frozen=True)
class BlockDef:
    name: str
    constant: str
    id: int
    size: int
    fields: tuple[FieldDef, ...]


@dataclass(frozen=True)
class Definitions:
    ubx_version: str
    sbf_version: str
    sbf_sync: bytes
    classes: tuple[ClassDef, ...]
    messages: tuple[MessageDef, ...]
    keys: tuple[KeyDef, ...]
    ports: tuple[PortDef, ...]
    msgout: tuple[MsgOutDef, ...]
    blocks: tuple[BlockDef, ...]
    # Deliberate, reviewed disagreements; printed on every run.
    notes: tuple[str, ...]


def cfg_value_bytes(key_id: int) -> int:
    """Value size from bits 28-30 of a configuration key id; 0 for a reserved size."""
    return {1: 1, 2: 1, 3: 2, 4: 4, 5: 8}.get((key_id >> 28) & 0x7, 0)


def type_size(cpp_type: str) -> int:
    return next(size for (_, size), name in CPP_TYPES.items() if name == cpp_type)


def _cpp_type(code: str, kinds: dict[str, str]) -> str | None:
    kind = kinds.get(code[:1])
    return CPP_TYPES.get((kind, int(code[1:]))) if kind and code[1:].isdigit() else None


def ubx_cpp_type(code: str) -> str | None:
    return _cpp_type(code, UBX_KINDS)


def sbf_cpp_type(code: str) -> str | None:
    return _cpp_type(code, SBF_KINDS)


def short_code(code: str) -> str:
    """ "U004" -> "U4" and "L001" -> "L", as the u-blox interface descriptions write types."""
    if code[:1] == "L":
        return "L"
    return f"{code[:1]}{int(code[1:])}" if code[1:].isdigit() else code


def upper_snake(name: str) -> str:
    """PVTGeodetic -> PVT_GEODETIC; NAV-PVT -> NAV_PVT."""
    split = re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name)
    return split.replace("-", "_").upper()


def _checked(pattern: re.Pattern[str], name: str, what: str, problems: list[str]) -> str:
    if not pattern.fullmatch(name):
        problems.append(f"{what} {name!r} is not a valid C++ identifier for this header")
    return name


class _Resolver:
    def __init__(self, ubx: UBXDatabase, sbf: SBFDatabase) -> None:
        self.ubx = ubx
        self.sbf = sbf
        self.problems: list[str] = []
        self.notes: list[str] = []

    def _problem(self, message: str) -> None:
        self.problems.append(message)

    def messages(self, manifest: Manifest) -> tuple[tuple[ClassDef, ...], tuple[MessageDef, ...]]:
        pyubx2 = f"pyubx2 {self.ubx.version}"
        by_name: dict[str, list[tuple[int, int]]] = {}
        for ids, name in self.ubx.messages.items():
            by_name.setdefault(name, []).append(ids)
        classes: dict[int, ClassDef] = {}
        messages = []
        for message in sorted(manifest.messages, key=lambda m: (m.cls, m.id)):
            ids = (message.cls, message.id)
            upstream = self.ubx.messages.get(ids)
            if upstream != message.name:
                known = ", ".join(
                    f"[0x{c:02x}, 0x{i:02x}]" for c, i in sorted(by_name.get(message.name, []))
                )
                self._problem(
                    f"UBX message {message.name}: QGC uses [0x{message.cls:02x}, 0x{message.id:02x}], "
                    + (f"{pyubx2} has {known}" if known else f"missing from {pyubx2}")
                    + (f" (that id is {upstream} upstream)" if upstream else "")
                )
                continue
            class_name = self.ubx.classes.get(message.cls, "")
            if not message.name.startswith(f"{class_name}-"):
                self._problem(
                    f"UBX message {message.name}: {pyubx2} names class 0x{message.cls:02x} {class_name!r}"
                )
                continue
            class_constant = _checked(
                _CONSTANT, upper_snake(class_name), "UBX class", self.problems
            )
            classes.setdefault(message.cls, ClassDef(class_constant, message.cls))
            messages.append(
                MessageDef(
                    _checked(_CONSTANT, upper_snake(message.name), "UBX message", self.problems),
                    class_constant,
                    message.id,
                )
            )
        return tuple(sorted(classes.values(), key=lambda c: c.value)), tuple(messages)

    def _upstream_key(self, name: str) -> tuple[int, str] | None:
        upstream = self.ubx.config_keys.get(name)
        if upstream is None:
            self._problem(f"UBX key {name}: missing from pyubx2 {self.ubx.version}")
        return upstream

    def key(self, key: UBXKey) -> KeyDef | None:
        pyubx2 = f"pyubx2 {self.ubx.version}"
        upstream = self._upstream_key(key.name)
        if upstream is None:
            return None
        upstream_id, code = upstream
        if upstream_id != key.id:
            self._problem(
                f"UBX key {key.name}: QGC uses id 0x{key.id:08x}, {pyubx2} has 0x{upstream_id:08x}"
            )
            return None
        upstream_type = ubx_cpp_type(code)
        if upstream_type is None or type_size(upstream_type) != cfg_value_bytes(upstream_id):
            self._problem(
                f"UBX key {key.name}: {pyubx2} type {code} does not fit key id 0x{upstream_id:08x}"
            )
            return None
        if key.type not in CPP_TYPES.values() or type_size(key.type) != cfg_value_bytes(key.id):
            self._problem(
                f"UBX key {key.name}: QGC type {key.type} does not fit key id 0x{key.id:08x}"
            )
            return None
        if key.upstream_type is not None:
            if key.upstream_type != code:
                self._problem(
                    f"UBX key {key.name}: reviewed against pyubx2 type {key.upstream_type}, {pyubx2} has {code}"
                )
                return None
            if upstream_type == key.type:
                self._problem(
                    f"UBX key {key.name}: upstream-type is stale; {pyubx2} type {code} is {key.type}"
                )
                return None
            self.notes.append(f"UBX key {key.name}: QGC keeps {key.type}; {pyubx2} declares {code}")
        elif upstream_type != key.type:
            self._problem(
                f"UBX key {key.name}: QGC writes {key.type}, {pyubx2} type {code} is {upstream_type}"
            )
            return None
        return KeyDef(
            _checked(_CONSTANT, key.name.removeprefix("CFG_"), "UBX key", self.problems),
            key.id,
            key.type,
            short_code(code),
            key.upstream_type is not None,
        )

    def msgout(self, manifest: Manifest) -> tuple[tuple[PortDef, ...], tuple[MsgOutDef, ...]]:
        pyubx2 = f"pyubx2 {self.ubx.version}"
        if MSGOUT_BASE_PORT in manifest.msgout_ports or len(set(manifest.msgout_ports)) != len(
            manifest.msgout_ports
        ):
            self._problem(f"UBX msgout-ports must be distinct ports other than {MSGOUT_BASE_PORT}")
        offsets: dict[str, set[int]] = {port: set() for port in manifest.msgout_ports}
        families = []
        for family in sorted(manifest.msgout, key=lambda f: f.name):
            base = self._upstream_key(f"{family.name}_{MSGOUT_BASE_PORT}")
            if base is None:
                continue
            if base != (family.i2c_id, "U001"):
                self._problem(
                    f"UBX key {family.name}_{MSGOUT_BASE_PORT}: QGC uses U1 key 0x{family.i2c_id:08x}, "
                    f"{pyubx2} has {base[1]} key 0x{base[0]:08x}"
                )
                continue
            port_ids = []
            for port in manifest.msgout_ports:
                upstream = self._upstream_key(f"{family.name}_{port}")
                if upstream is not None:
                    offsets[port].add(upstream[0] - family.i2c_id)
                    port_ids.append((port, upstream[0]))
            families.append(
                MsgOutDef(
                    _checked(_CONSTANT, family.name.removeprefix("CFG_"), "UBX key", self.problems),
                    family.i2c_id,
                    tuple(port_ids),
                )
            )
        ports = []
        for port, found in offsets.items():
            if len(found) > 1:
                self._problem(
                    f"UBX MSGOUT port {port}: {pyubx2} key offsets from I2C differ: {sorted(found)}"
                )
            elif found and next(iter(found)) <= 0:
                self._problem(f"UBX MSGOUT port {port}: {pyubx2} key does not follow the I2C key")
            elif found:
                ports.append(
                    PortDef(
                        _checked(_CONSTANT, port, "UBX MSGOUT port", self.problems), found.pop()
                    )
                )
        return tuple(sorted(ports, key=lambda p: p.offset)), tuple(families)

    def _bits(self, block: str, field: SBFField, definition: object) -> tuple[BitsDef, ...]:
        pysbf2 = f"pysbf2 {self.sbf.version}"
        if not field.bits:
            return ()
        if not (isinstance(definition, tuple) and isinstance(definition[1], dict)):
            self._problem(f"SBF {block}.{field.name}: {pysbf2} does not define it as a bitfield")
            return ()
        masks: dict[str, tuple[int, int]] = {}
        shift = 0
        for name, code in definition[1].items():
            width = int(code[1:])
            masks[name] = (((1 << width) - 1) << shift, shift)
            shift += width
        bits = []
        for bit in field.bits:
            if bit.name not in masks:
                self._problem(f"SBF {block}.{field.name}.{bit.name}: missing from {pysbf2}")
                continue
            mask, bit_shift = masks[bit.name]
            if mask != bit.mask:
                self._problem(
                    f"SBF {block}.{field.name}.{bit.name}: QGC masks 0x{bit.mask:x}, {pysbf2} has 0x{mask:x}"
                )
                continue
            bits.append(
                BitsDef(
                    _checked(_MEMBER, bit.accessor, "SBF accessor", self.problems),
                    "bool" if mask == 1 << bit_shift else field.type,
                    mask,
                    bit_shift,
                )
            )
        return tuple(bits)

    def block(self, block: SBFBlock) -> BlockDef | None:
        pysbf2 = f"pysbf2 {self.sbf.version}"
        upstream_id = self.sbf.block_ids.get(block.name)
        definition = self.sbf.blocks.get(block.name)
        if upstream_id is None or definition is None:
            self._problem(f"SBF block {block.name}: missing from {pysbf2}")
            return None
        if upstream_id != block.id:
            self._problem(
                f"SBF block {block.name}: QGC uses id {block.id}, {pysbf2} has {upstream_id}"
            )
            return None

        if not block.fields:
            self._problem(f"SBF block {block.name}: lists no fields")
            return None

        # Scalars, scaled scalars ([type, scale]) and bitfields ((X type, bits)) have fixed sizes; a
        # repeated or optional group, or a variable-size field, leaves every later offset data-dependent.
        layout: dict[str, tuple[int, str, Any]] = {}
        offset = SBF_HEADER_SIZE
        for name, entry in definition.items():
            code = entry[0] if isinstance(entry, (list, tuple)) else entry
            bitfield = isinstance(entry, tuple) and isinstance(entry[1], dict)
            if not (isinstance(code, str) and _FIXED_CODE.fullmatch(code)) or (
                isinstance(entry, tuple) and not (bitfield and code[:1] == "X")
            ):
                break
            layout[name] = (offset, code, entry)
            offset += int(code[1:])

        fields = []
        for field in block.fields:
            if field.name not in layout:
                reason = (
                    "missing from" if field.name not in definition else "not at a fixed offset in"
                )
                self._problem(f"SBF {block.name}.{field.name}: {reason} {pysbf2}")
                continue
            field_offset, code, entry = layout[field.name]
            upstream_type = sbf_cpp_type(code)
            if field_offset != field.offset or upstream_type != field.type:
                self._problem(
                    f"SBF {block.name}.{field.name}: QGC decodes {field.type} at offset {field.offset}, "
                    f"{pysbf2} has {code} ({upstream_type}) at offset {field_offset}"
                )
                continue
            fields.append(
                FieldDef(
                    _checked(_MEMBER, field.member, "SBF member", self.problems),
                    field.type,
                    field.offset,
                    self._bits(block.name, field, entry),
                )
            )
        if len(fields) != len(block.fields):
            return None
        return BlockDef(
            _checked(_TYPE_NAME, block.name, "SBF block", self.problems),
            upper_snake(block.name),
            block.id,
            max(field.offset + type_size(field.type) for field in fields),
            tuple(fields),
        )


def resolve(manifest: Manifest, ubx: UBXDatabase, sbf: SBFDatabase) -> Definitions:
    """Raise DefinitionError listing every entry missing upstream or disagreeing with it."""
    resolver = _Resolver(ubx, sbf)
    classes, messages = resolver.messages(manifest)
    keys = tuple(
        key
        for key in (resolver.key(key) for key in sorted(manifest.keys, key=lambda k: k.name))
        if key
    )
    ports, msgout = resolver.msgout(manifest)
    blocks = tuple(block for block in (resolver.block(block) for block in manifest.blocks) if block)
    if sbf.sync != b"$@":
        resolver.problems.append(f"SBF sync: pysbf2 {sbf.version} has {sbf.sync!r}, QGC uses b'$@'")
    if resolver.problems:
        raise DefinitionError(resolver.problems)
    return Definitions(
        ubx_version=ubx.version,
        sbf_version=sbf.version,
        sbf_sync=sbf.sync,
        classes=classes,
        messages=messages,
        keys=keys,
        ports=ports,
        msgout=msgout,
        blocks=blocks,
        notes=tuple(resolver.notes),
    )
