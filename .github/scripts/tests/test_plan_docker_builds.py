from __future__ import annotations

import re
from pathlib import Path

from plan_docker_builds import LINUX_2204_BUILD_ARGS, plan_builds

_RUN_DOCKER_SH = Path(__file__).resolve().parents[3] / "deploy" / "docker" / "run-docker.sh"


def test_plan_builds_pull_request_filters_by_changes():
    plan = plan_builds("pull_request", linux_changed=True, android_changed=False)
    assert plan["has_jobs"] is True
    assert plan["matrix"]["include"] == [
        {
            "platform": "Linux",
            "target": "linux",
            "variant": "linux",
            "build_args": "",
            "fuse": True,
            "artifact_pattern": "*.AppImage",
        },
        {
            "platform": "Linux-22.04",
            "target": "linux",
            "variant": "linux-2204",
            "build_args": LINUX_2204_BUILD_ARGS,
            "fuse": True,
            "artifact_pattern": "*.AppImage",
        },
        {
            "platform": "Linux-aarch64",
            "target": "linux-cross",
            "variant": "linux-aarch64",
            "build_args": "",
            "fuse": False,
            "artifact_pattern": "QGroundControl",
        },
    ]


def test_2204_reuses_linux_target_with_distinct_cache_variant():
    include = plan_builds("push", linux_changed=False, android_changed=False)["matrix"]["include"]
    by_platform = {e["platform"]: e for e in include}
    assert by_platform["Linux"]["target"] == by_platform["Linux-22.04"]["target"]
    assert by_platform["Linux"]["variant"] != by_platform["Linux-22.04"]["variant"]
    assert "ubuntu:22.04" in by_platform["Linux-22.04"]["build_args"]
    assert by_platform["Linux"]["build_args"] == ""


def test_plan_builds_pull_request_android_only_excludes_linux():
    plan = plan_builds("pull_request", linux_changed=False, android_changed=True)
    assert [e["target"] for e in plan["matrix"]["include"]] == ["android"]


def test_2204_build_args_match_run_docker_sh():
    sh_args = re.findall(r'--build-arg "([^"]*)"', _RUN_DOCKER_SH.read_text())
    assert sh_args == LINUX_2204_BUILD_ARGS.split("\n")


def test_plan_builds_push_includes_all():
    plan = plan_builds("push", linux_changed=False, android_changed=False)
    assert len(plan["matrix"]["include"]) == 4
    assert [e["platform"] for e in plan["matrix"]["include"]] == [
        "Linux",
        "Linux-22.04",
        "Linux-aarch64",
        "Android",
    ]
