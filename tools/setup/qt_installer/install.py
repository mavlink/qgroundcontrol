"""Install Qt: resolve the archives, download and extract them in parallel, patch, record."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import TYPE_CHECKING

from common.io import write_json
from qgc_tools.python_env import executable

from .download import TRUSTED_URL, download_archive, fetch_updates_xml, trusted_sha256
from .patch import patch_cross, patch_desktop
from .repo import (
    QtRequest,
    effective_host,
    host_desktop_arch,
    is_cross_arch,
    resolve_arch_dir,
    resolve_plan,
)

if TYPE_CHECKING:
    from .repo import Archive, Plan

MANIFEST = ".qgc-qt-manifest.json"
MODULES_MARKER = ".qgc-modules"
_JOBS = min(8, os.cpu_count() or 4)


def find_cmake() -> str:
    """Return the CMake that extracts archives: PATH's, else the tools environment's."""
    found = shutil.which("cmake")
    if found:
        return found
    bundled = executable("cmake")
    if bundled.is_file():
        return str(bundled)
    raise RuntimeError(
        "CMake extracts the Qt archives (cmake -E tar) but is not on PATH. Install it, sync the "
        "tools `build` group (python3 tools/setup/install_python.py build), or pass --installer aqt."
    )


def extract(archive: Path, dest: Path, cmake: str) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(
        [cmake, "-E", "tar", "xf", str(archive)],
        cwd=dest,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"cmake -E tar xf {archive.name} failed: {result.stderr or result.stdout}"
        )


def _install_archive(archive: Archive, outdir: Path, staging: Path, cmake: str) -> str:
    sha256 = trusted_sha256(archive.url_path)
    path = staging / archive.package / archive.filename
    path.parent.mkdir(parents=True, exist_ok=True)
    download_archive(archive.url_path, path, sha256)
    extract(path, outdir / archive.dest, cmake)
    path.unlink()
    print(f"Installed {archive.filename}")
    return sha256


def _install_archives(plan: Plan, outdir: Path, cmake: str, jobs: int) -> list[str]:
    with (
        tempfile.TemporaryDirectory(prefix=".qgc-qt-download-", dir=outdir) as staging,
        ThreadPoolExecutor(jobs) as pool,
    ):
        futures = [
            pool.submit(_install_archive, archive, outdir, Path(staging), cmake)
            for archive in plan.archives
        ]
        try:
            return [future.result() for future in futures]
        except BaseException:
            for future in futures:
                future.cancel()
            raise


def _patch(request: QtRequest, outdir: Path, prefix: Path) -> list[str]:
    if request.archives is not None and "qtbase" not in request.archives:
        return []  # aqt leaves an --archives subset without qtbase unpatched
    if is_cross_arch(request.target, request.arch):
        desktop_dir = resolve_arch_dir(host_desktop_arch(request.host, request.arch))
        host = effective_host(request.host)
        return patch_cross(prefix, outdir, request.version, host, desktop_dir)
    return patch_desktop(prefix, request.host)


def _write_records(
    request: QtRequest, prefix: Path, plan: Plan, digests: list[str], patched: list[str]
) -> None:
    manifest = {
        "installer": "qgc",
        "version": request.version,
        "host": request.host,
        "target": request.target,
        "arch": request.arch,
        "modules": list(request.modules),
        "archives_filter": None if request.archives is None else list(request.archives),
        "repository": f"{TRUSTED_URL}/{request.repo_dir}",
        "archives": [
            {"package": a.package, "file": a.filename, "dest": a.dest, "sha256": digest}
            for a, digest in zip(plan.archives, digests, strict=True)
        ],
        "patched": patched,
    }
    write_json(prefix / MANIFEST, manifest)
    # install_qt.py reuses a preinstalled SDK only when it has every requested module.
    if request.archives is None:
        (prefix / MODULES_MARKER).write_text(" ".join(request.modules) + "\n", encoding="utf-8")


def install(request: QtRequest, outdir: Path, *, jobs: int = _JOBS) -> Path:
    """Install *request* into ``outdir/<version>/<arch dir>`` and return that directory."""
    outdir = outdir.absolute()
    prefix = outdir / request.version / resolve_arch_dir(request.arch)
    cmake = find_cmake()
    plan = resolve_plan(request, fetch_updates_xml(request.repo_dir))
    for module, dependency in plan.unlisted_dependencies:
        print(f"warning: {module} depends on {dependency}, which is not installed", file=sys.stderr)
    print(f"Installing {len(plan.archives)} archives from {TRUSTED_URL}/{request.repo_dir}")
    started = time.monotonic()
    outdir.mkdir(parents=True, exist_ok=True)
    # aqt's py7zr creates 0755 directories whatever the umask, while libarchive and the patch step
    # honour it; a fixed umask gives the same modes as aqt under the usual 022 on every machine.
    umask = os.umask(0o022)
    try:
        digests = _install_archives(plan, outdir, cmake, jobs)
        patched = _patch(request, outdir, prefix)
        _write_records(request, prefix, plan, digests, patched)
    finally:
        os.umask(umask)
    print(
        f"Installed Qt {request.version} {request.arch} into {prefix} in {time.monotonic() - started:.0f} s"
    )
    return prefix


def ensure_host_desktop(request: QtRequest, outdir: Path) -> None:
    """Install the desktop Qt a mobile or cross Qt needs when it is missing (aqt --autodesktop)."""
    if not is_cross_arch(request.target, request.arch):
        return
    desktop = QtRequest(
        effective_host(request.host),
        "desktop",
        request.version,
        host_desktop_arch(request.host, request.arch),
        request.modules,
        request.archives,
    )
    bin_dir = outdir / request.version / resolve_arch_dir(desktop.arch) / "bin"
    if not any((bin_dir / name).is_file() for name in ("qmake", "qmake.exe")):
        install(desktop, outdir)
