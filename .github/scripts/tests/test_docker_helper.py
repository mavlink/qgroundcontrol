#!/usr/bin/env python3
"""Tests for docker_helper.py."""

from __future__ import annotations

import argparse

import pytest

from docker_helper import VALID_BUILD_TYPES, VALID_DOCKERFILES, cmd_validate


class TestValidate:
    def test_valid_ubuntu_release(self) -> None:
        args = argparse.Namespace(dockerfile="Dockerfile-build-ubuntu", build_type="Release")
        cmd_validate(args)

    def test_valid_android_debug(self) -> None:
        args = argparse.Namespace(dockerfile="Dockerfile-build-android", build_type="Debug")
        cmd_validate(args)

    def test_invalid_dockerfile(self) -> None:
        args = argparse.Namespace(dockerfile="Dockerfile-bad", build_type="Release")
        with pytest.raises(SystemExit):
            cmd_validate(args)

    def test_invalid_build_type(self) -> None:
        args = argparse.Namespace(dockerfile="Dockerfile-build-ubuntu", build_type="BadType")
        with pytest.raises(SystemExit):
            cmd_validate(args)
