"""Tests for pages_render.py: inline SVG charts and the dashboard page."""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from datetime import datetime, timedelta, timezone

import pytest
from ci.publish import pages_render as mod

NOW = datetime(2026, 10, 23, 23, 0, tzinfo=timezone.utc)
REPO = "mavlink/qgroundcontrol"
SVG = "{http://www.w3.org/2000/svg}"
NASTY = 'Release <x86_64> & "arm64"'


def _column(index: int, *values: float | None) -> mod.Column:
    return mod.Column(
        label=f"10-{index:02d}",
        title=f"abc{index:04d} · 2026-10-{index:02d} · success",
        url=f"https://github.com/{REPO}/actions/runs/{index}",
        values=values,
    )


def _svg(chart: mod.Chart) -> ET.Element:
    return ET.fromstring(mod.svg_chart(chart, "c1"))


def _text(element: ET.Element, tag: str) -> str | None:
    child = element.find(f"{SVG}{tag}")
    assert child is not None
    return child.text


def test_chart_is_valid_svg_with_title_description_and_run_links() -> None:
    chart = mod.Chart(
        "Linux: wall time and Build steps",
        ("Wall time", NASTY),
        (_column(1, 600, 54), _column(2, None, 60), _column(3, 780, None)),
        "duration",
    )
    root = _svg(chart)
    assert root.get("role") == "img"
    assert root.get("aria-labelledby") == "c1-t c1-d"
    assert _text(root, "title") == "Linux: wall time and Build steps"
    assert _text(root, "desc") == f"3 runs. Latest: Wall time 13m 00s; {NASTY} 1m 00s."
    links = root.findall(f"{SVG}a")
    assert [link.get("href") for link in links] == [
        f"https://github.com/{REPO}/actions/runs/{index}" for index in (1, 2, 3)
    ]
    assert _text(links[1], "title") == f"abc0002 · 2026-10-02 · success\n{NASTY}: 1m 00s"
    # One line and one marker path per series with values.
    assert len(root.findall(f"{SVG}path")) == 4


def test_chart_escapes_labels() -> None:
    chart = mod.Chart(NASTY, (NASTY,), (_column(1, 5), _column(2, 6)), "count")
    text = mod.svg_chart(chart, "c1")
    assert NASTY not in text
    assert "Release &lt;x86_64&gt; &amp; &quot;arm64&quot;" in text
    assert _text(_svg(chart), "title") == NASTY


def test_chart_without_values_renders_nothing() -> None:
    chart = mod.Chart("Empty", ("a",), (_column(1, None),), "count")
    assert mod.svg_chart(chart, "c1") == ""
    assert mod.figure(chart, "c1") == ""


@pytest.mark.parametrize(
    ("values", "unit", "expected"),
    [
        ([0, 1], 1.0, [0.0, 1.0]),
        ([500, 4000], 60.0, [0.0, 1200.0, 2400.0, 3600.0, 4800.0]),
        ([182.5, 193.6], 0.0, [180.0, 185.0, 190.0, 195.0]),
        ([66.0, 66.4], 0.0, [66.0, 66.2, 66.4]),
        ([3, 3], 1.0, [2.0, 3.0, 4.0]),
    ],
)
def test_axis_ticks_are_round_and_span_the_values(
    values: list[float], unit: float, expected: list[float]
) -> None:
    ticks = mod.axis_ticks(values, unit)
    assert ticks == pytest.approx(expected)
    assert ticks[0] <= min(values) and ticks[-1] >= max(values)


def test_figure_tables_latest_values_and_change() -> None:
    chart = mod.Chart(
        "Linux: artifact sizes",
        ("QGroundControl-x86_64", "Gone"),
        (_column(1, 193.0, 10.0), _column(2, 193.5, None)),
        "size",
    )
    html = mod.figure(chart, "linux-1")
    assert "<caption>Linux: artifact sizes: latest values</caption>" in html
    assert '<td class="n">193.5 MB</td><td class="n">+512 KB</td>' in html
    # A series missing from the latest run shows its last value and run.
    assert '<td>Gone</td><td class="n">10.0 MB</td><td class="n"></td>' in html
    assert f'href="https://github.com/{REPO}/actions/runs/1">abc0001' in html


def _record(commit: str, days_ago: int, run_id: int, **fields: object) -> dict:
    created = NOW - timedelta(days=days_ago)
    return {
        "commit": commit,
        "workflow": "Linux",
        "run_id": run_id,
        "attempt": 1,
        "created_at": created.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "conclusion": "success",
        "wall_seconds": 700 + run_id,
        "build_seconds": {"Release linux_gcc_64": 54},
        "artifact_bytes": {"QGroundControl-x86_64": 202_952_797 + run_id},
        "tests": {
            "linux-coverage-unit": {"total": 388, "failed": 0, "skipped": 0, "seconds": 70.0}
        },
        "coverage": {"line_percent": 66.3, "line_covered": 65916, "line_total": 99364},
        **fields,
    }


def test_dashboard_renders_every_metric_as_valid_charts() -> None:
    failure = {"tests": {"linux-coverage-unit": {"total": 388, "failed": 1, "skipped": 0, "seconds": 71.0}},
               "failed_tests": [["linux-coverage-unit", "Vehicle<Test>"]]}  # fmt: skip
    records = [_record(f"{index:040x}", 10 - index, index) for index in range(1, 9)]
    records[3].update(failure)
    analysis = {
        "commit": records[-1]["commit"],
        "workflow": "Code Analysis",
        "run_id": 99,
        "created_at": records[-1]["created_at"],
        "conclusion": "success",
        "warnings": {"clang-tidy": 23622, "clazy": 1022},
    }
    history = {"version": 1, "repository": REPO, "records": [*records, analysis]}

    html = mod.dashboard(
        history, repo=REPO, workflows=["Linux", "Windows", "Code Analysis"], now=NOW, chart_runs=5
    )

    svgs = re.findall(r"<svg.*?</svg>", html, flags=re.DOTALL)
    titles = [_text(ET.fromstring(svg), "title") for svg in svgs]
    assert titles == [
        "Linux: wall time and Build steps",
        "Linux: artifact sizes",
        "Linux: tests per suite",
        "Linux: test time per suite",
        "Linux: failed tests per suite",
        "Linux: src/ line coverage",
        "Code Analysis: unique warnings per tool",
    ]
    # Charts show the latest five runs; the overview names each workflow's latest run.
    assert all(len(ET.fromstring(svg).findall(f"{SVG}a")) <= 5 for svg in svgs)
    assert 'href="#windows"' not in html
    assert f'href="https://github.com/{REPO}/actions/runs/99">run 99</a>' in html
    assert "<code>Vehicle&lt;Test&gt;</code>" in html
    assert '<td>linux-coverage-unit</td><td class="n">1 of 8</td>' in html
    assert 'href="data/history.json"' in html
    assert "<script" not in html and "http://" not in html.replace("http://www.w3.org", "")


def test_dashboard_of_an_empty_history_says_so() -> None:
    html = mod.dashboard({"version": 1, "records": []}, repo=REPO, workflows=["Linux"], now=NOW)
    assert "No master push builds have been recorded yet." in html
    assert "<svg" not in html


def test_pages_link_the_site_relative_to_their_depth() -> None:
    root = mod.page("CI health", "<p>x</p>", depth=0, description="d", generated=NOW)
    nested = mod.page("Downloads", "<p>x</p>", depth=1, description="d", generated=NOW)
    assert '<a href="./">Dashboard</a> <a href="downloads/">Downloads</a>' in root
    assert '<a href="../">Dashboard</a> <a href="../downloads/">Downloads</a>' in nested
    assert "<title>Downloads · QGroundControl</title>" in nested
    assert '<html lang="en">' in nested
