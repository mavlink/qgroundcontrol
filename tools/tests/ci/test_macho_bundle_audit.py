"""Tests for macho_bundle_audit.py."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

import pytest
from ci.packaging import macho_bundle_audit
from ci.packaging.macho_bundle_audit import (
    LoadCommands,
    audit_bundle,
    audit_load_commands,
    check_signature,
    macho_files,
    parse_load_commands,
)

if TYPE_CHECKING:
    from pathlib import Path

MH_MAGIC_64 = bytes.fromhex("cffaedfe")
GST = "Contents/Frameworks/GStreamer.framework/Versions/1.0"
FAT_MAGIC = bytes.fromhex("cafebabe")


def otool_output(
    path: str,
    *,
    filetype: int = 6,
    dylibs: tuple[str, ...] = (),
    rpaths: tuple[str, ...] = (),
    install_name: str = "",
    archs: tuple[str, ...] = ("x86_64", "arm64"),
) -> str:
    """Return ``otool -l -arch all`` output for a universal file with these load commands."""
    commands = [("LC_ID_DYLIB", "name", install_name)] if install_name else []
    commands += [("LC_LOAD_DYLIB", "name", name) for name in dylibs]
    commands += [("LC_RPATH", "path", rpath) for rpath in rpaths]
    text = []
    for arch in archs:
        text += [
            f"{path} (architecture {arch}):",
            "Mach header",
            "      magic  cputype cpusubtype  caps    filetype ncmds sizeofcmds      flags",
            f" 0xfeedfacf 16777228          0  0x00           {filetype}    {len(commands)}       1000 0x00200085",
        ]
        for index, (command, field, value) in enumerate(commands):
            text += [
                f"Load command {index}",
                f"          cmd {command}",
                "      cmdsize 56",
                f"         {field} {value} (offset {24 if field == 'name' else 12})",
            ]
            if field == "name":
                text += ["   time stamp 2 Wed Dec 31 19:00:02 1969", "      current version 1.0.0"]
    return "\n".join(text) + "\n"


def test_parses_libraries_and_rpaths_once_across_architectures():
    text = otool_output(
        "QtCore",
        dylibs=("@rpath/libz.dylib", "/usr/lib/libc++.1.dylib"),
        rpaths=("@loader_path/../Frameworks",),
        install_name="@rpath/QtCore.framework/Versions/A/QtCore",
    )
    text = text.replace(
        "LC_LOAD_DYLIB\n      cmdsize 56\n         name /usr/lib",
        "LC_LOAD_WEAK_DYLIB\n      cmdsize 56\n         name /usr/lib",
        1,
    )

    assert parse_load_commands(text) == LoadCommands(
        executable=False,
        dylibs=("@rpath/libz.dylib", "/usr/lib/libc++.1.dylib"),
        rpaths=("@loader_path/../Frameworks",),
    )
    assert parse_load_commands(otool_output("QGroundControl", filetype=2)).executable


def test_finds_mach_o_files_but_not_symlinks_or_resources(tmp_path):
    framework = tmp_path / "Contents" / "Frameworks" / "QtCore.framework"
    _write(framework / "Versions" / "A" / "QtCore", MH_MAGIC_64)
    (framework / "Versions" / "Current").symlink_to("A")
    (framework / "QtCore").symlink_to("Versions/Current/QtCore")
    _write(tmp_path / "Contents" / "MacOS" / "QGroundControl", FAT_MAGIC)
    _write(tmp_path / "Contents" / "Resources" / "qgroundcontrol.icns", b"icns")
    _write(tmp_path / "Contents" / "Resources" / "empty", b"")

    assert macho_files(tmp_path) == [
        framework / "Versions" / "A" / "QtCore",
        tmp_path / "Contents" / "MacOS" / "QGroundControl",
    ]


def _write(path: Path, data: bytes) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return path


class FakeTools:
    """Answer otool from per-file load commands and codesign from a canned signature."""

    def __init__(self, bundle: Path) -> None:
        self.bundle = bundle
        self.outputs: dict[str, str] = {}
        self.codesign: dict[str, tuple[int, str]] = {"--display": (0, "Signature=adhoc\n")}
        self.calls: list[list[str]] = []

    def add(
        self,
        relative: str,
        *,
        filetype: int = 6,
        dylibs: tuple[str, ...] = (),
        rpaths: tuple[str, ...] = (),
        install_name: str = "",
    ) -> None:
        path = _write(self.bundle / relative, MH_MAGIC_64)
        self.outputs[str(path)] = otool_output(
            str(path), filetype=filetype, dylibs=dylibs, rpaths=rpaths, install_name=install_name
        )

    def __call__(self, cmd: list[str]) -> subprocess.CompletedProcess[str]:
        self.calls.append(cmd)
        if cmd[0] == "otool":
            assert cmd[1:4] == ["-l", "-arch", "all"]
            if cmd[4] not in self.outputs:
                return subprocess.CompletedProcess(cmd, 1, "", "is not an object file")
            return subprocess.CompletedProcess(cmd, 0, self.outputs[cmd[4]], "")
        if cmd[0] == "codesign":
            code, text = self.codesign[cmd[1]]
            return subprocess.CompletedProcess(cmd, code, "", text)
        raise AssertionError(f"unexpected command: {cmd}")


@pytest.fixture
def tools(tmp_path, monkeypatch) -> FakeTools:
    """A bundle whose every library resolves inside it or to macOS."""
    fake = FakeTools(tmp_path / "QGroundControl.app")
    monkeypatch.setattr(macho_bundle_audit, "_run", fake)
    monkeypatch.setattr(macho_bundle_audit.shutil, "which", lambda name: f"/usr/bin/{name}")
    fake.add(
        "Contents/MacOS/QGroundControl",
        filetype=2,
        dylibs=(
            "/usr/lib/libSystem.B.dylib",
            "/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa",
            "@rpath/QtCore.framework/Versions/A/QtCore",
            "@rpath/libgstreamer-1.0.0.dylib",
            "@executable_path/../Frameworks/libavcodec.61.dylib",
        ),
        rpaths=(
            "@executable_path/../Frameworks",
            f"@executable_path/../{GST.removeprefix('Contents/')}/lib",
        ),
    )
    fake.add(
        "Contents/Frameworks/QtCore.framework/Versions/A/QtCore",
        dylibs=("/usr/lib/libc++.1.dylib",),
        install_name="@rpath/QtCore.framework/Versions/A/QtCore",
    )
    fake.add("Contents/Frameworks/libavcodec.61.dylib", install_name="@rpath/libavcodec.61.dylib")
    fake.add(f"{GST}/lib/libgstreamer-1.0.0.dylib", install_name="@rpath/libgstreamer-1.0.0.dylib")
    # Plugins resolve @rpath through their own LC_RPATH; the main executable's has no lib/gstreamer-1.0.
    fake.add(
        f"{GST}/lib/gstreamer-1.0/libgstapp.dylib",
        dylibs=("@rpath/libgstreamer-1.0.0.dylib", "@loader_path/../libgstreamer-1.0.0.dylib"),
        rpaths=("@loader_path/..", "@executable_path/../lib"),
    )
    # An absolute LC_RPATH matters only where dyld searches it: this file loads nothing through @rpath.
    fake.add(
        f"{GST}/lib/python3.9/_cairo.so",
        dylibs=("/usr/lib/libSystem.B.dylib",),
        rpaths=("/Applications/Xcode.app",),
    )
    fake.add(
        f"{GST}/libexec/gstreamer-1.0/gst-plugin-scanner",
        filetype=2,
        dylibs=(
            "@rpath/libgstreamer-1.0.0.dylib",
            "@executable_path/../../lib/libgstreamer-1.0.0.dylib",
        ),
        rpaths=("@loader_path/../../lib",),
    )
    _write(fake.bundle / "Contents" / "Info.plist", b"<plist/>")
    return fake


def test_a_bundle_whose_libraries_all_resolve_passes(tools, capsys, gh_summary):
    assert audit_bundle(tools.bundle)

    out = capsys.readouterr().out
    assert "Every library 7 Mach-O files load resolves inside QGroundControl.app or to macOS" in out
    assert "::notice::QGroundControl.app signature ad-hoc, not verified" in out
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Bundle audit\n\n| Item | Value |")
    assert "| Result | passed |" in summary
    assert "| Mach-O files | 7 |" in summary
    assert "| Unresolved libraries | 0 |" in summary


def test_reports_each_library_that_does_not_load_from_the_bundle_or_macos(
    tools, capsys, gh_summary
):
    main = str(tools.bundle / "Contents/MacOS/QGroundControl")
    tools.outputs[main] = tools.outputs[main].replace(
        "path @executable_path/../Frameworks (offset",
        "path /Library/Frameworks/GStreamer.framework/Versions/1.0/lib (offset",
    )
    tools.add(
        "Contents/PlugIns/sqldrivers/libqsqlmimer.dylib",
        dylibs=("/usr/local/lib/libmimerapi.dylib", "/opt/homebrew/lib/libglib-2.0.0.dylib"),
    )
    tools.add(
        "Contents/PlugIns/platforms/libqcocoa.dylib",
        dylibs=(
            "@rpath/QtMissing.framework/QtMissing",
            "@loader_path/libgone.dylib",
            "libbare.dylib",
        ),
    )
    _write(tools.bundle / "Contents/PlugIns/broken.dylib", MH_MAGIC_64)

    assert not audit_bundle(tools.bundle)

    files, problems = audit_load_commands(tools.bundle)
    assert files == 10
    assert sorted(problems) == [
        "Contents/MacOS/QGroundControl -> @rpath/QtCore.framework/Versions/A/QtCore: not found inside the bundle through any LC_RPATH",
        "Contents/MacOS/QGroundControl: LC_RPATH /Library/Frameworks/GStreamer.framework/Versions/1.0/lib: absolute path outside the bundle",
        f"Contents/PlugIns/broken.dylib: otool -l failed for {tools.bundle / 'Contents/PlugIns/broken.dylib'}: is not an object file",
        "Contents/PlugIns/platforms/libqcocoa.dylib -> @loader_path/libgone.dylib: not found inside the bundle",
        "Contents/PlugIns/platforms/libqcocoa.dylib -> @rpath/QtMissing.framework/QtMissing: not found inside the bundle through any LC_RPATH",
        "Contents/PlugIns/platforms/libqcocoa.dylib -> libbare.dylib: relative path, resolved against the working directory",
        "Contents/PlugIns/sqldrivers/libqsqlmimer.dylib -> /opt/homebrew/lib/libglib-2.0.0.dylib: absolute path outside the bundle and macOS",
        "Contents/PlugIns/sqldrivers/libqsqlmimer.dylib -> /usr/local/lib/libmimerapi.dylib: absolute path outside the bundle and macOS",
    ]
    out = capsys.readouterr().out
    assert "::error title=Bundle audit::8 libraries in QGroundControl.app don't load" in out
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | failed |" in summary
    assert "| Unresolved libraries | 8 |" in summary
    assert "libmimerapi.dylib" in summary


def test_lookups_that_leave_the_bundle_do_not_resolve(tools):
    _write(tools.bundle.parent / "outside" / "liboutside.dylib", MH_MAGIC_64)
    up = "/".join([".."] * 8)
    tools.add(
        f"{GST}/lib/gstreamer-1.0/libgstapp.dylib",
        dylibs=("@rpath/liboutside.dylib", f"@loader_path/{up}/outside/liboutside.dylib"),
        rpaths=(f"@loader_path/{up}/outside",),
    )

    _, problems = audit_load_commands(tools.bundle)

    assert sorted(problem.split(" -> ")[1] for problem in problems) == [
        f"@loader_path/{up}/outside/liboutside.dylib: not found inside the bundle",
        "@rpath/liboutside.dylib: not found inside the bundle through any LC_RPATH",
    ]


@pytest.mark.parametrize(
    ("display", "verify", "expected"),
    [
        ((0, "Signature=adhoc\n"), None, ("ad-hoc, not verified", "")),
        ((1, "code object is not signed at all\n"), None, ("unsigned, not verified", "")),
        ((1, "No such file or directory\n"), None, ("failed", "No such file or directory")),
        (
            (0, "Authority=Developer ID Application: QGC\n"),
            (0, "valid on disk\n"),
            ("verified", ""),
        ),
        (
            (0, "Authority=Developer ID Application: QGC\n"),
            (3, "a sealed resource is missing or invalid\n"),
            ("failed", "a sealed resource is missing or invalid"),
        ),
    ],
    ids=["adhoc", "unsigned", "unreadable", "verified", "invalid"],
)
def test_verifies_only_an_identity_signature_strictly(tools, display, verify, expected):
    tools.codesign = {"--display": display, "--verify": verify}

    assert check_signature(tools.bundle) == expected
    verifies = [cmd for cmd in tools.calls if cmd[:2] == ["codesign", "--verify"]]
    assert verifies == (
        [["codesign", "--verify", "--deep", "--strict", "--verbose=2", str(tools.bundle)]]
        if verify
        else []
    )


def test_an_invalid_signature_fails_the_audit(tools, capsys, gh_summary):
    tools.codesign = {
        "--display": (0, "Authority=Developer ID Application: QGC\n"),
        "--verify": (1, "a sealed resource is missing or invalid\n"),
    }

    assert not audit_bundle(tools.bundle)

    assert (
        "::error title=Bundle audit::codesign rejected QGroundControl.app"
        in capsys.readouterr().out
    )
    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Signature | failed |" in summary
    assert "a sealed resource is missing or invalid" in summary


def test_without_codesign_only_the_libraries_are_audited(tools, monkeypatch):
    monkeypatch.setattr(macho_bundle_audit.shutil, "which", lambda name: None)

    assert check_signature(tools.bundle) == ("not checked (no codesign)", "")
    assert audit_bundle(tools.bundle)
    assert not [cmd for cmd in tools.calls if cmd[0] == "codesign"]


def test_main_audits_a_bundle_with_the_given_otool(tools):
    assert macho_bundle_audit.main([str(tools.bundle)]) == 0
    assert macho_bundle_audit.main([str(tools.bundle.parent)]) == 1
