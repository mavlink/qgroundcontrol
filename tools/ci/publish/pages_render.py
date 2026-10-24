"""HTML pages and inline SVG charts for the GitHub Pages site.

Every page is one self-contained HTML document: inline CSS, inline SVG charts, no scripts and
no external assets. Charts plot one workflow's latest master runs; each run is a column that
links to the run and lists its values in a tooltip, and a table of the latest values follows
every chart for keyboard and screen-reader users.
"""

from __future__ import annotations

import html
import math
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from common.format import format_count, format_duration, format_percent
from common.gh_actions import parse_github_timestamp

from ci.publish.pages_history import failing_tests, run_url

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Sequence
    from datetime import datetime

SITE_TITLE = "QGroundControl"
CHART_RUNS = 60
FAILING_TESTS_DAYS = 30
MAX_FAILING_TESTS = 50
# Okabe-Ito colors, distinguishable with color-vision deficiencies; later series are dashed.
PALETTE = ("#0072B2", "#E69F00", "#009E73", "#D55E00", "#CC79A7", "#56B4E9", "#7F7F7F", "#000000")
NAV = (("Dashboard", ""), ("Downloads", "downloads/"), ("Schemas", "schemas/"), ("API", "api/"))

_WIDTH, _HEIGHT = 720, 230
_LEFT, _RIGHT, _TOP, _BOTTOM = 76, 10, 10, 26
_MAX_X_LABELS = 6

CSS = """
body{font:15px/1.5 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;margin:0;color:#1f2328}
header,main,footer{max-width:1040px;margin:0 auto;padding:0 16px}
header{border-bottom:1px solid #d0d7de}
nav a{margin-right:1em}
h1{font-size:1.6em;margin:.4em 0}h2{margin-top:2em;border-bottom:1px solid #d0d7de}
a{color:#0969da}
table{border-collapse:collapse;margin:.5em 0 1.5em;font-size:.9em}
th,td{border:1px solid #d0d7de;padding:3px 8px;text-align:left;vertical-align:top}
td.n{text-align:right;font-variant-numeric:tabular-nums}
caption{text-align:left;font-weight:600;padding:4px 0}
code,pre{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:.88em}
pre{background:#f6f8fa;padding:8px 12px;overflow-x:auto}
figure{margin:1em 0}figcaption{font-weight:600}
svg.chart{width:100%;max-width:720px;height:auto;font-size:11px}
svg.chart .grid{stroke:#d0d7de}svg.chart text{fill:#57606a}
svg.chart .col rect{fill:transparent}svg.chart .col:hover rect{fill:rgba(9,105,218,.08)}
ul.legend{list-style:none;padding:0;margin:.2em 0;font-size:.9em}
ul.legend li{display:inline-block;margin-right:1.2em}
.sw{display:inline-block;width:14px;height:3px;margin-right:4px;vertical-align:middle}
.muted{color:#57606a}
"""


def esc(value: object) -> str:
    return html.escape(str(value), quote=True)


def link(text: str, url: str) -> str:
    return f'<a href="{esc(url)}">{esc(text)}</a>'


def page(title: str, body: str, *, depth: int, description: str, generated: datetime) -> str:
    """Return a complete HTML document; *depth* is the page's directory depth below the root."""
    root = "../" * depth
    nav = " ".join(f'<a href="{root + target or "./"}">{esc(name)}</a>' for name, target in NAV)
    stamp = generated.strftime("%Y-%m-%d %H:%M UTC")
    return (
        "<!DOCTYPE html>\n"
        '<html lang="en"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width,initial-scale=1">'
        f'<meta name="description" content="{esc(description)}">'
        f"<title>{esc(title)} · {SITE_TITLE}</title><style>{CSS}</style></head>\n"
        f'<body><header><nav aria-label="Site">{nav}'
        '<a href="https://github.com/mavlink/qgroundcontrol">GitHub</a></nav>'
        f"<h1>{esc(title)}</h1></header>\n<main>\n{body}\n</main>\n"
        f'<footer><p class="muted">Generated {esc(stamp)} by '
        '<a href="https://github.com/mavlink/qgroundcontrol/blob/master/.github/workflows/pages.yml">'
        "the Pages workflow</a>.</p></footer></body></html>\n"
    )


def table(
    headers: Sequence[str], rows: Iterable[Sequence[str]], *, caption: str, numeric: set[int]
) -> str:
    """Return an HTML table; *rows* hold already-escaped HTML cells."""
    head = "".join(f'<th scope="col">{esc(h)}</th>' for h in headers)
    body = "".join(
        "<tr>"
        + "".join(
            f'<td class="n">{cell}</td>' if index in numeric else f"<td>{cell}</td>"
            for index, cell in enumerate(row)
        )
        + "</tr>\n"
        for row in rows
    )
    return f"<table><caption>{esc(caption)}</caption><tr>{head}</tr>\n{body}</table>"


@dataclass(frozen=True)
class Column:
    """One run on a chart's x axis, with one value (or None) per series."""

    label: str
    title: str
    url: str
    values: tuple[float | None, ...]


def _mib(value: float) -> str:
    return f"{value:,.1f} MB" if abs(value) >= 1 else f"{value * 1024:,.0f} KB"


def _count(value: float) -> str:
    return format_count(round(value))


# Value formatting per chart kind: sizes are charted in MiB, durations in seconds.
FORMATTERS: dict[str, Callable[[float], str]] = {
    "count": _count,
    "duration": format_duration,
    "percent": format_percent,
    "size": _mib,
}


@dataclass(frozen=True)
class Chart:
    title: str
    series: tuple[str, ...]
    columns: tuple[Column, ...]
    kind: str

    def formatter(self, value: float) -> str:
        return FORMATTERS[self.kind](value)

    def tick_unit(self, span: float) -> float:
        """The y-axis step is a whole number of these: tests, or minutes over longer spans."""
        if self.kind == "count":
            return 1.0
        if self.kind == "duration":
            return 60.0 if span >= 240 else 1.0
        return 0.0


def _nice_step(span: float, ticks: int = 4) -> float:
    raw = span / ticks
    magnitude = 10 ** math.floor(math.log10(raw))
    return next(m * magnitude for m in (1, 2, 2.5, 5, 10) if raw <= m * magnitude)


def axis_ticks(values: Sequence[float], unit: float = 0.0) -> list[float]:
    """Return evenly spaced, rounded y-axis ticks spanning *values*, from zero when close to it.

    With a *unit*, the step is a whole multiple of it.
    """
    low, high = min(values), max(values)
    if low == high:
        pad = abs(low) * 0.05 or 1.0
        low, high = low - pad, high + pad
    if low >= 0 and low < (high - low) / 2:
        low = 0.0
    if unit:
        step = max(1.0, _nice_step((high - low) / unit)) * unit
    else:
        step = _nice_step(high - low)
    start = math.floor(low / step) * step
    count = math.ceil(round((high - start) / step, 9))
    return [start + index * step for index in range(count + 1)]


def _xy(value: float, ticks: list[float]) -> float:
    plot = _HEIGHT - _TOP - _BOTTOM
    return _TOP + plot * (1 - (value - ticks[0]) / (ticks[-1] - ticks[0]))


def _num(value: float) -> str:
    """SVG coordinates in whole viewBox units: a pixel at full size, and a smaller page."""
    return str(round(value))


def svg_chart(chart: Chart, chart_id: str) -> str:
    """Return the chart as inline SVG, or '' when it has no values."""
    values = [v for column in chart.columns for v in column.values if v is not None]
    if not values:
        return ""
    ticks = axis_ticks(values, chart.tick_unit(max(values) - min(values)))
    step = (_WIDTH - _LEFT - _RIGHT) / len(chart.columns)
    xs = [_LEFT + (index + 0.5) * step for index in range(len(chart.columns))]
    latest = "; ".join(
        f"{name} {chart.formatter(value)}"
        for index, name in enumerate(chart.series)
        if (value := _latest(chart, index)) is not None
    )
    parts = [
        f'<svg class="chart" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {_WIDTH} {_HEIGHT}" '
        f'role="img" aria-labelledby="{chart_id}-t {chart_id}-d">',
        f'<title id="{chart_id}-t">{esc(chart.title)}</title>',
        f'<desc id="{chart_id}-d">{esc(f"{len(chart.columns)} runs. Latest: {latest}.")}</desc>',
    ]
    for tick in ticks:
        y = _num(_xy(tick, ticks))
        parts.append(
            f'<line class="grid" x1="{_LEFT}" x2="{_WIDTH - _RIGHT}" y1="{y}" y2="{y}"/>'
            f'<text x="{_LEFT - 6}" y="{y}" text-anchor="end" dominant-baseline="middle">'
            f"{esc(chart.formatter(tick))}</text>"
        )
    every = max(1, math.ceil(len(chart.columns) / _MAX_X_LABELS))
    for index in range(0, len(chart.columns), every):
        parts.append(
            f'<text x="{_num(xs[index])}" y="{_HEIGHT - 8}" text-anchor="middle">'
            f"{esc(chart.columns[index].label)}</text>"
        )
    for index, _name in enumerate(chart.series):
        points = [
            (xs[column_index], _xy(value, ticks))
            for column_index, column in enumerate(chart.columns)
            if (value := column.values[index]) is not None
        ]
        if not points:
            continue
        color = PALETTE[index % len(PALETTE)]
        dash = ' stroke-dasharray="5 3"' if index >= len(PALETTE) else ""
        line = "".join(f"{'L' if i else 'M'}{_num(x)} {_num(y)}" for i, (x, y) in enumerate(points))
        dots = "".join(f"M{_num(x)} {_num(y)}h0" for x, y in points)
        parts.append(
            f'<path d="{line}" fill="none" stroke="{color}" stroke-width="1.5"{dash}/>'
            f'<path d="{dots}" stroke="{color}" stroke-width="4" stroke-linecap="round"/>'
        )
    for column, x in zip(chart.columns, xs, strict=True):
        lines = [column.title] + [
            f"{name}: {chart.formatter(value)}"
            for name, value in zip(chart.series, column.values, strict=True)
            if value is not None
        ]
        parts.append(
            f'<a class="col" href="{esc(column.url)}" tabindex="-1">'
            f'<rect x="{_num(x - step / 2)}" y="{_TOP}" width="{_num(step)}" '
            f'height="{_HEIGHT - _TOP - _BOTTOM}"/><title>{esc(chr(10).join(lines))}</title></a>'
        )
    parts.append("</svg>")
    return "".join(parts)


def _latest(chart: Chart, index: int, skip: int = 0) -> float | None:
    found = [c.values[index] for c in chart.columns if c.values[index] is not None]
    return found[-1 - skip] if len(found) > skip else None


def _latest_column(chart: Chart, index: int) -> Column | None:
    return next((c for c in reversed(chart.columns) if c.values[index] is not None), None)


def legend(chart: Chart) -> str:
    items = []
    for index, name in enumerate(chart.series):
        color = PALETTE[index % len(PALETTE)]
        dashed = " (dashed)" if index >= len(PALETTE) else ""
        items.append(
            f'<li><span class="sw" style="background:{color}"></span>{esc(name)}{dashed}</li>'
        )
    return f'<ul class="legend">{"".join(items)}</ul>'


def figure(chart: Chart, chart_id: str) -> str:
    """Return a chart with its legend and a table of each series' latest value and change."""
    svg = svg_chart(chart, chart_id)
    if not svg:
        return ""
    rows = []
    for index, name in enumerate(chart.series):
        column = _latest_column(chart, index)
        latest, previous = _latest(chart, index), _latest(chart, index, 1)
        if column is None or latest is None:
            continue
        change = ""
        if previous is not None:
            delta = latest - previous
            sign = "+" if delta > 0 else "-"
            change = "±0" if delta == 0 else f"{sign}{chart.formatter(abs(delta))}"
        rows.append(
            [esc(name), esc(chart.formatter(latest)), esc(change), link(column.title, column.url)]
        )
    caption = f"{chart.title}: latest values"
    latest_table = table(
        ["Series", "Latest", "Change", "Run"], rows, caption=caption, numeric={1, 2}
    )
    return (
        f'<figure><figcaption id="{chart_id}">{esc(chart.title)}</figcaption>'
        f"{svg}{legend(chart)}</figure>\n{latest_table}\n"
    )


def _date(timestamp: str) -> str:
    parsed = parse_github_timestamp(timestamp)
    return parsed.strftime("%Y-%m-%d") if parsed else ""


def _columns(
    repo: str, records: Sequence[dict[str, Any]], values: Callable[[dict[str, Any]], tuple]
) -> tuple[Column, ...]:
    return tuple(
        Column(
            label=_date(record["created_at"])[5:],
            title=f"{record['commit'][:7]} · {_date(record['created_at'])} · "
            f"{record.get('conclusion') or 'unknown'}",
            url=run_url(repo, int(record["run_id"])),
            values=values(record),
        )
        for record in records
    )


def _keys(records: Iterable[dict[str, Any]], field: str) -> tuple[str, ...]:
    return tuple(sorted({key for record in records for key in record.get(field) or {}}))


def workflow_charts(repo: str, workflow: str, records: Sequence[dict[str, Any]]) -> list[Chart]:
    """Return a workflow's charts over *records* (oldest first); charts without data are left out."""
    charts = []
    jobs = _keys(records, "build_seconds")
    charts.append(
        Chart(
            f"{workflow}: wall time and Build steps",
            ("Wall time", *(f"{job} (Build)" for job in jobs)),
            _columns(
                repo,
                records,
                lambda r: (
                    r.get("wall_seconds"),
                    *((r.get("build_seconds") or {}).get(job) for job in jobs),
                ),
            ),
            "duration",
        )
    )
    if packages := _keys(records, "artifact_bytes"):
        charts.append(
            Chart(
                f"{workflow}: artifact sizes",
                packages,
                _columns(
                    repo,
                    records,
                    lambda r: tuple(
                        size / 1048576
                        if (size := (r.get("artifact_bytes") or {}).get(name))
                        else None
                        for name in packages
                    ),
                ),
                "size",
            )
        )
    if suites := _keys(records, "tests"):

        def suite_values(field: str) -> Callable[[dict[str, Any]], tuple]:
            return lambda r: tuple(
                ((r.get("tests") or {}).get(suite) or {}).get(field) for suite in suites
            )

        charts.append(
            Chart(
                f"{workflow}: tests per suite",
                suites,
                _columns(repo, records, suite_values("total")),
                "count",
            )
        )
        charts.append(
            Chart(
                f"{workflow}: test time per suite",
                suites,
                _columns(repo, records, suite_values("seconds")),
                "duration",
            )
        )
        failed = _columns(repo, records, suite_values("failed"))
        if any(value for column in failed for value in column.values):
            charts.append(Chart(f"{workflow}: failed tests per suite", suites, failed, "count"))
    if any(record.get("coverage") for record in records):
        charts.append(
            Chart(
                f"{workflow}: src/ line coverage",
                ("Line coverage",),
                _columns(repo, records, lambda r: ((r.get("coverage") or {}).get("line_percent"),)),
                "percent",
            )
        )
    if tools := _keys(records, "warnings"):
        charts.append(
            Chart(
                f"{workflow}: unique warnings per tool",
                tools,
                _columns(
                    repo,
                    records,
                    lambda r: tuple((r.get("warnings") or {}).get(tool) for tool in tools),
                ),
                "count",
            )
        )
    return charts


def _slug(text: str) -> str:
    return "".join(ch if ch.isalnum() else "-" for ch in text.lower()).strip("-")


def latest_builds_table(repo: str, by_workflow: dict[str, list[dict[str, Any]]]) -> str:
    rows = []
    for workflow, records in by_workflow.items():
        record = records[-1]
        rows.append(
            [
                f'<a href="#{_slug(workflow)}">{esc(workflow)}</a>',
                link(record["commit"][:7], f"https://github.com/{repo}/commit/{record['commit']}"),
                esc(_date(record["created_at"])),
                esc(record.get("conclusion") or "unknown"),
                esc(format_duration(record.get("wall_seconds"))),
                link(f"run {record['run_id']}", run_url(repo, int(record["run_id"]))),
            ]
        )
    return table(
        ["Workflow", "Commit", "Date", "Result", "Wall time", "Run"],
        rows,
        caption="Latest master push builds",
        numeric={4},
    )


def failing_tests_table(repo: str, rows: list[dict[str, Any]], days: int) -> str:
    if not rows:
        return f"<p>No test failed in master push builds in the last {days} days.</p>"
    cells = [
        [
            f"<code>{esc(row['name'])}</code>",
            esc(row["suite"]),
            esc(f"{row['failures']} of {row['runs']}"),
            esc(_date(row["last_seen"])),
            link(row["commit"][:7], f"https://github.com/{repo}/commit/{row['commit']}")
            + " · "
            + link(f"{row['workflow']} run", run_url(repo, int(row["run_id"]))),
        ]
        for row in rows[:MAX_FAILING_TESTS]
    ]
    more = (
        f'<p class="muted">…and {len(rows) - MAX_FAILING_TESTS} more.</p>'
        if len(rows) > MAX_FAILING_TESTS
        else ""
    )
    return (
        table(
            ["Test", "Suite", "Failed runs", "Last failed", "Last failing run"],
            cells,
            caption=f"Tests that failed in master push builds in the last {days} days",
            numeric={2},
        )
        + more
    )


def dashboard(
    history: dict[str, Any],
    *,
    repo: str,
    workflows: Sequence[str],
    now: datetime,
    chart_runs: int = CHART_RUNS,
    failing_days: int = FAILING_TESTS_DAYS,
) -> str:
    """Return the CI health dashboard page for *history*."""
    records = sorted(history["records"], key=lambda r: str(r["created_at"]))
    by_workflow = {
        workflow: found
        for workflow in workflows
        if (found := [r for r in records if r["workflow"] == workflow])
    }
    commits = {r["commit"] for r in records}
    intro = (
        f"<p>Health of <code>master</code> push builds of {link(repo, f'https://github.com/{repo}')}: "
        "artifact sizes, tests, <code>src/</code> line coverage, Code Analysis warnings, and build "
        f"times. The history holds {format_count(len(commits))} commits"
        + (f" since {esc(_date(records[0]['created_at']))}" if records else "")
        + f"; charts show each workflow's latest {chart_runs} runs, and each column links to its "
        'run. Raw data: <a href="data/history.json">data/history.json</a>.</p>'
    )
    if not records:
        body = intro + "<p>No master push builds have been recorded yet.</p>"
        return page(
            "CI health", body, depth=0, description="QGroundControl CI health", generated=now
        )
    toc = " · ".join(f'<a href="#{_slug(w)}">{esc(w)}</a>' for w in by_workflow)
    sections = [
        intro,
        f'<nav aria-label="Workflows"><p>{toc} · <a href="#failing-tests">Failing tests</a></p></nav>',
        latest_builds_table(repo, by_workflow),
    ]
    for workflow, found in by_workflow.items():
        sections.append(f'<h2 id="{_slug(workflow)}">{esc(workflow)}</h2>')
        for index, chart in enumerate(workflow_charts(repo, workflow, found[-chart_runs:])):
            sections.append(figure(chart, f"{_slug(workflow)}-{index}"))
    sections.append('<h2 id="failing-tests">Recently failing tests</h2>')
    sections.append(
        failing_tests_table(repo, failing_tests(records, now=now, days=failing_days), failing_days)
    )
    return page(
        "CI health",
        "\n".join(sections),
        depth=0,
        description="Artifact sizes, tests, coverage, warnings and build times of QGroundControl master builds",
        generated=now,
    )
