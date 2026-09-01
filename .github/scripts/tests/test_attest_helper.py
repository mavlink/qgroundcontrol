"""Tests for attest_helper.py."""

from __future__ import annotations

from unittest.mock import patch


class TestAttestHelper:
    """Tests for attest_helper main logic."""

    def _run_main(self, args: list[str], *, event_name: str = "push") -> dict[str, str]:
        """Run attest_helper.main with mocked dependencies, return captured outputs."""
        captured = {}

        def fake_write_output(d: dict) -> None:
            captured.update(d)

        with (
            patch.dict("os.environ", {"GITHUB_EVENT_NAME": event_name}, clear=False),
            patch("attest_helper.write_github_output", side_effect=fake_write_output),
        ):
            import sys

            import attest_helper

            old_argv = sys.argv
            try:
                sys.argv = ["attest_helper.py", *args]
                attest_helper.main()
            finally:
                sys.argv = old_argv
        return captured

    def test_pull_request_skips(self, tmp_path):
        subject = tmp_path / "artifact.zip"
        subject.touch()
        outputs = self._run_main(
            [
                "check",
                "--subject-path",
                str(subject),
                "--subject-name",
                "my-artifact",
                "--runner-temp",
                str(tmp_path),
            ],
            event_name="pull_request",
        )
        assert outputs == {"skip": "true"}

    def test_spdx_sbom_path(self, tmp_path):
        """SBOM path uses .spdx.json suffix for spdx-json format."""
        subject = tmp_path / "artifact.zip"
        subject.touch()
        outputs = self._run_main(
            [
                "check",
                "--subject-path",
                str(subject),
                "--subject-name",
                "my-artifact",
                "--sbom-format",
                "spdx-json",
                "--runner-temp",
                str(tmp_path),
            ]
        )
        assert outputs["skip"] == "false"
        assert outputs["sbom-path"] == str(tmp_path / "my-artifact.sbom.spdx.json")

    def test_cyclonedx_sbom_path(self, tmp_path):
        """SBOM path uses .cdx.json suffix for cyclonedx-json format."""
        subject = tmp_path / "artifact.zip"
        subject.touch()
        outputs = self._run_main(
            [
                "check",
                "--subject-path",
                str(subject),
                "--subject-name",
                "my-artifact",
                "--sbom-format",
                "cyclonedx-json",
                "--runner-temp",
                str(tmp_path),
            ]
        )
        assert outputs["skip"] == "false"
        assert outputs["sbom-path"] == str(tmp_path / "my-artifact.sbom.cdx.json")

    def test_scan_path_defaults_to_parent(self, tmp_path):
        """When scan-path is empty, defaults to subject's parent directory."""
        subject = tmp_path / "sub" / "artifact.zip"
        subject.parent.mkdir(parents=True, exist_ok=True)
        subject.touch()
        outputs = self._run_main(
            [
                "check",
                "--subject-path",
                str(subject),
                "--subject-name",
                "my-artifact",
                "--runner-temp",
                str(tmp_path),
            ]
        )
        assert outputs["scan-path"] == str(subject.parent)

    def test_resolve_path_uses_default_when_no_override(self, tmp_path):
        artifact = tmp_path / "Out.AppImage"
        artifact.touch()
        outputs = self._run_main(
            [
                "resolve-path",
                "--default",
                str(artifact),
            ]
        )
        assert outputs == {"parent": str(artifact.parent), "path": str(artifact)}

    def test_resolve_path_prefers_override(self, tmp_path):
        override = tmp_path / "override.zip"
        override.touch()
        default = tmp_path / "default.zip"
        default.touch()
        outputs = self._run_main(
            [
                "resolve-path",
                "--override",
                str(override),
                "--default",
                str(default),
            ]
        )
        assert outputs == {"parent": str(override.parent), "path": str(override)}

    def test_resolve_path_missing_artifact_exits(self, tmp_path):
        import pytest

        with pytest.raises(SystemExit) as excinfo:
            self._run_main(
                [
                    "resolve-path",
                    "--default",
                    str(tmp_path / "missing.zip"),
                ]
            )
        assert excinfo.value.code == 1

    def test_checksum_creates_sidecar(self, tmp_path):
        artifact = tmp_path / "QGroundControl.AppImage"
        artifact.write_bytes(b"artifact")

        outputs = self._run_main(["checksum", "--source-path", str(artifact)])

        checksum = tmp_path / "QGroundControl.AppImage.sha256"
        assert outputs == {"path": str(checksum)}
        assert checksum.read_text(encoding="utf-8").endswith("  QGroundControl.AppImage\n")

    def test_checksum_rejects_mismatch(self, tmp_path):
        import pytest

        artifact = tmp_path / "QGroundControl.dmg"
        artifact.write_bytes(b"artifact")
        checksum = tmp_path / "QGroundControl.dmg.sha256"
        checksum.write_text(f"{'0' * 64}  QGroundControl.dmg\n", encoding="utf-8")

        with pytest.raises(SystemExit) as excinfo:
            self._run_main(["checksum", "--source-path", str(artifact)])

        assert excinfo.value.code == 1
