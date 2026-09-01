"""Validate and enumerate the artifacts published with a QGC release."""

from __future__ import annotations

import argparse
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error
from common.io import read_json, verify_sha256_sidecar, write_text_if_changed

REQUIRED_PACKAGES: tuple[tuple[str, str], ...] = (
    ("Linux x86_64 AppImage", "QGroundControl-x86_64/*.AppImage"),
    ("Linux aarch64 AppImage", "QGroundControl-aarch64/*.AppImage"),
    ("macOS disk image", "QGroundControl/*.dmg"),
    ("Windows AMD64 installer", "QGroundControl-installer-AMD64/*.exe"),
    ("Windows ARM64 installer", "QGroundControl-installer-ARM64/*.exe"),
    (
        "Windows AMD64/ARM64 installer",
        "QGroundControl-installer-AMD64-ARM64/*.exe",
    ),
    ("Android APK", "QGroundControl-linux/*.apk"),
    ("iOS IPA", "QGroundControl-ios/*.ipa"),
)

REQUIRED_PLATFORM_SBOMS: tuple[str, ...] = (
    "QGroundControl-x86_64.sbom.spdx.json",
    "QGroundControl-aarch64.sbom.spdx.json",
    "QGroundControl-macos.sbom.spdx.json",
    "QGroundControl-installer-AMD64-windows.sbom.spdx.json",
    "QGroundControl-installer-ARM64-windows.sbom.spdx.json",
    "QGroundControl-installer-AMD64-ARM64-windows.sbom.spdx.json",
    "QGroundControl-linux.sbom.spdx.json",
    "QGroundControl-ios.sbom.spdx.json",
)

REQUIRED_DEPENDENCY_SBOMS: tuple[str, ...] = (
    "QGroundControl-x86_64.dependencies.cdx.json",
    "QGroundControl-aarch64.dependencies.cdx.json",
    "QGroundControl-macos.dependencies.cdx.json",
    "QGroundControl-installer-AMD64-windows.dependencies.cdx.json",
    "QGroundControl-installer-ARM64-windows.dependencies.cdx.json",
    "QGroundControl-installer-AMD64-ARM64-windows.dependencies.cdx.json",
    "QGroundControl-linux.dependencies.cdx.json",
    "QGroundControl-ios.dependencies.cdx.json",
)


def _require_unique(root: Path, pattern: str, label: str) -> Path:
    matches = sorted(path for path in root.glob(pattern) if path.is_file())
    if len(matches) != 1:
        raise ValueError(f"Expected exactly one {label} matching {pattern!r}; found {len(matches)}")
    return matches[0]


def _validate_sbom(sbom: Path, *, require_components: bool = False) -> None:
    document = read_json(sbom)
    if not isinstance(document, dict):
        raise ValueError(f"SBOM must contain a JSON object: {sbom}")

    if sbom.name.endswith(".spdx.json"):
        version = document.get("spdxVersion")
        if not isinstance(version, str) or not version.startswith("SPDX-"):
            raise ValueError(f"Invalid SPDX SBOM: {sbom}")
    elif sbom.name.endswith(".cdx.json"):
        if document.get("bomFormat") != "CycloneDX":
            raise ValueError(f"Invalid CycloneDX SBOM: {sbom}")
        if require_components:
            components = document.get("components")
            if not isinstance(components, list) or not components:
                raise ValueError(f"CycloneDX dependency SBOM contains no components: {sbom}")
    else:
        raise ValueError(f"Unsupported SBOM filename: {sbom}")


def collect_release_assets(artifacts_dir: Path, source_sboms: list[Path]) -> list[Path]:
    """Return the complete, validated release asset list."""
    if not artifacts_dir.is_dir():
        raise FileNotFoundError(f"Artifact directory does not exist: {artifacts_dir}")

    assets: list[Path] = []
    for label, pattern in REQUIRED_PACKAGES:
        package = _require_unique(artifacts_dir, pattern, label)
        checksum = package.with_name(f"{package.name}.sha256")
        verify_sha256_sidecar(package, checksum)
        assets.extend((package, checksum))

        if package.suffix == ".AppImage":
            zsync = package.with_name(f"{package.name}.zsync")
            if not zsync.is_file():
                raise FileNotFoundError(f"AppImage update metadata is missing: {zsync}")
            assets.append(zsync)

    for sbom_name in REQUIRED_PLATFORM_SBOMS:
        sbom = _require_unique(artifacts_dir, f"**/{sbom_name}", f"SBOM {sbom_name}")
        _validate_sbom(sbom)
        assets.append(sbom)

    for sbom_name in REQUIRED_DEPENDENCY_SBOMS:
        sbom = _require_unique(artifacts_dir, f"**/{sbom_name}", f"SBOM {sbom_name}")
        _validate_sbom(sbom, require_components=True)
        assets.append(sbom)

    for source_sbom in source_sboms:
        if not source_sbom.is_file():
            raise FileNotFoundError(f"Source SBOM does not exist: {source_sbom}")
        _validate_sbom(source_sbom)
        assets.append(source_sbom)

    return sorted(set(assets), key=lambda path: path.as_posix())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts-dir", required=True, type=Path)
    parser.add_argument("--source-sbom", action="append", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        assets = collect_release_assets(args.artifacts_dir, args.source_sbom)
        write_text_if_changed(args.output, "".join(f"{path}\n" for path in assets))
    except (OSError, ValueError) as exc:
        gh_error(str(exc))
        return 1

    print(f"Validated {len(assets)} release assets; manifest: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
