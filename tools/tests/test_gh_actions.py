"""Tests for tools/common/gh_actions.py."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path
from typing import TYPE_CHECKING
from unittest.mock import patch

import common.gh_actions as mod
import pytest

from ._helpers import completed

if TYPE_CHECKING:
    from collections.abc import Callable

    from pytest_subprocess import FakeProcess
    from pytest_subprocess.fake_popen import FakePopen

pytestmark = pytest.mark.windows

GH_RATE_LIMIT = ["gh", "api", "rate_limit"]


def test_gh_uses_text_safe_subprocess_wrapper() -> None:
    result = subprocess.CompletedProcess(["gh", "api", "rate_limit"], 0, "{}", "")
    with patch.object(mod, "run_captured", return_value=result) as run:
        assert mod.gh("api", "rate_limit") is result
    run.assert_called_once_with(
        ["gh", "api", "rate_limit"], check=True, timeout=mod.DEFAULT_GH_TIMEOUT_SECONDS
    )


def test_gh_explicit_timeout_overrides_default() -> None:
    result = subprocess.CompletedProcess(["gh", "run", "download"], 0, "", "")
    with patch.object(mod, "run_captured", return_value=result) as run:
        mod.gh("run", "download", timeout=900)
    assert run.call_args.kwargs["timeout"] == 900


@pytest.fixture
def no_backoff(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("common.proc.time.sleep", lambda _seconds: None)


def _assert_captures_utf8_text(process: FakePopen) -> None:
    assert process.kwargs is not None
    assert process.kwargs["stdout"] == subprocess.PIPE
    assert process.kwargs["stderr"] == subprocess.PIPE
    assert (process.kwargs["encoding"], process.kwargs["errors"]) == ("utf-8", "replace")


@pytest.mark.usefixtures("no_backoff")
def test_gh_read_retry_recovers_from_transient_server_error(fp: FakeProcess) -> None:
    attempts = [
        fp.register(GH_RATE_LIMIT, returncode=1, stderr="gh: HTTP 502: Bad Gateway"),
        fp.register(GH_RATE_LIMIT, stdout="{}"),
    ]
    result = mod.gh("api", "rate_limit", retry_transient=True, max_attempts=2)
    assert result.stdout == "{}"
    for attempt in attempts:
        (process,) = attempt.calls
        _assert_captures_utf8_text(process)


@pytest.mark.usefixtures("no_backoff")
def test_gh_read_retry_exhausts_rate_limit_failures(fp: FakeProcess) -> None:
    fp.register(
        GH_RATE_LIMIT,
        returncode=1,
        stderr="gh: API rate limit exceeded (HTTP 429)",
        occurrences=3,
    )
    with pytest.raises(subprocess.CalledProcessError) as raised:
        mod.gh("api", "rate_limit", retry_transient=True, max_attempts=3)
    assert "HTTP 429" in raised.value.stderr
    assert fp.call_count(GH_RATE_LIMIT) == 3


@pytest.mark.usefixtures("no_backoff")
@pytest.mark.parametrize("check", [True, False])
def test_gh_read_retry_does_not_retry_permanent_errors(fp: FakeProcess, check: bool) -> None:
    fp.register(GH_RATE_LIMIT, returncode=1, stderr="gh: Not Found (HTTP 404)")
    if check:
        with pytest.raises(subprocess.CalledProcessError):
            mod.gh("api", "rate_limit", retry_transient=True)
    else:
        assert mod.gh("api", "rate_limit", retry_transient=True, check=False).returncode == 1
    assert fp.call_count(GH_RATE_LIMIT) == 1


@pytest.mark.usefixtures("no_backoff")
def test_gh_unchecked_read_retries_transient_result(fp: FakeProcess) -> None:
    fp.register(GH_RATE_LIMIT, returncode=1, stderr="gh: HTTP 503: Service Unavailable")
    fp.register(GH_RATE_LIMIT, stdout="{}")
    result = mod.gh("api", "rate_limit", retry_transient=True, check=False)
    assert (result.returncode, result.stdout) == (0, "{}")
    assert fp.call_count(GH_RATE_LIMIT) == 2


def test_gh_does_not_retry_mutation_by_default() -> None:
    failure = subprocess.CompletedProcess(["gh"], 1, "", "HTTP 503")
    with patch.object(mod, "run_captured", return_value=failure) as run:
        result = mod.gh("api", "repos/owner/repo/issues/1", "-X", "PATCH", check=False)
    assert result is failure
    run.assert_called_once()


def test_gh_rejects_retry_for_mutation() -> None:
    with (
        patch.object(mod, "run_with_retry") as run,
        pytest.raises(ValueError, match="read-only"),
    ):
        mod.gh("api", "repos/owner/repo/issues/1", "-X", "PATCH", retry_transient=True)
    run.assert_not_called()


def test_gh_rejects_retry_for_non_api_command() -> None:
    with (
        patch.object(mod, "run_with_retry") as run,
        pytest.raises(ValueError, match="read-only"),
    ):
        mod.gh("workflow", "run", "build.yml", retry_transient=True)
    run.assert_not_called()


def test_gh_api_json_parses_retried_read() -> None:
    payload = json.dumps({"id": 7, "status": "completed"})
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        assert mod.gh_api_json("repos/owner/repo/actions/runs/7") == {
            "id": 7,
            "status": "completed",
        }

    gh_mock.assert_called_once_with("api", "repos/owner/repo/actions/runs/7", retry_transient=True)


def test_list_workflow_runs_for_sha_uses_jq_get_method() -> None:
    payload = json.dumps({"id": 1, "name": "Linux"})
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert runs == [{"id": 1, "name": "Linux"}]
    called_args = gh_mock.call_args[0]
    assert "--method" in called_args
    assert "GET" in called_args
    assert ".workflow_runs[]?" in called_args
    assert gh_mock.call_args.kwargs["retry_transient"] is True


def test_list_workflow_runs_for_sha_unpacks_ndjson_stream() -> None:
    payload = (
        json.dumps({"id": 1, "name": "Linux"}) + "\n" + json.dumps({"id": 2, "name": "Windows"})
    )
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        runs = mod.list_workflow_runs_for_sha("owner/repo", "abc123")

    assert [run["id"] for run in runs] == [1, 2]
    gh_mock.assert_called_once()


def test_list_run_artifacts_parses_ndjson_stream() -> None:
    payload = (
        json.dumps({"name": "QGroundControl", "size_in_bytes": 1})
        + "\n"
        + json.dumps({"name": "QGroundControl2", "size_in_bytes": 2})
    )
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        artifacts = mod.list_run_artifacts("owner/repo", 77)

    assert [a["name"] for a in artifacts] == ["QGroundControl", "QGroundControl2"]
    called_args = gh_mock.call_args[0]
    assert ".artifacts[]?" in called_args
    assert gh_mock.call_args.kwargs["retry_transient"] is True
    gh_mock.assert_called_once()


def test_list_run_artifacts_rejects_invalid_run_id() -> None:
    try:
        mod.list_run_artifacts("owner/repo", "not-an-int")
    except ValueError as exc:
        assert "run_id must be an integer" in str(exc)
    else:
        raise AssertionError("Expected ValueError for invalid run_id")


def test_list_run_jobs_requests_the_latest_attempt() -> None:
    payload = json.dumps({"name": "Build", "steps": []})
    with patch.object(mod, "gh", return_value=completed(stdout=payload)) as gh_mock:
        jobs = mod.list_run_jobs("owner/repo", "77")

    assert jobs == [{"name": "Build", "steps": []}]
    args = gh_mock.call_args[0]
    assert "repos/owner/repo/actions/runs/77/jobs" in args
    assert ".jobs[]?" in args
    assert "filter=latest" in args
    with pytest.raises(ValueError, match="run_id must be positive"):
        mod.list_run_jobs("owner/repo", 0)


def test_require_repository_prefers_override_and_requires_value(capsys) -> None:
    with patch.dict(
        os.environ,
        {"GH_REPO": "override/repo", "GITHUB_REPOSITORY": "event/repo"},
        clear=True,
    ):
        assert mod.require_repository() == "override/repo"
    with patch.dict(os.environ, {"GITHUB_REPOSITORY": "event/repo"}, clear=True):
        assert mod.require_repository() == "event/repo"
    with patch.dict(
        os.environ,
        {"GH_REPO": "  ", "GITHUB_REPOSITORY": " fallback/repo "},
        clear=True,
    ):
        assert mod.require_repository() == "fallback/repo"
    with patch.dict(os.environ, {}, clear=True), pytest.raises(SystemExit):
        mod.require_repository()
    assert capsys.readouterr().out == "::error::GH_REPO or GITHUB_REPOSITORY must be set\n"


@pytest.mark.parametrize("cache_dir", [".cache/ccache", ".cache/moccache", ".cache/CPM", ".qt"])
def test_cache_path_survives_runner_workspace_changes(monkeypatch, tmp_path, cache_dir):
    paths = []
    for root in ("actions-runner/_work/qgc/qgc", "a/qgc/qgc"):
        workspace = tmp_path / root
        monkeypatch.setenv("GITHUB_WORKSPACE", str(workspace))
        paths.append(mod.github_cache_path(workspace / cache_dir))
        assert mod.github_cache_path(Path(cache_dir)) == cache_dir
    assert paths == [cache_dir, cache_dir]


def test_external_cache_path_is_not_relocated(monkeypatch, tmp_path):
    monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path / "workspace"))
    external = tmp_path / "sdk"
    assert mod.github_cache_path(external) == external.as_posix()
    assert mod.github_cache_path(Path("../sdk")) == external.as_posix()


class TestWriteGithubOutput:
    def test_simple_values(self, tmp_path) -> None:
        out = tmp_path / "output"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}, clear=False):
            mod.write_github_output({"key1": "val1", "key2": "val2"})
        content = out.read_text()
        assert "key1=val1\n" in content
        assert "key2=val2\n" in content

    def test_multiline_value(self, tmp_path) -> None:
        out = tmp_path / "output"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}, clear=False):
            mod.write_github_output({"body": "line1\nline2"})
        content = out.read_text()
        assert "body<<EOF_body_" in content
        assert "line1\nline2\n" in content

    def test_multiline_delimiter_resists_collision(self, tmp_path) -> None:
        out = tmp_path / "output"
        out.touch()
        value = "line1\nEOF_body_deadbeef\ntail"
        with patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}, clear=False):
            mod.write_github_output({"body": value})
        content = out.read_text()
        assert "body<<EOF_body_" in content
        assert "EOF_body\n" not in content
        assert value in content

    def test_noop_without_env(self) -> None:
        with patch.dict(os.environ, {}, clear=True):
            mod.write_github_output({"key": "val"})


class TestWriteStepSummary:
    def test_writes_markdown(self, tmp_path) -> None:
        out = tmp_path / "summary"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(out)}, clear=False):
            mod.write_step_summary("## Hello\n")
        assert out.read_text() == "## Hello\n"

    def test_noop_without_env(self) -> None:
        with patch.dict(os.environ, {}, clear=True):
            mod.write_step_summary("test")


class TestAnnotations:
    def test_gh_error_format(self, capsys) -> None:
        mod.gh_error("boom")
        assert capsys.readouterr().out == "::error::boom\n"

    def test_gh_warning_format(self, capsys) -> None:
        mod.gh_warning("careful")
        assert capsys.readouterr().out == "::warning::careful\n"

    def test_gh_notice_format(self, capsys) -> None:
        mod.gh_notice("fyi")
        assert capsys.readouterr().out == "::notice::fyi\n"

    def test_escapes_newlines_and_percent(self, capsys) -> None:
        mod.gh_error("a\nb\rc%d")
        assert capsys.readouterr().out == "::error::a%0Ab%0Dc%25d\n"

    def test_title_escapes_property_delimiters(self, capsys) -> None:
        mod.gh_error("FAIL!  : A::b()", title="Test failed: A, B\n50%")
        assert capsys.readouterr().out == (
            "::error title=Test failed%3A A%2C B%0A50%25::FAIL!  : A::b()\n"
        )

    @pytest.mark.parametrize("emit", [mod.gh_error, mod.gh_warning, mod.gh_notice])
    def test_location_properties_precede_the_title(self, capsys, emit) -> None:
        emit(
            "expected ';' after expression", title="Compile error", file="src/A.cc", line=42, col=7
        )
        level = emit.__name__.removeprefix("gh_")
        assert capsys.readouterr().out == (
            f"::{level} file=src/A.cc,line=42,col=7,title=Compile error::"
            "expected ';' after expression\n"
        )

    def test_file_property_escapes_delimiters(self, capsys) -> None:
        mod.gh_error("x", file="odd,dir/a:b%c.cc", line=3)
        assert capsys.readouterr().out == "::error file=odd%2Cdir/a%3Ab%25c.cc,line=3::x\n"

    def test_line_and_column_need_their_container(self, capsys) -> None:
        mod.gh_error("no file", line=3, col=4)
        mod.gh_error("no line", file="a.cc", col=4)
        assert capsys.readouterr().out == "::error::no file\n::error file=a.cc::no line\n"

    def test_emits_to_stdout_not_stderr(self, capsys) -> None:
        mod.gh_warning("x")
        captured = capsys.readouterr()
        assert captured.out == "::warning::x\n"
        assert captured.err == ""

    def test_group_closes_after_an_exception(self, capsys) -> None:
        with pytest.raises(RuntimeError), mod.gh_group("Logs\n50%"):
            print("line")
            raise RuntimeError
        assert capsys.readouterr().out == "::group::Logs%0A50%25\nline\n::endgroup::\n"


class TestAppendGithubEnv:
    def test_writes_env_vars(self, tmp_path) -> None:
        out = tmp_path / "env"
        out.touch()
        with patch.dict(os.environ, {"GITHUB_ENV": str(out)}, clear=False):
            mod.append_github_env({"FOO": "bar", "BAZ": "qux"})
        content = out.read_text()
        assert "FOO=bar\n" in content
        assert "BAZ=qux\n" in content

    def test_noop_without_env(self) -> None:
        with patch.dict(os.environ, {}, clear=True):
            mod.append_github_env({"FOO": "bar"})

    @pytest.mark.parametrize("value", ["a\nINJECTED=1", "a\rb", "a\r\n"])
    def test_rejects_line_breaks_before_writing(self, tmp_path, value: str) -> None:
        out = tmp_path / "env"
        out.touch()
        with (
            patch.dict(os.environ, {"GITHUB_ENV": str(out)}, clear=False),
            pytest.raises(ValueError, match="BAD must be a single line"),
        ):
            mod.append_github_env({"OK": "1", "BAD": value})
        assert out.read_text() == ""


class TestAppendGithubPath:
    def test_appends_entry(self, tmp_path) -> None:
        out = tmp_path / "path"
        with patch.dict(os.environ, {"GITHUB_PATH": str(out)}, clear=False):
            mod.append_github_path("/opt/tool/bin")
        assert out.read_text() == "/opt/tool/bin\n"

    def test_rejects_line_breaks(self, tmp_path) -> None:
        out = tmp_path / "path"
        with (
            patch.dict(os.environ, {"GITHUB_PATH": str(out)}, clear=False),
            pytest.raises(ValueError, match="single line"),
        ):
            mod.append_github_path("/opt/tool/bin\n/evil")
        assert not out.exists()


class TestIsCi:
    @pytest.mark.parametrize("name", ["CI", "GITHUB_ACTIONS"])
    @pytest.mark.parametrize("value", ["1", "true", "TRUE", "yes", "on"])
    def test_truthy_ci_variable(self, monkeypatch, name: str, value: str) -> None:
        monkeypatch.setenv(name, value)
        assert mod.is_ci() is True

    @pytest.mark.parametrize("value", [None, "0", "false", "no", "off", ""])
    def test_falsy_or_unset(self, monkeypatch, value: str | None) -> None:
        if value is not None:
            monkeypatch.setenv("CI", value)
        assert mod.is_ci() is False


class TestEnvDefaults:
    def test_env_default_treats_empty_as_unset(self, monkeypatch) -> None:
        monkeypatch.setenv("QGC_TEST_VALUE", "set")
        assert mod.env_default("QGC_TEST_VALUE", "fallback") == "set"
        monkeypatch.setenv("QGC_TEST_VALUE", "")
        assert mod.env_default("QGC_TEST_VALUE", "fallback") == "fallback"
        monkeypatch.delenv("QGC_TEST_VALUE")
        assert mod.env_default("QGC_TEST_VALUE") == ""

    @pytest.mark.parametrize(
        ("value", "default", "expected"),
        [("true", False, True), ("false", True, False), ("", True, True), (None, False, False)],
    )
    def test_env_flag(self, monkeypatch, value: str | None, default: bool, expected: bool) -> None:
        if value is None:
            monkeypatch.delenv("QGC_TEST_FLAG", raising=False)
        else:
            monkeypatch.setenv("QGC_TEST_FLAG", value)
        assert mod.env_flag("QGC_TEST_FLAG", default) is expected


ARTIFACT_ZIP = ["gh", "api", "repos/o/r/actions/artifacts/7/zip"]


def test_download_artifact_zip_writes_the_archive(fp: FakeProcess, tmp_path: Path) -> None:
    fp.register(ARTIFACT_ZIP, stdout=b"PK\x03\x04zip")
    destination = tmp_path / "nested" / "a.zip"
    mod.download_artifact_zip("o/r", 7, destination)
    assert destination.read_bytes() == b"PK\x03\x04zip"


def test_download_artifact_zip_retries_transient_failures(
    fp: FakeProcess, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr("common.gh_actions.time.sleep", lambda _seconds: None)
    fp.register(ARTIFACT_ZIP, returncode=1, stderr="gh: HTTP 502: Bad Gateway")
    fp.register(ARTIFACT_ZIP, stdout=b"zip")
    mod.download_artifact_zip("o/r", "7", tmp_path / "a.zip")
    assert fp.call_count(ARTIFACT_ZIP) == 2
    assert (tmp_path / "a.zip").read_bytes() == b"zip"


def _time_out_after_partial_write(destination: Path) -> Callable[[FakePopen], None]:
    def time_out(_process: FakePopen) -> None:
        destination.write_bytes(b"partial")
        raise subprocess.TimeoutExpired(ARTIFACT_ZIP, 0.01)

    return time_out


def test_download_artifact_zip_retries_a_timed_out_transfer(
    fp: FakeProcess, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr("common.gh_actions.time.sleep", lambda _seconds: None)
    destination = tmp_path / "a.zip"
    fp.register(ARTIFACT_ZIP, callback=_time_out_after_partial_write(destination))
    fp.register(ARTIFACT_ZIP, stdout=b"zip")
    mod.download_artifact_zip("o/r", 7, destination, timeout=0.01)
    assert fp.call_count(ARTIFACT_ZIP) == 2
    assert destination.read_bytes() == b"zip"


def test_download_artifact_zip_timeouts_end_in_the_cleanup_error(
    fp: FakeProcess, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr("common.gh_actions.time.sleep", lambda _seconds: None)
    destination = tmp_path / "a.zip"
    fp.register(ARTIFACT_ZIP, callback=_time_out_after_partial_write(destination), occurrences=2)
    with pytest.raises(RuntimeError, match=r"timed out after 0\.01s"):
        mod.download_artifact_zip("o/r", 7, destination, attempts=2, timeout=0.01)
    assert fp.call_count(ARTIFACT_ZIP) == 2
    assert not destination.exists()


def test_download_artifact_zip_fails_without_leaving_a_file(
    fp: FakeProcess, tmp_path: Path
) -> None:
    fp.register(ARTIFACT_ZIP, returncode=1, stderr="gh: Artifact has expired (HTTP 410)")
    with pytest.raises(RuntimeError, match="HTTP 410"):
        mod.download_artifact_zip("o/r", 7, tmp_path / "a.zip")
    assert fp.call_count(ARTIFACT_ZIP) == 1
    assert not (tmp_path / "a.zip").exists()
    with pytest.raises(ValueError, match="positive"):
        mod.download_artifact_zip("o/r", 0, tmp_path / "a.zip")


@pytest.mark.parametrize(
    ("value", "expected"),
    [
        ("2026-10-23T20:46:14Z", "2026-10-23T20:46:14+00:00"),
        ("2026-10-23T22:46:14+02:00", "2026-10-23T20:46:14+00:00"),
        ("2026-10-23T20:46:14", "2026-10-23T20:46:14+00:00"),
        ("", None),
        (None, None),
        ("yesterday", None),
    ],
)
def test_parse_github_timestamp(value: object, expected: str | None) -> None:
    parsed = mod.parse_github_timestamp(value)
    assert (parsed.isoformat() if parsed else None) == expected


def test_seconds_between_requires_an_ordered_pair() -> None:
    assert mod.seconds_between("2026-10-23T20:46:14Z", "2026-10-23T20:59:07Z") == 773.0
    assert mod.seconds_between("2026-10-23T20:59:07Z", "2026-10-23T20:46:14Z") is None
    assert mod.seconds_between(None, "2026-10-23T20:46:14Z") is None
