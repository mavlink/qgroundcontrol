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
from typing import Mapping

try:
    from defusedxml.ElementTree import parse as _xml_parse
except ImportError:
    from xml.etree.ElementTree import parse as _xml_parse

logger = logging.getLogger(__name__)


def _env(env: Mapping[str, str], key: str, default: str = "") -> str:
    return str(env.get(key, default)).strip()


def _parse_coverage_percent(path: Path) -> float | None:
    if not path.exists():
        return None
    try:
        root = _xml_parse(path).getroot()
        return float(root.get("line-rate", 0.0)) * 100.0
    except Exception:
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

    status = "Passed" if exit_code == "0" else "Failed"
    details = f"[View]({run_url})" if run_url else None
    note = f"Pre-commit hooks: {passed} passed, {failed or '0'} failed, {skipped or '0'} skipped."
    return status, details, note


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


def _render_test_results(base_dir: Path, env: Mapping[str, str]) -> list[str]:
    pattern = _env(env, "TEST_RESULTS_GLOB", "artifacts/test-results-*/test-output.txt")
    files = sorted(base_dir.glob(pattern))
    if not files:
        return []

    lines = ["### Test Results", ""]
    total_passed = 0
    total_failed = 0
    total_skipped = 0
    has_failures = False

    for file in files:
        arch = file.parent.name.removeprefix("test-results-")
        content = file.read_text(encoding="utf-8", errors="ignore")
        passed, failed, skipped = _count_test_results(content)

        total_passed += passed
        total_failed += failed
        total_skipped += skipped

        if failed > 0:
            has_failures = True
            lines.append(f"**{arch}**: {passed} passed, {failed} failed, {skipped} skipped")
            lines.append("")
            lines.append("<details><summary>Failed tests</summary>")
            lines.append("")
            lines.append("```")
            lines.extend(_failed_test_lines(content))
            lines.append("```")
            lines.append("</details>")
        else:
            lines.append(f"**{arch}**: {passed} passed, {skipped} skipped")
        lines.append("")

    if has_failures:
        lines.append(f"**Total: {total_passed} passed, {total_failed} failed, {total_skipped} skipped**")
    else:
        lines.append(f"**Total: {total_passed} passed, {total_skipped} skipped**")
    lines.append("")
    return lines


def _render_coverage(base_dir: Path, env: Mapping[str, str]) -> list[str]:
    coverage_path = base_dir / _env(env, "COVERAGE_XML", "artifacts/coverage-report/coverage.xml")
    if not coverage_path.exists():
        return []

    lines = ["### Code Coverage", ""]
    current = _parse_coverage_percent(coverage_path)
    baseline_path = base_dir / _env(env, "BASELINE_COVERAGE_XML", "baseline-coverage.xml")
    baseline = _parse_coverage_percent(baseline_path)

    coverage_text = f"{current:.1f}%" if current is not None else "N/A"

    if baseline is not None and current is not None:
        diff = current - baseline
        lines.append("| Coverage | Baseline | Change |")
        lines.append("|----------|----------|--------|")
        lines.append(f"| {coverage_text} | {baseline:.1f}% | {diff:+.1f}% |")
    else:
        lines.append(f"Coverage: **{coverage_text}**")
        lines.append("")
        lines.append("*No baseline available for comparison*")

    lines.append("")
    return lines


def _format_delta_mb(delta_bytes: int) -> str:
    delta_mb = delta_bytes / 1024.0 / 1024.0
    if delta_bytes > 0:
        return f"+{delta_mb:.2f} MB (increase)"
    if delta_bytes < 0:
        return f"{delta_mb:.2f} MB (decrease)"
    return "No change"


def _render_artifact_sizes(base_dir: Path, env: Mapping[str, str]) -> list[str]:
    pr_sizes_path = base_dir / _env(env, "PR_SIZES_JSON", "pr-sizes.json")
    if not pr_sizes_path.exists():
        return []

    try:
        pr_data = json.loads(pr_sizes_path.read_text(encoding="utf-8"))
    except Exception:
        logger.debug("Failed to parse PR sizes from %s", pr_sizes_path, exc_info=True)
        return []

    baseline_path = base_dir / _env(env, "BASELINE_SIZES_JSON", "baseline-sizes.json")
    baseline: dict[str, dict] = {}
    if baseline_path.exists():
        try:
            baseline_data = json.loads(baseline_path.read_text(encoding="utf-8"))
            baseline = {a["name"]: a for a in baseline_data.get("artifacts", [])}
        except Exception:
            logger.debug("Failed to parse baseline sizes from %s", baseline_path, exc_info=True)
            baseline = {}

    lines = ["### Artifact Sizes", ""]
    if baseline:
        lines.append("| Artifact | Size | Δ from master |")
        lines.append("|----------|------|---------------|")
    else:
        lines.append("| Artifact | Size |")
        lines.append("|----------|------|")

    total_delta = 0
    for artifact in pr_data.get("artifacts", []):
        name = artifact["name"]
        size_human = artifact["size_human"]

        if baseline and name in baseline:
            old_size = int(baseline[name]["size_bytes"])
            new_size = int(artifact["size_bytes"])
            delta = new_size - old_size
            total_delta += delta
            lines.append(f"| {name} | {size_human} | {_format_delta_mb(delta)} |")
        else:
            lines.append(f"| {name} | {size_human} |")

    lines.append("")
    if baseline and total_delta != 0:
        direction = "increased" if total_delta > 0 else "decreased"
        lines.append(f"**Total size {direction} by {abs(total_delta) / 1024.0 / 1024.0:.2f} MB**")
    elif not baseline:
        lines.append("*No baseline available for comparison*")
    return lines


def generate_comment(env: Mapping[str, str], base_dir: Path, now_utc: datetime | None = None) -> str:
    table = _env(env, "BUILD_TABLE")
    summary = _env(env, "BUILD_SUMMARY", "Some builds still in progress.")
    precommit_status = _env(env, "PRECOMMIT_STATUS", "Not Triggered")
    precommit_url = _env(env, "PRECOMMIT_URL")
    triggered_by = _env(env, "TRIGGERED_BY", "Unknown")

    precommit_details = f"[View]({precommit_url})" if precommit_url else "-"
    precommit_note = ""

    precommit_path = base_dir / _env(env, "PRECOMMIT_RESULTS_PATH", "artifacts/pre-commit-results/pre-commit-results.json")
    parsed_status, parsed_details, parsed_note = _parse_precommit_results(precommit_path)
    if parsed_status:
        precommit_status = parsed_status
    if parsed_details:
        precommit_details = parsed_details
    if parsed_note:
        precommit_note = parsed_note

    lines: list[str] = [
        "## Build Results",
        "",
        "### Platform Status",
        "",
    ]

    if table:
        lines.extend(table.strip().splitlines())
    else:
        lines.extend(["| Platform | Status | Details |", "|----------|--------|--------|"])

    lines.extend(
        [
            f"**{summary}**",
            "",
            "### Pre-commit",
            "",
            "| Check | Status | Details |",
            "|-------|--------|---------|",
            f"| pre-commit | {precommit_status} | {precommit_details} |",
            "",
        ]
    )

    if precommit_note:
        lines.append(precommit_note)
        lines.append("")

    lines.extend(_render_test_results(base_dir, env))
    lines.extend(_render_coverage(base_dir, env))
    lines.extend(_render_artifact_sizes(base_dir, env))

    now = now_utc or datetime.now(UTC)
    lines.extend(
        [
            "",
            "---",
            f"<sub>Updated: {now.strftime('%Y-%m-%d %H:%M:%S UTC')} • Triggered by: {triggered_by}</sub>",
        ]
    )

    return "\n".join(lines).rstrip() + "\n"


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
