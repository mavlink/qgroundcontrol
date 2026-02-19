"""Tests for generate_matrix.py."""

from __future__ import annotations

import pytest

from generate_matrix import generate_matrix


@pytest.fixture
def sample_config() -> dict:
    return {
        "platforms": {
            "linux": {
                "timeout_minutes": 90,
                "variants": [
                    {
                        "name": "x64",
                        "runner": "ubuntu-22.04",
                        "host": "linux",
                        "arch": "linux_gcc_64",
                        "package": "QGroundControl-x86_64",
                        "build_types": ["Debug", "Release"],
                    },
                    {
                        "name": "arm64",
                        "runner": "ubuntu-24.04-arm",
                        "host": "linux_arm64",
                        "arch": "linux_gcc_arm64",
                        "package": "QGroundControl-aarch64",
                        "build_types": ["Release"],
                    },
                ],
            },
            "android": {
                "timeout_minutes": 120,
                "variants": [
                    {
                        "name": "linux",
                        "runner": "ubuntu-latest",
                        "host": "linux-emulator",
                        "qt_host": "linux",
                        "arch": "linux_gcc_64",
                        "qt_host_path": "gcc_64",
                        "shell": "bash",
                        "primary": False,
                        "emulator": True,
                        "abis": "x86_64",
                        "build_types": ["Release"],
                    }
                ],
            },
        }
    }


def test_generate_matrix_linux_all_build_types(sample_config: dict) -> None:
    matrix = generate_matrix(sample_config, "linux")
    includes = matrix.get("include", [])

    assert len(includes) == 3
    assert matrix["timeout_minutes"] == 90
    assert any(i["name"] == "x64" and i["build_type"] == "Debug" for i in includes)
    assert any(i["name"] == "x64" and i["build_type"] == "Release" for i in includes)
    assert any(i["name"] == "arm64" and i["build_type"] == "Release" for i in includes)


def test_generate_matrix_with_build_type_override(sample_config: dict) -> None:
    matrix = generate_matrix(sample_config, "linux", build_type="Debug")
    includes = matrix.get("include", [])

    assert len(includes) == 1
    assert includes[0]["name"] == "x64"
    assert includes[0]["build_type"] == "Debug"


def test_generate_matrix_pr_prefers_debug(sample_config: dict) -> None:
    matrix = generate_matrix(sample_config, "linux", event_name="pull_request")
    includes = matrix.get("include", [])

    # x64 supports Debug+Release -> pick Debug for PR
    # arm64 supports only Release -> fallback to first available
    assert len(includes) == 2
    assert any(i["name"] == "x64" and i["build_type"] == "Debug" for i in includes)
    assert any(i["name"] == "arm64" and i["build_type"] == "Release" for i in includes)


def test_generate_matrix_variant_filter(sample_config: dict) -> None:
    matrix = generate_matrix(sample_config, "linux", variant_filter="arm64")
    includes = matrix.get("include", [])

    assert len(includes) == 1
    assert includes[0]["name"] == "arm64"
    assert includes[0]["build_type"] == "Release"


def test_generate_matrix_carries_optional_fields(sample_config: dict) -> None:
    matrix = generate_matrix(sample_config, "android")
    includes = matrix.get("include", [])

    assert len(includes) == 1
    entry = includes[0]
    assert entry["host"] == "linux-emulator"
    assert entry["qt_host"] == "linux"
    assert entry["qt_host_path"] == "gcc_64"
    assert entry["shell"] == "bash"
    assert entry["primary"] is False
    assert entry["emulator"] is True
    assert entry["abis"] == "x86_64"


def test_generate_matrix_invalid_platform_exits(sample_config: dict) -> None:
    with pytest.raises(SystemExit):
        generate_matrix(sample_config, "freebsd")
