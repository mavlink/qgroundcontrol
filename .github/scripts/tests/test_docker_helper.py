#!/usr/bin/env python3
"""Docker input validation and publication-policy contracts."""

from __future__ import annotations

import argparse
import subprocess

import pytest
from _helpers import REPO_ROOT
from docker_helper import cmd_validate, resolve_push_target


def test_build_inputs_accept_supported_pairs_and_reject_invalid_values() -> None:
    for target, build_type in (
        ("linux", "Release"),
        ("android", "Debug"),
        ("linux-cross", "Release"),
    ):
        cmd_validate(argparse.Namespace(target=target, build_type=build_type))
    for target, build_type in (("bogus", "Release"), ("linux", "BadType")):
        with pytest.raises(SystemExit):
            cmd_validate(argparse.Namespace(target=target, build_type=build_type))


def test_push_targets_are_limited_to_upstream_pushes() -> None:
    upstream = "mavlink/qgroundcontrol"
    expected = {
        "refs/tags/v5.0.0": "dronecode/qgroundcontrol",
        "refs/heads/master": "ghcr.io/mavlink/qgroundcontrol",
        "refs/heads/Stable_V4.4": "ghcr.io/mavlink/qgroundcontrol",
        "refs/heads/feature-x": "",
    }
    for ref, target in expected.items():
        assert resolve_push_target("push", upstream, ref) == target
        assert resolve_push_target("push", "someuser/qgroundcontrol", ref) == ""
    assert resolve_push_target("pull_request", upstream, "refs/tags/v5.0.0") == ""
    assert resolve_push_target("workflow_dispatch", upstream, "refs/heads/master") == ""


def test_apt_bootstrap_installs_ca_bundle_before_enabling_https() -> None:
    setup = (REPO_ROOT / "deploy/docker/lib/setup-base.sh").read_text(encoding="utf-8")

    assert ". /usr/local/lib/qgc/retry.sh" in setup
    assert 'APT::Update::Error-Mode "any";' in setup
    assert setup.count("retry apt-get update") == 2
    ca_install = "retry apt-get install -y --no-install-recommends ca-certificates"
    assert ca_install in setup
    assert "https://archive.ubuntu.com/ubuntu" in setup
    assert "https://security.ubuntu.com/ubuntu" in setup
    assert setup.index(ca_install) < setup.index(
        "'s|http://archive.ubuntu.com/ubuntu|https://archive.ubuntu.com/ubuntu|g'"
    )
    assert "mirror://mirrors.ubuntu.com" not in setup


def test_retry_helper_retries_and_propagates_final_failure() -> None:
    retry_helper = REPO_ROOT / "deploy/docker/lib/retry.sh"
    command = f"sleep() {{ :; }}; . '{retry_helper}'; retry false"

    result = subprocess.run(["sh", "-c", command], check=False, capture_output=True, text=True)

    assert result.returncode != 0
    assert result.stderr.count("retrying") == 2
