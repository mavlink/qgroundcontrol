"""Tests for pe_dependency_audit.py."""

from __future__ import annotations

import struct
from typing import TYPE_CHECKING

import pytest
from ci.packaging import pe_dependency_audit
from ci.packaging.pe_dependency_audit import (
    KNOWN_SYSTEM_DLLS,
    Imports,
    PEFormatError,
    SystemDlls,
    is_redistributable,
    read_imports,
)

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows

SECTION_RVA = 0x1000
SECTION_OFFSET = 0x200
PE32_IMAGE_BASE = 0x400000
PE32_PLUS_IMAGE_BASE = 0x140000000


def make_pe(
    imports: tuple[str, ...] = (),
    delayed: tuple[str, ...] = (),
    *,
    pe32: bool = False,
    vc6_delay: bool = False,
) -> bytes:
    """Return a minimal PE image with one section holding its import and delay-load tables."""
    image_base = PE32_IMAGE_BASE if pe32 else PE32_PLUS_IMAGE_BASE
    imports_size = 20 * (len(imports) + 1)
    delayed_rva = SECTION_RVA + imports_size
    names_rva = delayed_rva + 32 * (len(delayed) + 1)
    names = b""
    name_rvas = []
    for name in (*imports, *delayed):
        name_rvas.append(names_rva + len(names))
        names += name.encode() + b"\0"
    section = b"".join(struct.pack("<5I", 0, 0, 0, rva, 0) for rva in name_rvas[: len(imports)])
    section += bytes(20)
    for rva in name_rvas[len(imports) :]:
        # Attributes, then the name's RVA, or for a pre-VC7 descriptor its virtual address.
        attributes, name = (0, rva + image_base) if vc6_delay else (1, rva)
        section += struct.pack("<8I", attributes, name, 0, 0, 0, 0, 0, 0)
    section += bytes(32) + names

    directories = [(0, 0)] * 16
    if imports:
        directories[1] = (SECTION_RVA, imports_size)
    if delayed:
        directories[13] = (delayed_rva, 32 * len(delayed))
    packed_directories = b"".join(struct.pack("<II", *entry) for entry in directories)
    if pe32:
        optional = struct.pack("<H26xI60xI", 0x10B, image_base, 16)
    else:
        optional = struct.pack("<H22xQ76xI", 0x20B, image_base, 16)
    optional += packed_directories
    coff = struct.pack("<HHIIIHH", 0x8664, 1, 0, 0, 0, len(optional), 0x22)
    header = struct.pack(
        "<8sIIII16x", b".idata", len(section), SECTION_RVA, len(section), SECTION_OFFSET
    )
    image = b"MZ" + bytes(0x3A) + struct.pack("<I", 0x40) + b"PE\0\0" + coff + optional + header
    return image.ljust(SECTION_OFFSET, b"\0") + section


def _write(path: Path, data: bytes) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return path


@pytest.mark.parametrize("pe32", [False, True], ids=["pe32+", "pe32"])
def test_reads_imports_and_delay_imports(pe32):
    image = make_pe(("KERNEL32.dll", "Qt6Core.dll"), ("dbghelp.dll",), pe32=pe32)

    assert read_imports(image) == Imports(("KERNEL32.dll", "Qt6Core.dll"), ("dbghelp.dll",))


def test_reads_vc6_delay_descriptors_holding_virtual_addresses():
    image = make_pe(delayed=("old.dll",), pe32=True, vc6_delay=True)

    assert read_imports(image).delayed == ("old.dll",)


def test_image_without_import_tables_imports_nothing():
    assert read_imports(make_pe()) == Imports((), ())


@pytest.mark.parametrize(
    ("data", "message"),
    [
        (b"#!/bin/sh\n" * 10, "no MZ header"),
        (b"MZ" + bytes(0x3A) + struct.pack("<I", 0x40) + b"NE\0\0", "no PE signature"),
        (make_pe(("a.dll",))[:0x60], "truncated"),
    ],
    ids=["script", "dos-only", "truncated"],
)
def test_rejects_files_that_are_not_pe_images(data, message):
    with pytest.raises(PEFormatError, match=message):
        read_imports(data)


def test_rejects_import_names_outside_every_section():
    image = bytearray(make_pe(("a.dll",)))
    struct.pack_into("<I", image, SECTION_OFFSET + 12, 0x900000)

    with pytest.raises(PEFormatError, match="RVA 0x900000 is in no section"):
        read_imports(bytes(image))


@pytest.mark.parametrize(
    ("name", "expected"),
    [
        ("VCRUNTIME140.dll", True),
        ("vcruntime140_1.dll", True),
        ("MSVCP140_ATOMIC_WAIT.dll", True),
        ("msvcp140d.dll", True),
        ("concrt140.dll", True),
        ("ucrtbased.dll", True),
        ("msvcrt.dll", False),
        ("msvcp_win.dll", False),
        ("ucrtbase.dll", False),
        ("kernel32.dll", False),
    ],
)
def test_redistributable_runtime_names(name, expected):
    assert is_redistributable(name) is expected


def test_system_dlls_from_the_built_in_list_and_api_sets():
    system = SystemDlls([], ["Extra.dll"])

    assert "KERNEL32.dll" in system
    assert "api-ms-win-crt-runtime-l1-1-0.dll" in system
    assert "ext-ms-win-ntuser-window-l1-1-0.dll" in system
    assert "extra.dll" in system
    assert "Qt6Core.dll" not in system
    assert all(not is_redistributable(name) for name in KNOWN_SYSTEM_DLLS)


def test_system_dlls_from_a_directory_never_include_the_msvc_runtime(tmp_path):
    for name in ("Kernel32.DLL", "vcruntime140.dll", "msvcp140.dll"):
        (tmp_path / name).write_bytes(b"")

    system = SystemDlls([tmp_path], [])

    assert "kernel32.dll" in system
    assert "VCRUNTIME140.dll" not in system
    assert "msvcp140.dll" not in system
    assert "user32.dll" not in system


@pytest.fixture
def install(tmp_path: Path) -> Path:
    """An install whose every import resolves: bin/, beside the importer, Windows, or API sets."""
    root = tmp_path / "install"
    _write(
        root / "bin" / "QGroundControl.exe",
        make_pe(
            (
                "Qt6Core.dll",
                "KERNEL32.dll",
                "api-ms-win-crt-runtime-l1-1-0.dll",
                "VCRUNTIME140.dll",
            ),
            ("version.dll",),
        ),
    )
    _write(root / "bin" / "Qt6Core.dll", make_pe(("kernel32.dll",)))
    _write(root / "bin" / "vcruntime140.dll", make_pe(("kernel32.dll",)))
    _write(
        root / "lib" / "gstreamer-1.0" / "gstfoo.dll",
        make_pe(("GSTREAMER-1.0-0.DLL", "gstbar.dll")),
    )
    _write(root / "lib" / "gstreamer-1.0" / "gstbar.dll", make_pe(("gstreamer-1.0-0.dll",)))
    _write(root / "bin" / "gstreamer-1.0-0.dll", make_pe(("kernel32.dll",)))
    _write(
        root / "libexec" / "gstreamer-1.0" / "gst-plugin-scanner.exe",
        make_pe(("gstreamer-1.0-0.dll",)),
    )
    (root / "share").mkdir()
    (root / "share" / "notice.txt").write_text("not a PE file")
    return root


@pytest.fixture
def system32(tmp_path: Path) -> Path:
    directory = tmp_path / "System32"
    directory.mkdir()
    for name in ("kernel32.dll", "version.dll", "vcruntime140.dll"):
        (directory / name).write_bytes(b"")
    return directory


def _main(install: Path, *args: str) -> int:
    return pe_dependency_audit.main(["--install-dir", str(install), *args])


def test_passes_when_every_import_resolves(install, system32, capsys, gh_summary):
    assert _main(install, "--system-dir", str(system32)) == 0

    assert "All imports of 7 files resolve" in capsys.readouterr().out
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### DLL dependency audit\n")
    assert "| Result | passed |" in summary
    assert "| Files checked | 7 |" in summary
    assert "| Missing imports | 0 |" in summary


def test_reports_each_missing_import_and_fails(install, system32, capsys, gh_summary):
    (install / "bin" / "vcruntime140.dll").unlink()
    _write(
        install / "lib" / "gio" / "modules" / "gioopenssl.dll",
        make_pe(("libssl-3-x64.dll",), ("absent.dll",)),
    )

    assert _main(install, "--system-dir", str(system32)) == 1

    out = capsys.readouterr().out
    assert "bin/QGroundControl.exe -> VCRUNTIME140.dll\n" in out
    assert "lib/gio/modules/gioopenssl.dll -> libssl-3-x64.dll\n" in out
    assert "lib/gio/modules/gioopenssl.dll -> absent.dll (delay-load)\n" in out
    assert (
        "::error title=DLL dependency audit::3 imported DLLs of 2 files are neither installed"
        in out
    )
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | failed |" in summary
    assert "| `lib/gio/modules/gioopenssl.dll` | `absent.dll` | delay-load |" in summary


def test_the_built_in_list_resolves_system_dlls_off_windows(
    install, monkeypatch, capsys, gh_summary
):
    monkeypatch.setattr(pe_dependency_audit, "default_system_dirs", list)

    assert _main(install) == 0
    assert "| System DLLs | built-in list |" in gh_summary.read_text(encoding="utf-8")

    _write(install / "bin" / "uses-hid.dll", make_pe(("hid.dll",)))
    assert _main(install) == 1
    assert "bin/uses-hid.dll -> hid.dll" in capsys.readouterr().out
    assert _main(install, "--assume-system", "HID.dll") == 0


def test_defaults_to_system32_on_windows(install, system32, monkeypatch, gh_summary):
    monkeypatch.setattr(pe_dependency_audit, "default_system_dirs", lambda: [system32])

    assert _main(install) == 0
    assert f"| System DLLs | `{system32}` |" in gh_summary.read_text(encoding="utf-8")


def test_files_that_are_not_pe_images_fail(install, system32, capsys):
    (install / "bin" / "broken.dll").write_text("not an image")

    assert _main(install, "--system-dir", str(system32)) == 1
    out = capsys.readouterr().out
    assert "Not a PE image: bin/broken.dll: no MZ header" in out
    assert "::error::1 .exe/.dll files are not PE images" in out


@pytest.mark.parametrize("missing", ["install", "system"])
def test_missing_directories_fail(tmp_path, install, capsys, missing):
    if missing == "install":
        code = _main(tmp_path / "absent")
        expected = "Install directory not found"
    else:
        code = _main(install, "--system-dir", str(tmp_path / "absent"))
        expected = "System DLL directory not found"

    assert code == 1
    assert expected in capsys.readouterr().out
