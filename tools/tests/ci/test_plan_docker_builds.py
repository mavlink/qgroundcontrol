from __future__ import annotations

import json
import re
import subprocess
import sys
from typing import TYPE_CHECKING

import pytest
from ci.planning import detect_changes
from ci.planning.plan_docker_builds import build_args_str, load_variants, main, plan_builds
from qgc_tools import release_tag_check

from .._helpers import REPO_ROOT, git

_DOCKER_DIR = REPO_ROOT / "deploy" / "docker"

if TYPE_CHECKING:
    from pathlib import Path


@pytest.mark.parametrize(
    "change",
    [
        {"fuse": "false"},
        {"build_args": {"BASE_REF": 1}},
        {"selector": "unknown"},
        {"target": None},
        {"ci_variant": ""},
    ],
)
def test_variant_loader_rejects_invalid_fields(tmp_path: Path, change: dict) -> None:
    variant = dict(load_variants()[0]) | change
    manifest = tmp_path / "variants.json"
    manifest.write_text(json.dumps({"variants": [variant]}))
    with pytest.raises(ValueError):
        load_variants(manifest)


@pytest.mark.parametrize("field", ["id", "ci_variant"])
def test_variant_loader_rejects_duplicate_identity(tmp_path: Path, field: str) -> None:
    variants = load_variants()[:2]
    variants[1][field] = variants[0][field]
    manifest = tmp_path / "variants.json"
    manifest.write_text(json.dumps({"variants": variants}))
    with pytest.raises(ValueError, match="duplicates"):
        load_variants(manifest)


# Build-arg strings pinned independently of variants.json so a bad edit to the
# JSON is caught here, not silently propagated into the CI matrix.
_LINUX_2204_BUILD_ARGS = "\n".join(
    [
        "BASE_REF=public.ecr.aws/docker/library/ubuntu:22.04@sha256:4f838adc7181d9039ac795a7d0aba05a9bd9ecd480d294483169c5def983b64d",
        "APT_EXTRA=gcc-12 g++-12",
        "CC_PIN=gcc-12",
        "CXX_PIN=g++-12",
    ]
)
_LINUX_2604_BUILD_ARGS = "BASE_REF=public.ecr.aws/docker/library/ubuntu:26.04@sha256:f3d28607ddd78734bb7f71f117f3c6706c666b8b76cbff7c9ff6e5718d46ff64"
_LINUX_DEBIAN_BUILD_ARGS = "BASE_REF=public.ecr.aws/docker/library/debian:bookworm-slim@sha256:96e378d7e6531ac9a15ad505478fcc2e69f371b10f5cdf87857c4b8188404716"
_LINUX_FEDORA_BUILD_ARGS = "\n".join(
    [
        "BASE_REF=public.ecr.aws/docker/library/fedora:41@sha256:f1a3fab47bcb3c3ddf3135d5ee7ba8b7b25f2e809a47440936212a3a50957f3d",
        "SETUP_BASE=setup-base-dnf.sh",
        "DEP_PLATFORM=fedora",
    ]
)
_LINUX_ARCH_BUILD_ARGS = "\n".join(
    [
        "BASE_REF=public.ecr.aws/docker/library/archlinux:base@sha256:ff410a88e200b133e577f5730b7bfa324e26a333075ee056bf45e911c6ac5671",
        "SETUP_BASE=setup-base-pacman.sh",
        "DEP_PLATFORM=arch",
    ]
)


def test_plan_builds_pull_request_filters_by_changes():
    plan = plan_builds("pull_request", linux_changed=True, android_changed=False, full_matrix=True)
    assert plan["has_jobs"] is True
    assert plan["matrix"]["include"] == [
        {
            "platform": "Linux-Ubuntu-24.04",
            "target": "linux",
            "variant": "linux",
            "build_args": "",
            "fuse": True,
            "artifact_pattern": "QGroundControl-*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Ubuntu-22.04",
            "target": "linux",
            "variant": "linux-2204",
            "build_args": _LINUX_2204_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "QGroundControl-*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Ubuntu-26.04",
            "target": "linux",
            "variant": "linux-2604",
            "build_args": _LINUX_2604_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "QGroundControl-*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Debian",
            "target": "linux",
            "variant": "linux-debian",
            "build_args": _LINUX_DEBIAN_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "QGroundControl-*.AppImage",
            "package_pattern": "*.deb",
        },
        {
            "platform": "Linux-Fedora",
            "target": "linux",
            "variant": "linux-fedora",
            "build_args": _LINUX_FEDORA_BUILD_ARGS,
            "fuse": False,
            "artifact_pattern": "QGroundControl-*.AppImage",
            "package_pattern": "*.rpm",
        },
        {
            "platform": "Linux-Arch",
            "target": "linux",
            "variant": "linux-arch",
            "build_args": _LINUX_ARCH_BUILD_ARGS,
            "fuse": False,
            "artifact_pattern": "QGroundControl-*.AppImage",
            "package_pattern": "*.pkg.tar.zst",
        },
        {
            "platform": "Linux-aarch64",
            "target": "linux-cross",
            "variant": "linux-aarch64",
            "build_args": "",
            "fuse": False,
            "artifact_pattern": "Release/QGroundControl",
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


def test_artifact_patterns_exclude_packaging_tools_and_staging_copies():
    include = plan_builds("push", linux_changed=False, android_changed=False)["matrix"]["include"]
    artifact = {entry["platform"]: entry["artifact_pattern"] for entry in include}

    native_linux = {
        "Linux-Ubuntu-24.04",
        "Linux-Ubuntu-22.04",
        "Linux-Ubuntu-26.04",
        "Linux-Debian",
        "Linux-Fedora",
        "Linux-Arch",
    }
    assert all(artifact[platform] == "QGroundControl-*.AppImage" for platform in native_linux)
    assert artifact["Linux-aarch64"] == "Release/QGroundControl"
    assert artifact["Android"] == "android-build/QGroundControl.apk"


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


def test_plan_builds_push_respects_should_build_false():
    plan = plan_builds("push", linux_changed=True, android_changed=True, should_build=False)
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


def test_docker_compose_is_in_sync_with_variants():
    result = subprocess.run(
        [sys.executable, str(_DOCKER_DIR / "gen_compose.py"), "--check"],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr


def test_base_images_come_from_ecr_public_pinned_by_digest():
    # PR builds pull anonymously, and Docker Hub rate-limits anonymous pulls.
    dockerfile = (_DOCKER_DIR / "Dockerfile").read_text()
    refs = re.findall(r"^ARG BASE_REF=(\S+)$", dockerfile, re.MULTILINE)
    refs += [v["build_args"]["BASE_REF"] for v in load_variants() if "BASE_REF" in v["build_args"]]
    assert len(refs) == 6
    for ref in refs:
        assert re.fullmatch(
            r"public\.ecr\.aws/docker/library/[a-z0-9.-]+:[\w.-]+@sha256:[0-9a-f]{64}", ref
        ), ref


def test_build_args_str_preserves_order():
    assert build_args_str({"A": "1", "B": "2"}) == "A=1\nB=2"


def test_application_pr_uses_representative_docker_variants():
    plan = plan_builds("pull_request", True, True)
    assert [row["platform"] for row in plan["matrix"]["include"]] == [
        "Linux-Ubuntu-24.04",
        "Android",
    ]


@pytest.mark.parametrize(
    "files",
    [
        None,
        ["deploy/docker/Dockerfile"],
        ["cmake/CPack.cmake"],
        [".github/build-config.json"],
        ["tools/setup/install_dependencies/_debian.py"],
        ["src/Vehicle/CMakeLists.txt"],
        [".github/actions/docker/action.yml"],
        ["tools/uv.lock"],
    ],
)
def test_toolchain_and_unknown_diffs_use_full_matrix(files):
    from ci.planning.plan_docker_builds import needs_full_matrix

    assert needs_full_matrix(files)


@pytest.mark.parametrize(
    "path",
    [
        "src/Vehicle/Vehicle.cc",
        ".github/ISSUE_TEMPLATE/bug.yml",
        ".github/workflows/analysis.yml",
        "tools/analyze.py",
        "tools/tests/test_install_qt.py",
        "deploy/multipass/build.sh",
    ],
)
def test_unrelated_changes_do_not_require_full_matrix(path):
    from ci.planning.plan_docker_builds import needs_full_matrix

    assert not needs_full_matrix([path])


def test_rename_out_of_docker_inputs_uses_full_matrix(
    monkeypatch: pytest.MonkeyPatch, git_repo: Path, gh_output: Path
) -> None:
    (git_repo / "deploy" / "docker").mkdir(parents=True)
    (git_repo / "deploy" / "docker" / "Dockerfile").write_text("FROM scratch\n" * 20)
    git(git_repo, "add", ".")
    git(git_repo, "commit", "-qm", "base")
    base = git(git_repo, "rev-parse", "HEAD")
    (git_repo / "docs").mkdir()
    git(git_repo, "mv", "deploy/docker/Dockerfile", "docs/Dockerfile")
    git(git_repo, "commit", "-qm", "move")

    monkeypatch.chdir(git_repo)
    monkeypatch.setenv("EVENT_NAME", "pull_request")
    monkeypatch.setenv("PR_BASE_SHA", base)
    monkeypatch.setenv("PR_HEAD_SHA", git(git_repo, "rev-parse", "HEAD"))
    assert main([]) == 0

    outputs = dict(line.split("=", 1) for line in gh_output.read_text().splitlines())
    assert len(json.loads(outputs["matrix"])["include"]) == len(load_variants())


_ALL_PLATFORMS = [
    "Linux-Ubuntu-24.04",
    "Linux-Ubuntu-22.04",
    "Linux-Ubuntu-26.04",
    "Linux-Debian",
    "Linux-Fedora",
    "Linux-Arch",
    "Linux-aarch64",
    "Android",
]


@pytest.fixture
def event(monkeypatch: pytest.MonkeyPatch, gh_output: Path) -> pytest.MonkeyPatch:
    monkeypatch.setenv("GITHUB_REF_TYPE", "branch")
    return monkeypatch


def _planned(gh_output: Path) -> tuple[list[str], str]:
    outputs = dict(line.split("=", 1) for line in gh_output.read_text().splitlines())
    platforms = [row["platform"] for row in json.loads(outputs["matrix"])["include"]]
    return platforms, outputs["has_jobs"]


@pytest.mark.parametrize(
    ("changed", "expected"),
    [
        (["docs/index.md"], []),
        (["src/Vehicle/Vehicle.cc"], ["Linux-Ubuntu-24.04", "Android"]),
        (["android/AndroidManifest.xml"], ["Android"]),
        (["deploy/linux/AppImage.sh"], _ALL_PLATFORMS[:-1]),
        (None, _ALL_PLATFORMS),
    ],
)
def test_main_plans_pull_requests_from_their_diff(
    event: pytest.MonkeyPatch, gh_output: Path, changed: list[str] | None, expected: list[str]
) -> None:
    event.setenv("EVENT_NAME", "pull_request")
    event.setattr(detect_changes, "get_changed_files", lambda: changed)
    assert main([]) == 0
    assert _planned(gh_output) == (expected, "true" if expected else "false")


@pytest.mark.parametrize("event_name", ["push", "workflow_dispatch"])
def test_main_builds_every_variant_outside_pull_requests(
    event: pytest.MonkeyPatch, gh_output: Path, event_name: str
) -> None:
    def unexpected() -> None:
        raise AssertionError("non-PR events must not diff")

    event.setenv("EVENT_NAME", event_name)
    event.setattr(detect_changes, "get_changed_files", unexpected)
    assert main([]) == 0
    assert _planned(gh_output) == (_ALL_PLATFORMS, "true")


@pytest.mark.parametrize(
    ("release_tag", "expected"), [(True, (_ALL_PLATFORMS, "true")), (False, ([], "false"))]
)
def test_main_builds_tags_only_for_stable_releases(
    event: pytest.MonkeyPatch,
    gh_output: Path,
    release_tag: bool,
    expected: tuple[list[str], str],
) -> None:
    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setattr(detect_changes, "check_release_tag", lambda *_: release_tag)
    assert main([]) == 0
    assert _planned(gh_output) == expected


def test_main_fails_when_tag_branches_are_unknown(
    event: pytest.MonkeyPatch, gh_output: Path
) -> None:
    def fail(*_: str) -> bool:
        raise release_tag_check.GitQueryError("no history")

    event.setenv("EVENT_NAME", "push")
    event.setenv("GITHUB_REF_TYPE", "tag")
    event.setattr(detect_changes, "check_release_tag", fail)
    assert main([]) == 1
    assert gh_output.read_text() == ""
