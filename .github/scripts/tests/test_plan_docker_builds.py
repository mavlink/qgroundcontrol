from __future__ import annotations

import json
import subprocess
import sys

from _helpers import REPO_ROOT
from plan_docker_builds import build_args_str, load_variants, plan_builds

_DOCKER_DIR = REPO_ROOT / "deploy" / "docker"

# Build-arg strings pinned independently of variants.json so a bad edit to the
# JSON is caught here, not silently propagated into the CI matrix.
_LINUX_2204_BUILD_ARGS = "\n".join(
    [
        "BASE_REF=ubuntu:22.04@sha256:4f838adc7181d9039ac795a7d0aba05a9bd9ecd480d294483169c5def983b64d",
        "APT_EXTRA=gcc-12 g++-12",
        "PIP_CMAKE=cmake>=3.25,<4",
        "CC_PIN=gcc-12",
        "CXX_PIN=g++-12",
    ]
)
_LINUX_2604_BUILD_ARGS = (
    "BASE_REF=ubuntu:26.04@sha256:f3d28607ddd78734bb7f71f117f3c6706c666b8b76cbff7c9ff6e5718d46ff64"
)
_LINUX_DEBIAN_BUILD_ARGS = "BASE_REF=debian:bookworm-slim@sha256:96e378d7e6531ac9a15ad505478fcc2e69f371b10f5cdf87857c4b8188404716"
_LINUX_FEDORA_BUILD_ARGS = "\n".join(
    [
        "BASE_REF=fedora:41@sha256:f1a3fab47bcb3c3ddf3135d5ee7ba8b7b25f2e809a47440936212a3a50957f3d",
        "SETUP_BASE=setup-base-dnf.sh",
        "DEP_PLATFORM=fedora",
    ]
)
_LINUX_ARCH_BUILD_ARGS = "\n".join(
    [
        "BASE_REF=archlinux:base@sha256:ff410a88e200b133e577f5730b7bfa324e26a333075ee056bf45e911c6ac5671",
        "SETUP_BASE=setup-base-pacman.sh",
        "DEP_PLATFORM=arch",
    ]
)


def test_plan_builds_pull_request_filters_by_changes():
    plan = plan_builds("pull_request", linux_changed=True, android_changed=False)
    assert plan["has_jobs"] is True
    assert plan["matrix"]["include"] == [
        {
            "platform": "Linux-Ubuntu-24.04",
            "target": "linux",
            "variant": "linux",
            "build_args": "",
            "fuse": True,
            "artifact_pattern": "*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Ubuntu-22.04",
            "target": "linux",
            "variant": "linux-2204",
            "build_args": _LINUX_2204_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Ubuntu-26.04",
            "target": "linux",
            "variant": "linux-2604",
            "build_args": _LINUX_2604_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Debian",
            "target": "linux",
            "variant": "linux-debian",
            "build_args": _LINUX_DEBIAN_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Fedora",
            "target": "linux",
            "variant": "linux-fedora",
            "build_args": _LINUX_FEDORA_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "*.AppImage",
            "package_pattern": "*.rpm",
        },
        {
            "platform": "Linux-Arch",
            "target": "linux",
            "variant": "linux-arch",
            "build_args": _LINUX_ARCH_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "*.AppImage",
            "package_pattern": "*.pkg.tar.zst",
        },
        {
            "platform": "Linux-aarch64",
            "target": "linux-cross",
            "variant": "linux-aarch64",
            "build_args": "",
            "fuse": False,
            "artifact_pattern": "QGroundControl",
            "package_pattern": "",
        },
    ]


def test_native_package_patterns_per_distro():
    include = plan_builds("push", linux_changed=False, android_changed=False)["matrix"]["include"]
    pkg = {e["platform"]: e["package_pattern"] for e in include}
    assert pkg["Linux-Ubuntu-24.04"] == "*.deb"
    assert pkg["Linux-Ubuntu-22.04"] == "*.deb"
    assert pkg["Linux-Ubuntu-26.04"] == "*.deb"
    assert pkg["Linux-Debian"] == "*.deb"
    assert pkg["Linux-Fedora"] == "*.rpm"
    assert pkg["Linux-Arch"] == "*.pkg.tar.zst"
    assert pkg["Linux-aarch64"] == ""
    assert pkg["Android"] == ""


def test_2204_reuses_linux_target_with_distinct_cache_variant():
    include = plan_builds("push", linux_changed=False, android_changed=False)["matrix"]["include"]
    by_platform = {e["platform"]: e for e in include}
    u2404, u2204 = by_platform["Linux-Ubuntu-24.04"], by_platform["Linux-Ubuntu-22.04"]
    assert u2404["target"] == u2204["target"]
    assert u2404["variant"] != u2204["variant"]
    assert "ubuntu:22.04" in u2204["build_args"]
    assert u2404["build_args"] == ""


def test_non_ubuntu_linux_variants_reuse_linux_target():
    include = plan_builds("push", linux_changed=False, android_changed=False)["matrix"]["include"]
    by_platform = {e["platform"]: e for e in include}
    for name in ("Linux-Debian", "Linux-Fedora", "Linux-Arch"):
        assert by_platform[name]["target"] == "linux"
        assert by_platform[name]["variant"] != "linux"
    assert "debian:bookworm" in by_platform["Linux-Debian"]["build_args"]
    assert "DEP_PLATFORM=fedora" in by_platform["Linux-Fedora"]["build_args"]
    assert "DEP_PLATFORM=arch" in by_platform["Linux-Arch"]["build_args"]


def test_plan_builds_pull_request_android_only_excludes_linux():
    plan = plan_builds("pull_request", linux_changed=False, android_changed=True)
    assert [e["target"] for e in plan["matrix"]["include"]] == ["android"]


def test_plan_builds_pull_request_no_changes_yields_empty_matrix():
    plan = plan_builds("pull_request", linux_changed=False, android_changed=False)
    assert plan["has_jobs"] is False
    assert plan["matrix"]["include"] == []


def test_plan_builds_push_includes_all():
    plan = plan_builds("push", linux_changed=False, android_changed=False)
    assert len(plan["matrix"]["include"]) == 8
    assert [e["platform"] for e in plan["matrix"]["include"]] == [
        "Linux-Ubuntu-24.04",
        "Linux-Ubuntu-22.04",
        "Linux-Ubuntu-26.04",
        "Linux-Debian",
        "Linux-Fedora",
        "Linux-Arch",
        "Linux-aarch64",
        "Android",
    ]


def test_every_variant_has_required_fields():
    required = {
        "id",
        "ci_variant",
        "platform",
        "selector",
        "target",
        "image",
        "fuse",
        "artifact_pattern",
        "package_pattern",
        "build_args",
    }
    for v in load_variants():
        assert required <= set(v), f"{v.get('id')} missing {required - set(v)}"
        assert v["selector"] in ("linux", "android")


def test_variant_info_helper_matches_json():
    """_variant_info.py (consumed by run-docker.sh) emits the JSON's build args."""
    out = subprocess.run(
        [sys.executable, str(_DOCKER_DIR / "_variant_info.py"), "fedora"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    assert "target=linux" in out
    assert "default_image=qgc-fedora-docker" in out
    assert "fuse=1" in out
    assert "SETUP_BASE=setup-base-dnf.sh" in out


def test_variant_info_helper_rejects_unknown():
    result = subprocess.run(
        [sys.executable, str(_DOCKER_DIR / "_variant_info.py"), "nope"],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 1
    assert "Unknown variant" in result.stderr


def test_docker_compose_is_in_sync_with_variants():
    result = subprocess.run(
        [sys.executable, str(_DOCKER_DIR / "gen_compose.py"), "--check"],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr


def test_build_args_str_preserves_order():
    assert build_args_str({"A": "1", "B": "2"}) == "A=1\nB=2"


def test_run_docker_sh_has_no_hardcoded_build_args():
    """Variant build args live only in variants.json, not duplicated in the shell wrapper."""
    text = (_DOCKER_DIR / "run-docker.sh").read_text()
    assert "--build-arg" not in text
    assert json.loads((_DOCKER_DIR / "variants.json").read_text())["variants"]
