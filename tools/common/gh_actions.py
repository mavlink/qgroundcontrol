"""Shared helpers for GitHub Actions API access via gh CLI."""

from __future__ import annotations

import contextlib
import json
import os
import re
import subprocess
from pathlib import Path
from typing import TYPE_CHECKING, Any

from .proc import run_captured, run_with_retry

if TYPE_CHECKING:
    from collections.abc import Iterator, Sequence


_TRANSIENT_GH_ERRORS = (
    "api rate limit exceeded",
    "secondary rate limit",
    "connection reset",
    "connection refused",
    "could not resolve host",
    "error connecting",
    "network is unreachable",
    "temporary failure",
    "timed out",
    "timeout",
    "tls handshake timeout",
)


def _is_transient_gh_output(stdout: object, stderr: object) -> bool:
    output = f"{stdout or ''}\n{stderr or ''}".lower()
    return bool(re.search(r"\b(?:http|status(?: code)?)[ :=]*(?:429|5\d\d)\b", output)) or any(
        marker in output for marker in _TRANSIENT_GH_ERRORS
    )


def _is_transient_gh_error(error: Exception) -> bool:
    if isinstance(error, subprocess.TimeoutExpired):
        return True
    return isinstance(error, subprocess.CalledProcessError) and _is_transient_gh_output(
        error.stdout, error.stderr
    )


def _is_transient_gh_result(result: subprocess.CompletedProcess[Any]) -> bool:
    return result.returncode != 0 and _is_transient_gh_output(result.stdout, result.stderr)


def _gh_api_method(args: Sequence[str]) -> str | None:
    if not args or args[0] != "api":
        return None
    for index, arg in enumerate(args):
        if arg in {"-X", "--method"} and index + 1 < len(args):
            return args[index + 1].upper()
        if arg.startswith("--method="):
            return arg.partition("=")[2].upper()
    return "POST" if any(arg in {"-f", "-F", "--raw-field", "--field"} for arg in args) else "GET"


# Bounds calls that set no timeout; long transfers such as `gh run download` pass their own.
DEFAULT_GH_TIMEOUT_SECONDS = 120.0


def gh(
    *args: str,
    check: bool = True,
    retry_transient: bool = False,
    max_attempts: int = 3,
    retry_backoff_seconds: float = 2.0,
    timeout: float | None = None,
) -> subprocess.CompletedProcess[str]:
    """Run gh, optionally retrying transient failures for read-only operations."""
    command = ["gh", *args]
    if not retry_transient:
        return run_captured(
            command,
            check=check,
            timeout=timeout if timeout is not None else DEFAULT_GH_TIMEOUT_SECONDS,
        )
    method = _gh_api_method(args)
    if method != "GET":
        raise ValueError("retry_transient is only valid for read-only GitHub API GET calls")
    return run_with_retry(
        command,
        max_attempts=max_attempts,
        retry_backoff_seconds=retry_backoff_seconds,
        retry_if=_is_transient_gh_error,
        retry_result_if=_is_transient_gh_result,
        timeout=timeout if timeout is not None else 60,
        check=check,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def gh_api_json(path: str) -> Any:
    """GET a GitHub REST API path, retrying transient failures, and return the parsed JSON."""
    return json.loads(gh("api", path, retry_transient=True).stdout)


def _paginate_items(path: str, item_key: str, params: dict[str, str]) -> list[dict[str, Any]]:
    """Fetch all pages of a GitHub list endpoint and return the unpacked items.

    ``gh --paginate --jq`` applies the filter per page and streams one JSON
    value per line (NDJSON), so the list is unpacked server-side; ``[]?``
    tolerates pages where the key is absent.
    """
    cmd = ["api", "--method", "GET", "--paginate", "--jq", f".{item_key}[]?", path]
    for key, value in params.items():
        cmd += ["-F", f"{key}={value}"]
    result = gh(*cmd, retry_transient=True)
    return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]


def list_workflow_runs_for_sha(repo: str, head_sha: str) -> list[dict[str, Any]]:
    """List workflow runs for a commit SHA across all paginated API results."""
    return _paginate_items(
        f"repos/{repo}/actions/runs",
        "workflow_runs",
        {"head_sha": head_sha, "per_page": "100"},
    )


def list_run_artifacts(repo: str, run_id: int | str) -> list[dict[str, Any]]:
    """List artifacts for a workflow run across all pages."""
    try:
        run_id_int = int(run_id)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"run_id must be an integer, got {run_id!r}") from exc
    if run_id_int <= 0:
        raise ValueError(f"run_id must be positive, got {run_id_int}")

    return _paginate_items(
        f"repos/{repo}/actions/runs/{run_id_int}/artifacts",
        "artifacts",
        {"per_page": "100"},
    )


def list_run_jobs(repo: str, run_id: int | str) -> list[dict[str, Any]]:
    """List the jobs of a workflow run's latest attempt, with their steps, across all pages."""
    try:
        run_id_int = int(run_id)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"run_id must be an integer, got {run_id!r}") from exc
    if run_id_int <= 0:
        raise ValueError(f"run_id must be positive, got {run_id_int}")

    return _paginate_items(
        f"repos/{repo}/actions/runs/{run_id_int}/jobs",
        "jobs",
        {"per_page": "100", "filter": "latest"},
    )


def parse_csv_list(value: str) -> list[str]:
    """Parse comma-separated values into a trimmed non-empty list."""
    return [item.strip() for item in value.split(",") if item.strip()]


def require_repository() -> str:
    """Return the configured owner/repo or terminate with a CI annotation."""
    for variable in ("GH_REPO", "GITHUB_REPOSITORY"):
        if repo := os.environ.get(variable, "").strip():
            return repo
    gh_error("GH_REPO or GITHUB_REPOSITORY must be set")
    raise SystemExit(1)


def github_cache_path(path: Path) -> str:
    """Use a stable archive path inside the workspace, preserving external paths.

    actions/cache hashes the literal path input into its cache version. Absolute
    workspace paths prevent sharing between hosted and self-hosted runners.
    """
    workspace = Path(os.environ.get("GITHUB_WORKSPACE", ".")).resolve()
    absolute = (workspace / path).resolve()
    try:
        return absolute.relative_to(workspace).as_posix()
    except ValueError:
        return absolute.as_posix()


def write_github_output(outputs: dict[str, str]) -> None:
    """Write key=value pairs to $GITHUB_OUTPUT for GitHub Actions.

    Handles multiline values using hash-based heredoc delimiters.
    """
    import hashlib

    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        return

    with open(github_output, "a", encoding="utf-8") as f:
        for key, value in outputs.items():
            if "\n" in value:
                value_hash = hashlib.sha256(value.encode("utf-8")).hexdigest()[:12]
                delim = f"EOF_{key}_{value_hash}"
                while delim in value:
                    delim = f"{delim}_X"
                f.write(f"{key}<<{delim}\n{value}\n{delim}\n")
            else:
                f.write(f"{key}={value}\n")


def write_step_summary(markdown: str) -> None:
    """Append markdown content to $GITHUB_STEP_SUMMARY."""
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        f.write(markdown)


def _require_single_line(name: str, value: str) -> None:
    # The runner reads these files line by line, so a line break would start another entry.
    if "\n" in value or "\r" in value:
        raise ValueError(f"{name} must be a single line: {value!r}")


def append_github_env(values: dict[str, str]) -> None:
    """Append single-line environment variables to $GITHUB_ENV.

    Raises ValueError, before writing anything, when a value contains CR or LF.
    """
    for key, value in values.items():
        _require_single_line(key, value)
    path = os.environ.get("GITHUB_ENV")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        for key, value in values.items():
            f.write(f"{key}={value}\n")


def append_github_path(path_entry: str) -> None:
    """Append a path entry to $GITHUB_PATH for subsequent steps; CR/LF raise ValueError."""
    _require_single_line("GITHUB_PATH entry", path_entry)
    path = os.environ.get("GITHUB_PATH")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        f.write(f"{path_entry}\n")


def parse_bool(value: str | None) -> bool:
    """Parse a CI-style boolean. Accepts 1/true/yes/on (case-insensitive)."""
    if value is None:
        return False
    return value.strip().lower() in {"1", "true", "yes", "on"}


def is_ci() -> bool:
    """True when running in any CI environment (``CI`` or ``GITHUB_ACTIONS`` is truthy)."""
    return parse_bool(os.environ.get("CI")) or parse_bool(os.environ.get("GITHUB_ACTIONS"))


def env_default(name: str, default: str = "") -> str:
    """Return environment variable ``name``, or ``default`` when it is unset or empty.

    Lets argparse defaults come from a workflow step's ``env:`` instead of shell flag assembly.
    """
    return os.environ.get(name) or default


def env_flag(name: str, default: bool = False) -> bool:
    """Return parse_bool of environment variable ``name``, or ``default`` when unset or empty."""
    value = os.environ.get(name)
    return parse_bool(value) if value else default


def _escape_annotation(message: str) -> str:
    # Workflow commands are newline-delimited; literal CR/LF truncates the
    # annotation, so percent-encode them (and bare % to keep decoding unambiguous).
    return message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def annotate(level: str, message: str, *, title: str = "") -> None:
    """Emit a GitHub Actions workflow annotation to stdout.

    level is one of "error", "warning", "notice". The runner parses these from
    the step log and renders them in the run UI; outside CI it is plain text.
    """
    # Property values also end at ':' and ','.
    props = (
        f" title={_escape_annotation(title).replace(':', '%3A').replace(',', '%2C')}"
        if title
        else ""
    )
    print(f"::{level}{props}::{_escape_annotation(message)}", flush=True)


def gh_error(message: str, *, title: str = "") -> None:
    """Emit a GitHub Actions error annotation."""
    annotate("error", message, title=title)


def gh_warning(message: str, *, title: str = "") -> None:
    """Emit a GitHub Actions warning annotation."""
    annotate("warning", message, title=title)


def gh_notice(message: str) -> None:
    """Emit a GitHub Actions notice annotation."""
    annotate("notice", message)


@contextlib.contextmanager
def gh_group(title: str) -> Iterator[None]:
    """Fold output printed inside the block into a collapsible GitHub Actions log group."""
    print(f"::group::{_escape_annotation(title)}", flush=True)
    try:
        yield
    finally:
        print("::endgroup::", flush=True)
