#!/usr/bin/env python3
"""Validate that a native Linux package contains only the QGC runtime layout."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path

PRIVATE_ROOT = "opt/QGroundControl"
REQUIRED_PATHS = {
    f"{PRIVATE_ROOT}/bin/QGroundControl",
    "usr/bin/QGroundControl",
    "usr/share/applications/org.mavlink.qgroundcontrol.desktop",
    "usr/share/icons/hicolor/256x256/apps/QGroundControl.png",
    "usr/share/icons/hicolor/scalable/apps/QGroundControl.svg",
    "usr/share/metainfo/org.mavlink.qgroundcontrol.appdata.xml",
}
EXPECTED_LAUNCHER_TARGET = "../../opt/QGroundControl/bin/QGroundControl"
EXPECTED_PACKAGE_NAME = "qgroundcontrol"
ARCH_PACKAGE_METADATA = {".BUILDINFO", ".INSTALL", ".MTREE", ".PKGINFO"}
RPM_PACKAGE_METADATA_ROOT = "usr/lib/.build-id"
SEMANTIC_VERSION_PREFIX = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:\D.*)?$")


def _allowed_system_paths() -> set[str]:
    allowed = set(REQUIRED_PATHS)
    for required in [*REQUIRED_PATHS, PRIVATE_ROOT]:
        current = Path(required)
        allowed.update(parent.as_posix() for parent in current.parents if parent.as_posix() != ".")
    return allowed


ALLOWED_SYSTEM_PATHS = _allowed_system_paths()


def normalize_paths(paths: list[str]) -> set[str]:
    """Normalize archive member names for generator-independent checks."""
    normalized_paths = set()
    for path in paths:
        normalized_path = path.removeprefix("./").strip("/")
        if normalized_path not in {"", "."}:
            normalized_paths.add(normalized_path)
    return normalized_paths


def validate_paths(paths: set[str]) -> list[str]:
    """Return actionable errors for an invalid native package file list."""
    errors = [f"missing required path: /{path}" for path in sorted(REQUIRED_PATHS - paths)]

    if not any(path.startswith(f"{PRIVATE_ROOT}/lib/libQt6Core.so") for path in paths):
        errors.append(f"missing bundled Qt runtime under /{PRIVATE_ROOT}/lib")

    for path in sorted(paths):
        if path.startswith(("usr/include/", f"{PRIVATE_ROOT}/include/")):
            errors.append(f"development header payload is forbidden: /{path}")
        elif path.endswith((".a", ".la")):
            errors.append(f"development library payload is forbidden: /{path}")
        elif "/pkgconfig/" in f"/{path}/" or "/cmake/" in f"/{path}/":
            errors.append(f"development metadata payload is forbidden: /{path}")
        elif path.startswith(("usr/lib/libQt6", "usr/lib/libgst", "usr/lib/libgstreamer")):
            errors.append(f"bundled runtime library must not use global /usr/lib: /{path}")
        elif not (
            path in ALLOWED_SYSTEM_PATHS
            or path == PRIVATE_ROOT
            or path.startswith(f"{PRIVATE_ROOT}/")
        ):
            errors.append(f"unexpected path outside the native runtime layout: /{path}")

    return errors


def validate_launcher_target(target: str | None) -> list[str]:
    """Validate that /usr/bin/QGroundControl is the expected relative symlink."""
    if target == EXPECTED_LAUNCHER_TARGET:
        return []
    if target is None:
        return ["/usr/bin/QGroundControl must be a symbolic link"]
    return [
        "invalid /usr/bin/QGroundControl symlink target: "
        f"{target!r} (expected {EXPECTED_LAUNCHER_TARGET!r})"
    ]


def validate_package_metadata(name: str, version: str) -> list[str]:
    """Validate stable package identity and a release-derived version."""
    errors = []
    if name != EXPECTED_PACKAGE_NAME:
        errors.append(f"invalid package name: {name!r} (expected {EXPECTED_PACKAGE_NAME!r})")

    match = SEMANTIC_VERSION_PREFIX.fullmatch(version)
    if match is None:
        errors.append(f"package version is not semantic: {version!r}")
    elif all(int(part) == 0 for part in match.groups()):
        errors.append(
            f"package version uses the 0.0.0 fallback: {version!r}; fetch Git tags before configuring"
        )
    return errors


def _run_text(command: list[str], *, stdin=None) -> str:
    return subprocess.run(
        command,
        check=True,
        stdin=stdin,
        capture_output=True,
        text=True,
    ).stdout


def _run_deb_tar(package: Path, tar_args: list[str]) -> str:
    archive = subprocess.Popen(
        ["dpkg-deb", "--fsys-tarfile", str(package)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if archive.stdout is None:
        raise RuntimeError("dpkg-deb did not expose package data")
    tar_error: Exception | None = None
    output = ""
    try:
        output = _run_text(["tar", *tar_args, "-"], stdin=archive.stdout)
    except Exception as error:
        tar_error = error
    finally:
        archive.stdout.close()
    error = archive.stderr.read() if archive.stderr is not None else b""
    if archive.wait() != 0:
        raise RuntimeError(error.decode(errors="replace").strip())
    if tar_error is not None:
        raise tar_error
    return output


def read_package_metadata(package: Path) -> tuple[str, str]:
    """Return the package-manager name and version for a native package."""
    name = package.name
    if name.endswith(".deb"):
        package_name = _run_text(["dpkg-deb", "--field", str(package), "Package"]).strip()
        version = _run_text(["dpkg-deb", "--field", str(package), "Version"]).strip()
        return package_name, version
    if name.endswith(".rpm"):
        output = _run_text(
            [
                "rpm",
                "-qp",
                "--queryformat",
                "%{NAME}\\n%{VERSION}-%{RELEASE}\\n",
                str(package),
            ]
        ).splitlines()
        if len(output) != 2:
            raise RuntimeError(f"unexpected RPM metadata output: {output!r}")
        return output[0], output[1]
    if name.endswith(".pkg.tar.zst"):
        metadata = _run_text(["tar", "-xOf", str(package), ".PKGINFO"])
        values = {}
        for line in metadata.splitlines():
            key, separator, value = line.partition(" = ")
            if separator and key in {"pkgname", "pkgver"}:
                values[key] = value
        if set(values) != {"pkgname", "pkgver"}:
            raise RuntimeError("Arch package metadata is missing pkgname or pkgver")
        return values["pkgname"], values["pkgver"]
    raise ValueError(f"unsupported native package: {package}")


def _launcher_target_from_tar_listing(listing: str) -> str | None:
    for line in listing.splitlines():
        if not line.startswith("l"):
            continue
        entry, separator, target = line.rpartition(" -> ")
        if not separator:
            continue
        path = entry.rsplit(maxsplit=1)[-1].removeprefix("./").strip("/")
        if path == "usr/bin/QGroundControl":
            return target.strip()
    return None


def read_launcher_target(package: Path) -> str | None:
    """Return the packaged /usr/bin/QGroundControl symlink target, if present."""
    name = package.name
    if name.endswith(".deb"):
        return _launcher_target_from_tar_listing(_run_deb_tar(package, ["-tvf"]))
    if name.endswith(".rpm"):
        listing = _run_text(
            [
                "rpm",
                "-qp",
                "--queryformat",
                "[%{FILENAMES}\\t%{FILELINKTOS}\\n]",
                str(package),
            ]
        )
        for line in listing.splitlines():
            path, _, target = line.partition("\t")
            if path.strip("/") == "usr/bin/QGroundControl":
                return target or None
        return None
    if name.endswith(".pkg.tar.zst"):
        return _launcher_target_from_tar_listing(_run_text(["tar", "-tvf", str(package)]))
    raise ValueError(f"unsupported native package: {package}")


def list_package_paths(package: Path) -> set[str]:
    """List package members using the native package tool."""
    name = package.name
    if name.endswith(".deb"):
        output = _run_deb_tar(package, ["-tf"])
    elif name.endswith(".rpm"):
        output = _run_text(["rpm", "-qlp", str(package)])
    elif name.endswith(".pkg.tar.zst"):
        output = _run_text(["tar", "-tf", str(package)])
    else:
        raise ValueError(f"unsupported native package: {package}")

    paths = normalize_paths(output.splitlines())
    if name.endswith(".pkg.tar.zst"):
        paths.difference_update(ARCH_PACKAGE_METADATA)
    elif name.endswith(".rpm"):
        paths = {
            path
            for path in paths
            if path != RPM_PACKAGE_METADATA_ROOT
            and not path.startswith(f"{RPM_PACKAGE_METADATA_ROOT}/")
        }
    return paths


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    args = parser.parse_args()

    package = args.package.resolve()
    if not package.is_file():
        parser.error(f"package not found: {package}")

    package_name, package_version = read_package_metadata(package)
    errors = validate_package_metadata(package_name, package_version)
    errors.extend(validate_paths(list_package_paths(package)))
    errors.extend(validate_launcher_target(read_launcher_target(package)))
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print(f"Native package validated: {package.name} ({package_name} {package_version})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
