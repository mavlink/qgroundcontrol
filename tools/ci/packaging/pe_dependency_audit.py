#!/usr/bin/env python3
"""Check that every DLL an installed Windows QGroundControl imports ships with it or is part of Windows.

Reads the import and delay-load import tables of every .exe and .dll under the install directory and
fails when an imported DLL is neither in the install's bin/ or beside the importing file nor a
Windows system DLL. The MSVC runtime never counts as a system DLL: a runner has it in System32, a
clean Windows does not.

On Windows a system DLL is one in System32 (--system-dir); elsewhere, such as for an installer
extracted with 7-Zip on Linux, it is one of KNOWN_SYSTEM_DLLS or --assume-system. API sets
(api-ms-win-*, ext-ms-*) are always system DLLs.
"""

from __future__ import annotations

import argparse
import mmap
import os
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, write_step_summary
from common.markdown import code_span, md_table, more_items, summary_section

PE_SUFFIXES = (".dll", ".exe")
IMPORT_DIRECTORY = 1
DELAY_IMPORT_DIRECTORY = 13
# Corrupt tables can lack their terminating entry; no real image imports from this many DLLs.
MAX_DESCRIPTORS = 4096
SUMMARY_ROWS = 50
API_SET_PREFIXES = ("api-ms-win-", "ext-ms-")
# The Visual C++ redistributable (CMake's InstallRequiredSystemLibraries) and the debug UCRT.
REDISTRIBUTABLE_RE = re.compile(
    r"(vcruntime|msvcp|msvcr|concrt|vccorlib|vcomp|vcamp|mfc|mfcm)\d+(_\w+)?d?\.dll|ucrtbased\.dll"
)
# The Windows 10 1809+ System32 DLLs that the x64 and arm64 installs import, for audits off Windows;
# --assume-system adds others.
KNOWN_SYSTEM_DLLS = frozenset(
    {
        "advapi32.dll",
        "authz.dll",
        "avrt.dll",
        "bcrypt.dll",
        "cabinet.dll",
        "comdlg32.dll",
        "crypt32.dll",
        "d2d1.dll",
        "d3d11.dll",
        "d3d12.dll",
        "d3d9.dll",
        "dnsapi.dll",
        "dwmapi.dll",
        "dwrite.dll",
        "dxgi.dll",
        "dxva2.dll",
        "evr.dll",
        "gdi32.dll",
        "icuuc.dll",
        "imagehlp.dll",
        "imm32.dll",
        "iphlpapi.dll",
        "kernel32.dll",
        "mf.dll",
        "mfplat.dll",
        "mfreadwrite.dll",
        "mpr.dll",
        "mscms.dll",
        "msi.dll",
        "ncrypt.dll",
        "netapi32.dll",
        "ntdll.dll",
        "odbc32.dll",
        "ole32.dll",
        "oleaut32.dll",
        "opengl32.dll",
        "propsys.dll",
        "rpcrt4.dll",
        "secur32.dll",
        "setupapi.dll",
        "shell32.dll",
        "shlwapi.dll",
        "uiautomationcore.dll",
        "user32.dll",
        "userenv.dll",
        "uxtheme.dll",
        "version.dll",
        "winhttp.dll",
        "wininet.dll",
        "winmm.dll",
        "wintrust.dll",
        "ws2_32.dll",
        "wtsapi32.dll",
    }
)


class PEFormatError(ValueError):
    """The file is not a PE image, or its headers point outside it."""


@dataclass(frozen=True)
class Imports:
    """The DLL names a PE image imports at load time and on first use (delay-load)."""

    normal: tuple[str, ...]
    delayed: tuple[str, ...]


@dataclass(frozen=True)
class Missing:
    importer: str
    dll: str
    delayed: bool


class _Image:
    def __init__(self, data: bytes | mmap.mmap) -> None:
        self.data = data
        if len(data) < 0x40 or data[:2] != b"MZ":
            raise PEFormatError("no MZ header")
        pe = self._u32(0x3C)
        if data[pe : pe + 4] != b"PE\0\0":
            raise PEFormatError("no PE signature")
        sections, optional_size = self._unpack("<2xH12xH", pe + 4)
        optional = pe + 24
        magic = self._unpack("<H", optional)[0]
        if magic == 0x10B:
            self.image_base = self._u32(optional + 28)
            directory_count = self._u32(optional + 92)
            directories = optional + 96
        elif magic == 0x20B:
            self.image_base = self._unpack("<Q", optional + 24)[0]
            directory_count = self._u32(optional + 108)
            directories = optional + 112
        else:
            raise PEFormatError(f"unknown optional header magic 0x{magic:X}")
        self.directories = [
            self._unpack("<II", directories + 8 * index)
            for index in range(min(directory_count, 16))
        ]
        table = optional + optional_size
        self.sections = [self._unpack("<8xIIII", table + 40 * index) for index in range(sections)]

    def _unpack(self, fmt: str, offset: int) -> tuple[int, ...]:
        try:
            return struct.unpack_from(fmt, self.data, offset)
        except struct.error as error:
            raise PEFormatError(f"truncated at 0x{offset:X}") from error

    def _u32(self, offset: int) -> int:
        return self._unpack("<I", offset)[0]

    def offset(self, rva: int) -> int:
        for virtual_size, address, raw_size, raw_offset in self.sections:
            if address <= rva < address + max(virtual_size, raw_size):
                return raw_offset + rva - address
        if self.sections and rva < min(section[1] for section in self.sections):
            return rva
        raise PEFormatError(f"RVA 0x{rva:X} is in no section")

    def string(self, rva: int) -> str:
        start = self.offset(rva)
        end = self.data.find(b"\0", start, start + 512)
        if end < 0:
            raise PEFormatError(f"unterminated name at RVA 0x{rva:X}")
        return bytes(self.data[start:end]).decode("ascii", errors="replace")

    def directory(self, index: int) -> int:
        rva, size = self.directories[index] if index < len(self.directories) else (0, 0)
        return self.offset(rva) if rva and size else -1

    def imports(self) -> tuple[str, ...]:
        names = []
        start = self.directory(IMPORT_DIRECTORY)
        for index in range(MAX_DESCRIPTORS if start >= 0 else 0):
            # IMAGE_IMPORT_DESCRIPTOR: OriginalFirstThunk, TimeDateStamp, ForwarderChain, Name, FirstThunk
            descriptor = self._unpack("<5I", start + 20 * index)
            if not any(descriptor):
                break
            names.append(self.string(descriptor[3]))
        return tuple(names)

    def delay_imports(self) -> tuple[str, ...]:
        names = []
        start = self.directory(DELAY_IMPORT_DIRECTORY)
        for index in range(MAX_DESCRIPTORS if start >= 0 else 0):
            # IMAGE_DELAYLOAD_DESCRIPTOR starts with Attributes and DllNameRVA.
            attributes, name = self._unpack("<II", start + 32 * index)
            if not name:
                break
            # Attribute bit 0 clear: a pre-VC7 descriptor holding virtual addresses.
            names.append(self.string(name if attributes & 1 else name - self.image_base))
        return tuple(names)


def read_imports(data: bytes | mmap.mmap) -> Imports:
    """Return the DLLs the PE image *data* imports; raise PEFormatError for anything else."""
    image = _Image(data)
    return Imports(image.imports(), image.delay_imports())


def read_file_imports(path: Path) -> Imports:
    with path.open("rb") as stream:
        if os.fstat(stream.fileno()).st_size == 0:
            raise PEFormatError("empty file")
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            return read_imports(data)


def is_redistributable(name: str) -> bool:
    return REDISTRIBUTABLE_RE.fullmatch(name.lower()) is not None


def _names_in(directory: Path) -> frozenset[str]:
    try:
        return frozenset(entry.name.lower() for entry in directory.iterdir() if entry.is_file())
    except OSError:
        return frozenset()


class SystemDlls:
    """Decide which DLL names Windows provides, from system directories or a list of names."""

    def __init__(self, system_dirs: list[Path], assumed: list[str]) -> None:
        names = {name.lower() for name in assumed}
        if system_dirs:
            for directory in system_dirs:
                names |= _names_in(directory)
        else:
            names |= KNOWN_SYSTEM_DLLS
        self.names = frozenset(names)

    def __contains__(self, name: str) -> bool:
        name = name.lower()
        if name.startswith(API_SET_PREFIXES):
            return True
        return name in self.names and not is_redistributable(name)


def pe_files(install_dir: Path) -> list[Path]:
    return sorted(
        path
        for path in install_dir.rglob("*")
        if path.suffix.lower() in PE_SUFFIXES and path.is_file()
    )


def default_system_dirs() -> list[Path]:
    """Return System32 on Windows; elsewhere nothing, which selects KNOWN_SYSTEM_DLLS."""
    if sys.platform != "win32":
        return []
    return [Path(os.environ.get("SYSTEMROOT", r"C:\Windows")) / "System32"]


def audit(install_dir: Path, system: SystemDlls) -> tuple[int, list[Missing], list[str]]:
    """Return the number of files read, the unresolved imports, and the files that aren't PE images."""
    bin_names = _names_in(install_dir / "bin")
    directory_names: dict[Path, frozenset[str]] = {}
    missing: list[Missing] = []
    unreadable: list[str] = []
    files = pe_files(install_dir)
    for path in files:
        relative = path.relative_to(install_dir).as_posix()
        try:
            imports = read_file_imports(path)
        except (OSError, PEFormatError) as error:
            unreadable.append(f"{relative}: {error}")
            continue
        local = directory_names.setdefault(path.parent, _names_in(path.parent))
        for names, delayed in ((imports.normal, False), (imports.delayed, True)):
            for name in names:
                lowered = name.lower()
                if lowered in bin_names or lowered in local or name in system:
                    continue
                missing.append(Missing(relative, name, delayed))
    return len(files), missing, unreadable


def _summary(
    install_dir: Path, files: int, missing: list[Missing], unreadable: list[str], source: str
) -> str:
    result = "passed" if not missing and not unreadable else "failed"
    facts = [
        ("Result", result),
        ("Install directory", code_span(install_dir)),
        ("Files checked", files),
        ("System DLLs", source),
        ("Missing imports", len(missing)),
        ("Unreadable files", len(unreadable) or None),
    ]
    body = ""
    if missing:
        rows = [
            (
                code_span(item.importer),
                code_span(item.dll),
                "delay-load" if item.delayed else "load",
            )
            for item in missing[:SUMMARY_ROWS]
        ]
        body = md_table(["File", "Missing DLL", "Import"], rows, markdown=True)
        if len(missing) > SUMMARY_ROWS:
            body += f"\n\n{more_items(len(missing) - SUMMARY_ROWS)}"
    return summary_section("DLL dependency audit", facts, body=body)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install-dir", type=Path, required=True)
    parser.add_argument(
        "--system-dir",
        type=Path,
        action="append",
        default=[],
        help="Directory of Windows system DLLs (repeatable; default on Windows: System32)",
    )
    parser.add_argument(
        "--assume-system",
        action="append",
        default=[],
        metavar="DLL",
        help="Also treat this DLL name as a Windows system DLL (repeatable)",
    )
    args = parser.parse_args(argv)
    if not args.install_dir.is_dir():
        gh_error(f"Install directory not found: {args.install_dir}")
        return 1
    system_dirs = args.system_dir or default_system_dirs()
    for directory in system_dirs:
        if not directory.is_dir():
            gh_error(f"System DLL directory not found: {directory}")
            return 1

    system = SystemDlls(system_dirs, args.assume_system)
    source = ", ".join(map(code_span, system_dirs)) or "built-in list"
    files, missing, unreadable = audit(args.install_dir, system)
    for item in missing:
        kind = " (delay-load)" if item.delayed else ""
        print(f"{item.importer} -> {item.dll}{kind}")
    for item in unreadable:
        print(f"Not a PE image: {item}")
    write_step_summary(_summary(args.install_dir, files, missing, unreadable, source))
    if unreadable:
        gh_error(f"{len(unreadable)} .exe/.dll files are not PE images: {unreadable[0]}")
    if missing:
        importers = len({item.importer for item in missing})
        first = ", ".join(f"{item.importer} -> {item.dll}" for item in missing[:3])
        gh_error(
            f"{len(missing)} imported DLLs of {importers} files are neither installed nor part of "
            f"Windows: {first}",
            title="DLL dependency audit",
        )
    if missing or unreadable:
        return 1
    print(f"All imports of {files} files resolve to the install or to Windows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
