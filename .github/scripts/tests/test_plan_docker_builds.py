"""Contract tests for Docker build planning and its variant manifest."""

from __future__ import annotations

import json
import subprocess
import sys
from importlib import import_module
from typing import TYPE_CHECKING

import pytest
from _helpers import REPO_ROOT
from plan_docker_builds import plan_builds

if TYPE_CHECKING:
    from pathlib import Path

DOCKER_DIR = REPO_ROOT / "deploy" / "docker"
load_variants = import_module("_variants").load_variants


def test_plan_selects_variants_for_event_and_changed_platforms() -> None:
    linux = plan_builds("pull_request", linux_changed=True, android_changed=False)
    assert linux["has_jobs"] is True
    assert all(entry["platform"].startswith("Linux") for entry in linux["matrix"]["include"])

    android = plan_builds("pull_request", linux_changed=False, android_changed=True)
    assert [entry["target"] for entry in android["matrix"]["include"]] == ["android"]

    empty = plan_builds("pull_request", linux_changed=False, android_changed=False)
    assert empty == {"has_jobs": False, "matrix": {"include": []}}

    push = plan_builds("push", linux_changed=False, android_changed=False)
    assert [entry["platform"] for entry in push["matrix"]["include"]] == [
        "Linux-Ubuntu-24.04",
        "Linux-Ubuntu-22.04",
        "Linux-Ubuntu-26.04",
        "Linux-Debian",
        "Linux-Fedora",
        "Linux-Arch",
        "Linux-aarch64",
        "Android",
    ]
    assert [entry["security_category"] for entry in push["matrix"]["include"][:2]] == [
        "grype-Linux-Ubuntu-24.04",
        "grype-Linux-Ubuntu-22.04",
    ]


def test_docker_workflow_uses_stable_security_category() -> None:
    workflow = (REPO_ROOT / ".github" / "workflows" / "docker.yml").read_text()
    assert "category: ${{ matrix.security_category }}" in workflow
    assert "category: 'grype-${{ matrix.platform }}'" not in workflow


def test_variant_manifest_has_complete_distinct_build_contracts() -> None:
    required = {
        "id",
        "ci_variant",
        "platform",
        "security_category",
        "selector",
        "target",
        "image",
        "fuse",
        "artifact_pattern",
        "package_pattern",
        "build_args",
    }
    variants = load_variants()
    assert len({variant["id"] for variant in variants}) == len(variants)
    assert len({variant["security_category"] for variant in variants}) == len(variants)
    for variant in variants:
        assert required <= set(variant), variant["id"]
        assert variant["selector"] in {"linux", "android"}

    by_platform = {variant["platform"]: variant for variant in variants}
    # These categories identify existing code-scanning configurations on master.
    # Renaming them prevents GitHub from comparing pull-request results to the baseline.
    assert by_platform["Linux-Ubuntu-24.04"]["security_category"] == "grype-Linux-Ubuntu-24.04"
    assert by_platform["Linux-Ubuntu-22.04"]["security_category"] == "grype-Linux-Ubuntu-22.04"
    assert by_platform["Linux-Ubuntu-22.04"]["target"] == "linux"
    assert "ubuntu:22.04" in by_platform["Linux-Ubuntu-22.04"]["build_args"]["BASE_REF"]
    assert "debian:bookworm" in by_platform["Linux-Debian"]["build_args"]["BASE_REF"]
    assert by_platform["Linux-Fedora"]["build_args"]["DEP_PLATFORM"] == "fedora"
    assert by_platform["Linux-Arch"]["build_args"]["DEP_PLATFORM"] == "arch"
    assert by_platform["Linux-Fedora"]["package_pattern"] == "*.rpm"
    assert by_platform["Linux-Arch"]["package_pattern"] == "*.pkg.tar.zst"
    assert by_platform["Android"]["artifact_pattern"] == "android-build/QGroundControl.apk"


def test_variant_loader_rejects_invalid_manifest(tmp_path: Path) -> None:
    variant = load_variants()[0]
    manifest = tmp_path / "variants.json"
    for variants in (
        [variant, variant],
        [{**variant, "id": "other"}, variant],
        [{**variant, "build_args": {"BASE_REF": 42}}],
    ):
        manifest.write_text(json.dumps({"variants": variants}))
        with pytest.raises(ValueError):
            load_variants(manifest)


def test_variant_info_helper_exposes_manifest_and_rejects_unknown() -> None:
    result = subprocess.run(
        [sys.executable, str(DOCKER_DIR / "_variant_info.py"), "fedora"],
        capture_output=True,
        text=True,
        check=True,
    )
    assert "target=linux" in result.stdout
    assert "default_image=qgc-fedora-docker" in result.stdout
    assert "SETUP_BASE=setup-base-dnf.sh" in result.stdout

    invalid = subprocess.run(
        [sys.executable, str(DOCKER_DIR / "_variant_info.py"), "unknown"],
        capture_output=True,
        text=True,
    )
    assert invalid.returncode == 1
    assert "Unknown variant" in invalid.stderr


def test_generated_compose_and_shell_wrapper_use_manifest() -> None:
    result = subprocess.run(
        [sys.executable, str(DOCKER_DIR / "gen_compose.py"), "--check"],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    assert "--build-arg" not in (DOCKER_DIR / "run-docker.sh").read_text()
