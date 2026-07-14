"""Tests for validate_native_package.py."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest
import validate_native_package as mod
from validate_native_package import (
    EXPECTED_LAUNCHER_TARGET,
    EXPECTED_PACKAGE_NAME,
    PRIVATE_ROOT,
    REQUIRED_PATHS,
    _launcher_target_from_tar_listing,
    normalize_paths,
    read_package_metadata,
    validate_launcher_target,
    validate_package_metadata,
    validate_paths,
)


def valid_runtime_paths() -> set[str]:
    return REQUIRED_PATHS | {f"{PRIVATE_ROOT}/lib/libQt6Core.so.6"}


def staged_native_runtime(root: Path) -> None:
    for relative_path in REQUIRED_PATHS - {"usr/bin/QGroundControl"}:
        staged_path = root / relative_path.replace("usr/share/", "opt/QGroundControl/share/")
        staged_path.parent.mkdir(parents=True, exist_ok=True)
        staged_path.touch()
    qt_core = root / PRIVATE_ROOT / "lib/libQt6Core.so.6"
    qt_core.parent.mkdir(parents=True, exist_ok=True)
    qt_core.touch()


def test_normalization_and_valid_private_runtime_layout() -> None:
    assert normalize_paths(["./usr/bin/QGroundControl", "/opt/QGroundControl/lib/", "./", "/"]) == {
        "usr/bin/QGroundControl",
        "opt/QGroundControl/lib",
    }
    assert validate_paths(valid_runtime_paths()) == []


def test_invalid_layout_reports_development_global_and_missing_runtime_errors() -> None:
    paths = valid_runtime_paths() | {
        "usr/include/example.h",
        f"{PRIVATE_ROOT}/lib/libexample.a",
        f"{PRIVATE_ROOT}/lib/pkgconfig/example.pc",
        f"{PRIVATE_ROOT}/lib/cmake/example/example-config.cmake",
    }
    errors = validate_paths(paths)
    assert len(errors) == 4
    assert all("development" in error for error in errors)
    errors = validate_paths(valid_runtime_paths() | {"usr/lib/libQt6Core.so.6"})
    assert errors == [
        "bundled runtime library must not use global /usr/lib: /usr/lib/libQt6Core.so.6"
    ]
    errors = validate_paths(set())
    assert len(errors) == len(REQUIRED_PATHS) + 1
    assert errors[-1] == f"missing bundled Qt runtime under /{PRIVATE_ROOT}/lib"


def test_rejects_unexpected_system_payloads_and_invalid_launcher() -> None:
    errors = validate_paths(
        valid_runtime_paths() | {"etc/qgc-unexpected.conf", "usr/local/bin/unexpected"}
    )
    assert errors == [
        "unexpected path outside the native runtime layout: /etc/qgc-unexpected.conf",
        "unexpected path outside the native runtime layout: /usr/local/bin/unexpected",
    ]

    assert validate_launcher_target(EXPECTED_LAUNCHER_TARGET) == []
    assert validate_launcher_target(None) == ["/usr/bin/QGroundControl must be a symbolic link"]
    assert "invalid" in validate_launcher_target("/wrong/target")[0]


def test_extracts_launcher_target_only_from_a_symlink_entry() -> None:
    listing = (
        "-rwxr-xr-x root/root 123 2026-01-01 ./opt/QGroundControl/bin/QGroundControl\n"
        "lrwxrwxrwx root/root 0 2026-01-01 ./usr/bin/QGroundControl -> "
        f"{EXPECTED_LAUNCHER_TARGET}\n"
    )
    assert _launcher_target_from_tar_listing(listing) == EXPECTED_LAUNCHER_TARGET
    assert (
        _launcher_target_from_tar_listing(
            "-rwxr-xr-x root/root 123 2026-01-01 ./usr/bin/QGroundControl\n"
        )
        is None
    )
    assert (
        _launcher_target_from_tar_listing(
            "lrwxrwxrwx root/root 0 2026-01-01 ./opt/usr/bin/QGroundControl -> "
            f"{EXPECTED_LAUNCHER_TARGET}\n"
        )
        is None
    )


def test_validates_package_name_and_version() -> None:
    assert validate_package_metadata(EXPECTED_PACKAGE_NAME, "5.0.3-1118.g31aa63725") == []
    assert validate_package_metadata(EXPECTED_PACKAGE_NAME, "5.0.3.1118.g31aa63725-1") == []

    assert "invalid package name" in validate_package_metadata("QGroundControl", "5.0.3")[0]
    assert "not semantic" in validate_package_metadata(EXPECTED_PACKAGE_NAME, "31aa63725-1")[0]
    assert (
        "0.0.0 fallback"
        in validate_package_metadata(EXPECTED_PACKAGE_NAME, "0.0.0-0.g31aa63725")[0]
    )


@pytest.mark.parametrize(
    ("package_name", "outputs", "expected"),
    [
        (
            "qgc.deb",
            ["qgroundcontrol\n", "5.0.3-1118.g31aa63725\n"],
            ("qgroundcontrol", "5.0.3-1118.g31aa63725"),
        ),
        (
            "qgc.rpm",
            ["qgroundcontrol\n5.0.3-1118.g31aa63725.fc43\n"],
            ("qgroundcontrol", "5.0.3-1118.g31aa63725.fc43"),
        ),
        (
            "qgc.pkg.tar.zst",
            ["pkgname = qgroundcontrol\npkgver = 5.0.3.1118.g31aa63725-1\n"],
            ("qgroundcontrol", "5.0.3.1118.g31aa63725-1"),
        ),
    ],
)
def test_reads_native_package_metadata(
    monkeypatch: pytest.MonkeyPatch,
    package_name: str,
    outputs: list[str],
    expected: tuple[str, str],
) -> None:
    output_iterator = iter(outputs)
    monkeypatch.setattr(mod, "_run_text", lambda command: next(output_iterator))
    assert read_package_metadata(Path(package_name)) == expected


@pytest.mark.parametrize(
    ("package_name", "listing", "expected"),
    [
        (
            "qgc.pkg.tar.zst",
            ".PKGINFO\n.MTREE\nopt/QGroundControl/bin/QGroundControl\n",
            {"opt/QGroundControl/bin/QGroundControl"},
        ),
        (
            "qgc.rpm",
            "/usr/lib/.build-id\n/usr/lib/.build-id/ab/hash\n"
            "/opt/QGroundControl/bin/QGroundControl\n",
            {"opt/QGroundControl/bin/QGroundControl"},
        ),
    ],
)
def test_list_package_paths_ignores_package_manager_metadata(
    monkeypatch: pytest.MonkeyPatch,
    package_name: str,
    listing: str,
    expected: set[str],
) -> None:
    monkeypatch.setattr(mod, "_run_text", lambda command: listing)
    assert mod.list_package_paths(Path(package_name)) == expected


def test_native_package_finalizer_moves_metadata_and_creates_launcher(tmp_path: Path) -> None:
    staged_native_runtime(tmp_path)
    script = Path(__file__).resolve().parents[3] / "cmake/install/FinalizeNativePackage.cmake"

    subprocess.run(
        ["cmake", f"-DQGC_NATIVE_PACKAGE_ROOT={tmp_path}", "-P", str(script)],
        check=True,
        capture_output=True,
        text=True,
    )

    assert (tmp_path / "usr/share/applications/org.mavlink.qgroundcontrol.desktop").is_file()
    launcher = tmp_path / "usr/bin/QGroundControl"
    assert launcher.is_symlink()
    assert launcher.readlink().as_posix() == EXPECTED_LAUNCHER_TARGET


def test_native_package_finalizer_rejects_missing_runtime_payload(tmp_path: Path) -> None:
    staged_native_runtime(tmp_path)
    (tmp_path / PRIVATE_ROOT / "lib/libQt6Core.so.6").unlink()
    script = Path(__file__).resolve().parents[3] / "cmake/install/FinalizeNativePackage.cmake"

    result = subprocess.run(
        ["cmake", f"-DQGC_NATIVE_PACKAGE_ROOT={tmp_path}", "-P", str(script)],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode != 0
    assert "bundled Qt Core is missing" in result.stderr
