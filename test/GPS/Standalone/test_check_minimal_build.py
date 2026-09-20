"""Regression contracts for isolated GPS build scheduling and failure handling."""

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import check_minimal_build as checker


class MinimalBuildTest(unittest.TestCase):
    def test_run_uses_requested_timeout_and_propagates_failures(self) -> None:
        completed = subprocess.CompletedProcess(["cmake"], 0, "built")
        with patch.object(checker.subprocess, "run", return_value=completed) as command:
            self.assertEqual(checker.run(["cmake"], timeout=300), "built")
            self.assertEqual(command.call_args.kwargs["timeout"], 300)
            checker.run(["ctest"])
            self.assertEqual(command.call_args.kwargs["timeout"], 120)
        with (
            patch.object(
                checker.subprocess,
                "run",
                return_value=subprocess.CompletedProcess(["cmake"], 1, "compiler failed"),
            ),
            self.assertRaisesRegex(ValueError, "compiler failed"),
        ):
            checker.run(["cmake"], timeout=300)
        with (
            patch.object(
                checker.subprocess,
                "run",
                side_effect=subprocess.TimeoutExpired(["cmake"], 300),
            ),
            self.assertRaises(subprocess.TimeoutExpired),
        ):
            checker.run(["cmake"], timeout=300)

    def test_builds_honor_jobs_and_keep_all_checks(self) -> None:
        for component in ("Receiver", "Native", "Driver"):
            for jobs in (None, 1, 8):
                with self.subTest(component=component, jobs=jobs):
                    self._check_build(component, jobs)

    def _check_build(self, component: str, jobs: int | None) -> None:
        calls: list[tuple[list[str], int]] = []
        expected_tests, header_target = checker.CASES[component]

        def run(command: list[str], *, timeout: int = 120) -> str:
            calls.append((command, timeout))
            if "--show-only=json-v1" in command:
                return json.dumps({"tests": [{"name": name} for name in sorted(expected_tests)]})
            return ""

        args = argparse.Namespace(
            component=component,
            config="Debug",
            source_dir=Path("source"),
            cmake="cmake",
            ctest="ctest",
            generator="Ninja",
            compiler="c++",
            qt_dir="qt",
            cpm_source_cache="",
            platform="",
            toolset="",
            parallel=jobs,
            build_timeout=300,
        )
        with tempfile.TemporaryDirectory(prefix="gps-build-contract-") as directory:
            build = Path(directory)
            with (
                patch.object(checker, "run", side_effect=run),
                patch.object(checker, "check_artifacts") as artifacts,
            ):
                checker.check_build(args, build)
                artifacts.assert_called_once_with(build, component, "Debug")
        builds = [(command, timeout) for command, timeout in calls if "--build" in command]
        self.assertEqual(len(builds), 2)
        for command, timeout in builds:
            self.assertEqual(timeout, 300)
            position = command.index("--parallel")
            if jobs is None:
                self.assertTrue(position == len(command) - 1 or command[position + 1] == "--target")
            else:
                self.assertEqual(command[position + 1], str(jobs))
        self.assertNotIn("--target", builds[0][0])
        self.assertEqual(builds[1][0][-2:], ["--target", header_target])
        self.assertIn("--no-tests=error", calls[-1][0])
        self.assertEqual(calls[0][1], 120)
        self.assertEqual(calls[-1][1], 120)

    def test_nonpositive_limits_are_rejected(self) -> None:
        for value in ("0", "-1"):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                checker.positive_integer(value)
        self.assertEqual(checker.positive_integer("8"), 8)


if __name__ == "__main__":
    unittest.main()
