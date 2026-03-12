#!/usr/bin/env python3
"""Generate Build Results PR comment markdown."""

from __future__ import annotations

import argparse
import json
import logging
import os
import re
from datetime import UTC, datetime
from pathlib import Path
from typing import Any, Mapping
from urllib.parse import quote, urlsplit, urlunsplit

import jinja2

from xml_utils import XMLParseError, xml_parse as _xml_parse

logger = logging.getLogger(__name__)

_TEMPLATE_DIR = Path(__file__).parent / "templates"


def _env(env: Mapping[str, str], key: str, default: str = "") -> str:
    return str(env.get(key, default)).strip()


def _parse_coverage_percent(path: Path) -> float | None:
    if not path.exists():
        return None
    try:
        root = _xml_parse(path).getroot()
        if int(root.get("lines-valid", 0)) == 0:
            return None
        return float(root.get("line-rate", 0.0)) * 100.0
    except (XMLParseError, OSError, ValueError):
        logger.debug("Failed to parse coverage from %s", path, exc_info=True)
        return None


def _parse_precommit_results(path: Path) -> tuple[str | None, str | None, str | None]:
    if not path.exists():
        return None, None, None

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        logger.debug("Failed to parse pre-commit results from %s", path, exc_info=True)
        return None, None, None

    exit_code = str(data.get("exit_code", "1")).strip()
    passed = str(data.get("passed", "0")).strip()
    failed = str(data.get("failed", "0")).strip()
    skipped = str(data.get("skipped", "0")).strip()
    run_url = str(data.get("run_url", "")).strip()

    status = "Passed" if exit_code == "0" else "Failed (non-blocking)"
    details = _view_link(run_url)
    note = f"Pre-commit hooks: {passed} passed, {failed or '0'} failed, {skipped or '0'} skipped."
    return status, details, note


def _sanitize_external_url(url: str) -> str | None:
    value = url.strip()
    if not value:
        return None
    if any(ch in value for ch in ("\r", "\n", "\t")):
        return None

    parsed = urlsplit(value)
    if parsed.scheme not in {"http", "https"} or not parsed.netloc:
        return None

    path = quote(parsed.path, safe="/-._~!$&'*,;=:@%+")
    query = quote(parsed.query, safe="&=:@/?-._~!$'*,;%+")
    fragment = quote(parsed.fragment, safe="-._~!$&'*,;=:@/?%+")
    return urlunsplit((parsed.scheme, parsed.netloc, path, query, fragment))


def _view_link(url: str) -> str | None:
    safe_url = _sanitize_external_url(url)
    if not safe_url:
        return None
    return f"[View]({safe_url})"


def _count_test_results(content: str) -> tuple[int, int, int]:
    passed = len(re.findall(r"Test #[0-9]+: .* Passed", content))
    failed = len(re.findall(r"Test #[0-9]+: .* \*\*\*Failed", content))
    skipped = len(re.findall(r"Test #[0-9]+: .* (\*\*\*Skipped|Skipped)", content))

    if passed == 0 and failed == 0 and skipped == 0:
        passed = len(re.findall(r"^PASS", content, re.MULTILINE))
        failed = len(re.findall(r"^FAIL", content, re.MULTILINE))
        skipped = len(re.findall(r"^SKIP", content, re.MULTILINE))

    return passed, failed, skipped


def _failed_test_lines(content: str, limit: int = 20) -> list[str]:
    lines: list[str] = []
    for line in content.splitlines():
        if re.search(r"Test #[0-9]+: .* \*\*\*Failed|^FAIL", line):
            lines.append(line)
            if len(lines) >= limit:
                break
    return lines


def _format_delta_mb(delta_bytes: int) -> str:
    delta_mb = delta_bytes / 1024.0 / 1024.0
    if delta_bytes > 0:
        return f"+{delta_mb:.2f} MB (increase)"
    if delta_bytes < 0:
        return f"{delta_mb:.2f} MB (decrease)"
    return "No change"


def _format_size_human(size_bytes: int) -> str:
    size_mb = size_bytes / 1024.0 / 1024.0
    if size_mb >= 1024:
        return f"{(size_mb / 1024):.2f} GB"
    return f"{size_mb:.2f} MB"


def _collect_test_data(base_dir: Path, env: Mapping[str, str]) -> dict[str, Any] | None:
    pattern = _env(env, "TEST_RESULTS_GLOB", "artifacts/test-results-*/test-output.txt")
    files = sorted(base_dir.glob(pattern))
    if not files:
        return None

    platforms: list[dict[str, Any]] = []
    total_passed = total_failed = total_skipped = 0
    has_failures = False

    for file in files:
        arch = file.parent.name.removeprefix("test-results-")
        content = file.read_text(encoding="utf-8", errors="ignore")
        passed, failed, skipped = _count_test_results(content)
        total_passed += passed
        total_failed += failed
        total_skipped += skipped

        entry: dict[str, Any] = {"arch": arch, "passed": passed, "failed": failed, "skipped": skipped}
        if failed > 0:
            has_failures = True
            entry["failed_lines"] = _failed_test_lines(content)
        platforms.append(entry)

    return {
        "platforms": platforms,
        "total_passed": total_passed,
        "total_failed": total_failed,
        "total_skipped": total_skipped,
        "has_failures": has_failures,
    }


def _collect_coverage_data(base_dir: Path, env: Mapping[str, str]) -> dict[str, Any] | None:
    coverage_path = base_dir / _env(env, "COVERAGE_XML", "artifacts/coverage-report/coverage.xml")
    if not coverage_path.exists():
        return None

    current = _parse_coverage_percent(coverage_path)
    baseline_path = base_dir / _env(env, "BASELINE_COVERAGE_XML", "baseline-coverage.xml")
    baseline = _parse_coverage_percent(baseline_path)

    result: dict[str, Any] = {"current": current, "baseline": baseline}
    if baseline is not None and current is not None:
        result["diff"] = current - baseline
    return result


def _collect_artifact_data(base_dir: Path, env: Mapping[str, str]) -> dict[str, Any] | None:
    pr_sizes_path = base_dir / _env(env, "PR_SIZES_JSON", "pr-sizes.json")
    if not pr_sizes_path.exists():
        return None

    try:
        pr_data = json.loads(pr_sizes_path.read_text(encoding="utf-8"))
    except Exception:
        logger.debug("Failed to parse PR sizes from %s", pr_sizes_path, exc_info=True)
        return None

    baseline_path = base_dir / _env(env, "BASELINE_SIZES_JSON", "baseline-sizes.json")
    baseline: dict[str, int] = {}
    if baseline_path.exists():
        try:
            baseline_data = json.loads(baseline_path.read_text(encoding="utf-8"))
            for artifact in baseline_data.get("artifacts", []):
                if not isinstance(artifact, dict):
                    continue
                name = str(artifact.get("name", "")).strip()
                if not name:
                    continue
                try:
                    baseline[name] = int(artifact.get("size_bytes", 0))
                except (TypeError, ValueError):
                    continue
        except Exception:
            logger.debug("Failed to parse baseline sizes from %s", baseline_path, exc_info=True)
            baseline = {}

    artifacts = pr_data.get("artifacts", [])
    if not isinstance(artifacts, list):
        return None

    items: list[dict[str, Any]] = []
    total_delta = 0
    for artifact in artifacts:
        if not isinstance(artifact, dict):
            continue
        name = str(artifact.get("name", "")).strip()
        if not name:
            continue
        try:
            new_size = int(artifact.get("size_bytes", 0))
        except (TypeError, ValueError):
            continue
        size_human = str(artifact.get("size_human", "")).strip() or _format_size_human(new_size)

        entry: dict[str, Any] = {"name": name, "size_human": size_human}
        if baseline and name in baseline:
            delta = new_size - baseline[name]
            total_delta += delta
            entry["delta"] = delta
            entry["delta_human"] = _format_delta_mb(delta)
        items.append(entry)

    return {
        "entries": items,
        "has_baseline": bool(baseline),
        "total_delta": total_delta,
        "total_delta_mb": abs(total_delta) / 1024.0 / 1024.0,
    }


def generate_comment(env: Mapping[str, str], base_dir: Path, now_utc: datetime | None = None) -> str:
    table = _env(env, "BUILD_TABLE")
    summary = _env(env, "BUILD_SUMMARY", "Some builds still in progress.")
    precommit_status = _env(env, "PRECOMMIT_STATUS", "Not Triggered")
    precommit_url = _env(env, "PRECOMMIT_URL")
    triggered_by = _env(env, "TRIGGERED_BY", "Unknown")

    precommit_details = _view_link(precommit_url) or "-"
    precommit_note = ""

    precommit_path = base_dir / _env(env, "PRECOMMIT_RESULTS_PATH", "artifacts/pre-commit-results/pre-commit-results.json")
    parsed_status, parsed_details, parsed_note = _parse_precommit_results(precommit_path)
    if parsed_status:
        precommit_status = parsed_status
    if parsed_details:
        precommit_details = parsed_details
    if parsed_note:
        precommit_note = parsed_note

    now = now_utc or datetime.now(UTC)

    jinja_env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(_TEMPLATE_DIR),
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
    )
    template = jinja_env.get_template("build_results.md.j2")

    rendered = template.render(
        table=table,
        summary=summary,
        precommit={"status": precommit_status, "details": precommit_details, "note": precommit_note},
        tests=_collect_test_data(base_dir, env),
        coverage=_collect_coverage_data(base_dir, env),
        artifacts=_collect_artifact_data(base_dir, env),
        timestamp=now.strftime("%Y-%m-%d %H:%M:%S UTC"),
        triggered_by=triggered_by,
    )

    # Normalize: collapse 3+ blank lines to 2, strip trailing whitespace per line
    lines = [line.rstrip() for line in rendered.splitlines()]
    result: list[str] = []
    blank_count = 0
    for line in lines:
        if not line:
            blank_count += 1
            if blank_count <= 2:
                result.append(line)
        else:
            blank_count = 0
            result.append(line)

    return "\n".join(result).rstrip() + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate Build Results PR comment markdown.",
    )
    parser.add_argument(
        "--base-dir",
        type=Path,
        default=Path.cwd(),
        help="Base directory for resolving relative paths (default: cwd)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output path for comment markdown (default: $COMMENT_OUTPUT or comment.md)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    base_dir = args.base_dir
    comment = generate_comment(os.environ, base_dir)
    if args.output:
        output_path = args.output
    else:
        output_path = base_dir / _env(os.environ, "COMMENT_OUTPUT", "comment.md")
    output_path.write_text(comment, encoding="utf-8")
    print(comment, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
