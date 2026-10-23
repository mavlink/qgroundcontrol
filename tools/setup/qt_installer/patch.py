"""aqtinstall's Qt 6 post-install edits, reproduced byte for byte.

Like aqt, every edited file is read with universal newlines and written with the platform's, so a
Windows host gets CRLF exactly where aqt writes it. aqt's qmake binary patch is a no-op for Qt 6 and
is skipped. QGC's builds pass QT_HOST_PATH, so these edits serve qmake, qtpaths and pkg-config users.
"""

from __future__ import annotations

import os
import re
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterable
    from pathlib import Path

_BUILD_PREFIXES = {
    "linux": "/home/qt/work/install",
    "linux_arm64": "/home/qt/work/install",
    "mac": "/Users/qt/work/install",
}
_WINDOWS_BUILD_PREFIX = "c:/Users/qt/work/install"
_UNPATCHED = (
    "/home/qt/work/install/",
    "/Users/qt/work/install/",
    "\\home\\qt\\work\\install\\",
    "\\Users\\qt\\work\\install\\",
)
_HOST_LIBRARY_EXECUTABLES = re.compile(r"^HostLibraryExecutables=[^\n]*$", re.MULTILINE)
_NDK_HOST = re.compile(r"^DEFAULT_ANDROID_NDK_HOST =[^\n]*$", re.MULTILINE)


def _edit(
    path: Path, edits: Iterable[tuple[str | re.Pattern[str], str]], *, executable: bool = False
) -> bool:
    """Apply *edits* in order, always rewriting the file as aqt does; return whether text changed."""
    mode = path.stat().st_mode | (0o111 if executable else 0)
    original = path.read_text(encoding="utf-8")
    text = original
    for old, new in edits:
        text = old.sub(new, text) if isinstance(old, re.Pattern) else text.replace(old, new)
    path.write_text(text, encoding="utf-8")
    os.chmod(path, mode)
    return text != original


def _rel(path: Path, prefix: Path) -> str:
    return path.relative_to(prefix).as_posix()


def patch_desktop(prefix: Path, host: str) -> list[str]:
    """Write ``bin/qt.conf`` and point pkg-config, libtool and prl files at *prefix*.

    Unlike aqt, ``linux_arm64`` gets the same edits as ``linux``: aqt leaves its ``prefix=`` lines
    pointing at Qt's build machine.
    """
    lib, bin_dir = prefix / "lib", prefix / "bin"
    (bin_dir / "qt.conf").write_text("[Paths]\nPrefix=..\n", encoding="utf-8")
    patched = ["bin/qt.conf"]
    windows = host.startswith("windows")
    build = _WINDOWS_BUILD_PREFIX if windows else _BUILD_PREFIXES.get(host)
    if build is None:
        return patched
    mac = host == "mac"
    changed = []
    for pc in sorted(lib.glob("pkgconfig/*.pc")):
        edits = [(f"prefix={build}", f"prefix={prefix}")]
        if mac:
            edits.append((f"-F{build}/lib", f"-F{lib}"))
        changed += [pc] if _edit(pc, edits) else []
    if not windows:
        forms = [
            "libdir='={}'",
            "libdir='{}'",
            "-L={}",
            "-L{}",
            *(["-F={}", "-F{}"] if mac else []),
        ]
        for la in sorted(lib.glob("*.la")):
            edits = [(form.format(f"{build}/lib"), form.format(lib)) for form in forms]
            changed += [la] if _edit(la, edits) else []
    for prl in sorted(lib.glob("*.prl")):
        changed += [prl] if _edit(prl, [(f"{build}/lib", "$$[QT_INSTALL_LIBS]")]) else []
    if windows:
        (bin_dir / "qtenv2.bat").write_text(
            "@echo off\necho Setting up environment for Qt usage...\n"
            f"set PATH={bin_dir};%PATH%\ncd /D {prefix}\n"
            "echo Remember to call vcvarsall.bat to complete environment setup!\n",
            encoding="utf-8",
        )
        patched.append("bin/qtenv2.bat")
    return patched + [_rel(path, prefix) for path in changed]


def patch_cross(prefix: Path, outdir: Path, version: str, host: str, desktop_dir: str) -> list[str]:
    """Point a mobile or cross-compiled Qt's host-tool scripts and target_qt.conf at its desktop Qt.

    *host* is the effective host (``all_os`` resolved) and *desktop_dir* the arch directory of the
    desktop Qt under ``outdir/version``, which need not be installed yet.
    """
    windows = host.startswith("windows")
    host_bin = ("\\" if windows else "/").join([str(outdir), version, desktop_dir, "bin"])
    bin_dir = prefix / "bin"
    changed = []
    for name in ("qmake", "qtpaths", "qmake6", "qtpaths6"):
        script = bin_dir / (f"{name}.bat" if windows else name)
        if windows and not script.is_file():
            script = bin_dir / f"host-{name}.bat"
        if script.is_file():
            _edit(script, [(f"{old}bin", host_bin) for old in _UNPATCHED], executable=True)
            changed.append(script)
    conf = bin_dir / "target_qt.conf"
    if conf.is_file():
        library_executables = "./bin" if windows else "./libexec"
        edits: list[tuple[str | re.Pattern[str], str]] = [
            (_HOST_LIBRARY_EXECUTABLES, f"HostLibraryExecutables={library_executables}"),
            *((f"Prefix={old}target", f"Prefix={prefix / 'target'}") for old in _UNPATCHED),
            ("HostPrefix=../../", f"HostPrefix=../../{desktop_dir}"),
            ("HostData=target", f"HostData=../{prefix.name}"),
        ]
        _edit(conf, edits)
        changed.append(conf)
    qdevice = prefix / "mkspecs" / "qdevice.pri"
    if qdevice.is_file():
        ndk_host = "darwin" if host == "mac" else host
        _edit(qdevice, [(_NDK_HOST, f"DEFAULT_ANDROID_NDK_HOST = {ndk_host}-x86_64")])
        changed.append(qdevice)
    return [_rel(path, prefix) for path in changed]
