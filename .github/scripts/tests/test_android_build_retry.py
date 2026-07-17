"""Android deployment-settings truncation retry contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING

import android_build_retry as mod

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


_TRUNCATION = (
    "Invalid json file: /tmp/android-QGroundControl-deployment-settings.json. "
    "Reason: unterminated object\n"
)
_REPOSITORY_FAILURE = (
    "Could not GET "
    "'https://repo.maven.apache.org/maven2/example/example.pom'. "
    "Received status code 403 from server: Forbidden\n"
)


def test_truncation_detection_requires_complete_signature_and_tolerates_binary_prefix(
    tmp_path: Path,
) -> None:
    log = tmp_path / "build.log"
    for content, expected in (
        (_TRUNCATION.encode(), True),
        (b"ninja error\nReason: unterminated object\n", False),
        (b"\xff\xfe garbage\n" + _TRUNCATION.encode(), True),
    ):
        log.write_bytes(content)
        assert mod.detect_truncation(log) is expected


def test_settings_cleanup_handles_present_and_missing_files(tmp_path: Path) -> None:
    settings = tmp_path / "settings.json"
    settings.write_text("{ truncated")
    mod.clean_settings_json(settings)
    assert not settings.exists()
    mod.clean_settings_json(settings)


def test_gradle_repository_failure_detection_is_narrow(tmp_path: Path) -> None:
    log = tmp_path / "build.log"
    for content, expected in (
        (_REPOSITORY_FAILURE, True),
        (
            "Could not GET 'https://dl.google.com/android/example.pom'. "
            "Received status code 503 from server\n",
            True,
        ),
        (
            "Could not GET 'https://repo.maven.apache.org/example.pom'. "
            "Received status code 404 from server\n",
            False,
        ),
        ("Received status code 403 from an unrelated service\n", False),
    ):
        log.write_text(content)
        assert mod.detect_gradle_repository_failure(log) is expected


def test_main_rejects_missing_or_unrelated_logs(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    args = ["--build-dir", str(tmp_path), "--build-type", "Debug"]
    assert mod.main(args) == 1
    assert "is missing" in capsys.readouterr().out
    (tmp_path / "qgc-build.log").write_text("ninja: error: unrelated\n")
    assert mod.main(args) == 1
    assert "non-retriable" in capsys.readouterr().out


def test_main_cleans_settings_invokes_retry_and_propagates_status(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / "qgc-build.log").write_text(_TRUNCATION)
    settings = tmp_path / "android-QGroundControl-deployment-settings.json"
    settings.write_text("{ truncated")
    calls: list[tuple[Path, str, Path]] = []

    def retry(build_dir: Path, build_type: str, retry_log: Path) -> int:
        calls.append((build_dir, build_type, retry_log))
        return 2

    monkeypatch.setattr(mod, "retry_build", retry)
    assert mod.main(["--build-dir", str(tmp_path), "--build-type", "Release"]) == 2
    assert calls == [(tmp_path, "Release", tmp_path / "qgc-build-retry.log")]
    assert not settings.exists()


def test_main_retries_transient_gradle_repository_failure(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / "qgc-build.log").write_text(_REPOSITORY_FAILURE)
    settings = tmp_path / "android-QGroundControl-deployment-settings.json"
    settings.write_text("valid settings must not be removed")
    calls: list[tuple[Path, str, Path]] = []
    delays: list[int] = []

    monkeypatch.setattr(
        mod,
        "retry_build",
        lambda build_dir, build_type, retry_log: (
            calls.append((build_dir, build_type, retry_log)) or 0
        ),
    )
    monkeypatch.setattr(mod.time, "sleep", delays.append)

    assert mod.main(["--build-dir", str(tmp_path), "--build-type", "Release"]) == 0
    assert calls == [(tmp_path, "Release", tmp_path / "qgc-build-retry.log")]
    assert delays == [mod._REPOSITORY_RETRY_DELAY_SECONDS]
    assert settings.exists()
