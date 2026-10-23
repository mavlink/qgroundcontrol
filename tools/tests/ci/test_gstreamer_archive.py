"""Tests for gstreamer_archive.py."""

from __future__ import annotations

from pathlib import Path

import pytest
from ci.gstreamer import gstreamer_archive
from ci.gstreamer.gstreamer_archive import ArchiveResult, GStreamerArchiver

PREFIX = Path("/opt/gstreamer")


def _archiver(platform: str, arch: str = "x86_64", **kwargs) -> GStreamerArchiver:
    return GStreamerArchiver(
        platform=platform, arch=arch, version="1.24.0", prefix=PREFIX, **kwargs
    )


class TestGStreamerArchiver:
    """Tests for GStreamerArchiver class."""

    def test_valid_platforms(self):
        """Test all valid platforms are accepted."""
        for platform in ["linux", "macos", "windows", "android", "ios"]:
            assert _archiver(platform).platform == platform

    def test_invalid_platform(self):
        """Test invalid platform raises ValueError."""
        with pytest.raises(ValueError, match="Unknown platform"):
            _archiver("freebsd")

    def test_get_archive_name_linux(self):
        """Test archive name generation for Linux."""
        assert _archiver("linux").get_archive_name() == "gstreamer-1.0-linux-x86_64-1.24.0"

    def test_get_archive_name_windows_uses_arch_verbatim(self):
        """The workflow's build step passes the Windows toolchain arch name (x64)."""
        assert _archiver("windows", "x64").get_archive_name() == "gstreamer-1.0-msvc-x64-1.24.0"

    def test_get_archive_name_macos(self):
        """Test archive name generation for macOS."""
        assert _archiver("macos", "arm64").get_archive_name() == "gstreamer-1.0-macos-arm64-1.24.0"

    def test_get_archive_name_ios_device(self):
        """Test archive name generation for iOS device."""
        assert _archiver("ios", "arm64").get_archive_name() == "gstreamer-1.0-ios-arm64-1.24.0"

    def test_get_archive_name_ios_simulator(self):
        """Test archive name generation for iOS simulator."""
        archiver = _archiver("ios", "arm64", simulator=True)
        assert archiver.get_archive_name() == "gstreamer-1.0-ios-arm64-simulator-1.24.0"

    def test_get_extension_windows(self):
        """Test file extension for Windows (zip)."""
        assert _archiver("windows", "x64")._get_extension() == "zip"

    def test_get_extension_linux(self):
        """Test file extension for Linux (tar.xz)."""
        assert _archiver("linux")._get_extension() == "tar.xz"

    def test_custom_output_dir(self):
        """Test custom output directory."""
        custom_dir = Path("/custom/output")
        assert _archiver("linux", output_dir=custom_dir).output_dir == custom_dir

    def test_prefix(self):
        """The prefix is used as given."""
        assert _archiver("linux").prefix == PREFIX


def test_prefix_is_required() -> None:
    with pytest.raises(SystemExit):
        gstreamer_archive.parse_args(["--platform", "linux", "--arch", "x86_64"])


def test_create_archive_writes_name_and_path_outputs(tmp_path: Path, gh_output: Path) -> None:
    prefix = tmp_path / "gstreamer"
    (prefix / "bin").mkdir(parents=True)
    (prefix / "bin" / "gst-launch-1.0").write_text("")
    archiver = GStreamerArchiver(
        platform="windows", arch="x64", version="1.24.0", prefix=prefix, output_dir=tmp_path
    )

    archive = archiver.create_archive()
    archiver.write_github_output()

    assert archive == tmp_path / "gstreamer-1.0-msvc-x64-1.24.0.zip"
    assert gh_output.read_text(encoding="utf-8").splitlines() == [
        "name=gstreamer-1.0-msvc-x64-1.24.0",
        f"path={archive}",
    ]


class TestArchiveResult:
    """Tests for ArchiveResult dataclass."""

    def test_archive_result_creation(self):
        """Test ArchiveResult creation."""
        result = ArchiveResult(
            name="gstreamer-1.0-linux-x86_64-1.24.0",
            path=Path("/tmp/archive.tar.xz"),
            extension="tar.xz",
        )
        assert result.name == "gstreamer-1.0-linux-x86_64-1.24.0"
        assert result.path == Path("/tmp/archive.tar.xz")
        assert result.extension == "tar.xz"


def _main(prefix: Path, output_dir: Path, *args: str) -> int:
    return gstreamer_archive.main(
        [
            "--platform",
            "windows",
            "--arch",
            "x64",
            "--version",
            "1.24.0",
            "--prefix",
            str(prefix),
            "--output-dir",
            str(output_dir),
            *args,
        ]
    )


@pytest.mark.parametrize(
    ("aws", "key_id", "error"),
    [
        (None, "AKIA", "::error::aws not found on PATH. --upload-s3 requires the AWS CLI"),
        ("/usr/bin/aws", "", "::error::--upload-s3 requires AWS credentials"),
    ],
    ids=["no-aws-cli", "no-credentials"],
)
def test_upload_without_aws_cli_or_credentials_fails_before_archiving(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys, aws, key_id, error
) -> None:
    monkeypatch.setattr(gstreamer_archive.shutil, "which", lambda name: aws)
    monkeypatch.setenv("AWS_ACCESS_KEY_ID", key_id)
    monkeypatch.setattr(gstreamer_archive, "upload_public_file", pytest.fail)

    assert _main(tmp_path / "gstreamer", tmp_path / "out", "--upload-s3") == 1

    assert error in capsys.readouterr().out
    assert not (tmp_path / "out").exists()


def test_upload_publishes_archive_to_the_versioned_gstreamer_key(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, gh_summary: Path
) -> None:
    prefix = tmp_path / "gstreamer"
    prefix.mkdir()
    (prefix / "README").write_text("sdk")
    monkeypatch.setattr(gstreamer_archive.shutil, "which", lambda name: "/usr/bin/aws")
    monkeypatch.setenv("AWS_ACCESS_KEY_ID", "AKIA")
    uploads = []
    monkeypatch.setattr(gstreamer_archive, "upload_public_file", lambda *args: uploads.append(args))

    assert _main(prefix, tmp_path, "--upload-s3") == 0

    archive = tmp_path / "gstreamer-1.0-msvc-x64-1.24.0.zip"
    key = "dependencies/gstreamer/windows/1.24.0/gstreamer-1.0-msvc-x64-1.24.0.zip"
    assert uploads == [(archive, "qgroundcontrol", key)]
    summary = gh_summary.read_text()
    assert summary.startswith("### GStreamer Windows SDK\n\n| Item | Value |")
    assert "| S3 path | `dependencies/gstreamer/windows/1.24.0/` |" in summary
