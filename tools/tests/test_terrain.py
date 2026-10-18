"""Tests for terrain tile math, Copernicus cell lookup and upload policy."""

from __future__ import annotations

import functools
import io
import os
import subprocess
import sys
import urllib.error
from concurrent.futures import Future
from pathlib import Path

import pytest
from terrain import cli, copernicus, fetch, jobs, metadata, tilemath, upload

TileKey = tilemath.TileKey


def test_mercator_bounds_cover_world_at_zoom_zero() -> None:
    half = tilemath.MERCATOR_HALF_EXTENT
    assert tilemath.mercator_bounds(TileKey(0, 0, 0)) == pytest.approx((-half, -half, half, half))


def test_lonlat_bounds_put_y_zero_at_the_north_edge() -> None:
    west, south, east, north = tilemath.lonlat_bounds(TileKey(1, 0, 0))
    assert (west, east) == pytest.approx((-180.0, 0.0))
    assert south == pytest.approx(0.0, abs=1e-9)
    assert north == pytest.approx(tilemath.MAX_LATITUDE)


@pytest.mark.parametrize(
    ("lon", "lat"), [(7.65, 45.97), (-122.5, 37.75), (-70.0, -32.65), (0.001, -0.001)]
)
def test_tile_for_lonlat_contains_the_point(lon: float, lat: float) -> None:
    key = tilemath.tile_for_lonlat(lon, lat, tilemath.MAX_ZOOM)
    west, south, east, north = tilemath.lonlat_bounds(key)
    assert west <= lon < east
    assert south <= lat < north


def test_tile_for_lonlat_clamps_to_the_grid() -> None:
    n = 1 << 4
    assert tilemath.tile_for_lonlat(180.0, -90.0, 4) == TileKey(4, n - 1, n - 1)
    assert tilemath.tile_for_lonlat(-180.0, 90.0, 4) == TileKey(4, 0, 0)


def test_mercator_lonlat_round_trip() -> None:
    mx, my = tilemath.lonlat_to_mercator(86.925, 27.988)
    assert tilemath.mercator_to_lonlat(mx, my) == pytest.approx((86.925, 27.988))


def test_tiles_in_bbox_spans_rows_and_columns() -> None:
    tiles = tilemath.tiles_in_bbox(-1.0, -1.0, 1.0, 1.0, 1)
    assert tiles == [TileKey(1, 0, 0), TileKey(1, 1, 0), TileKey(1, 0, 1), TileKey(1, 1, 1)]
    with pytest.raises(ValueError):
        tilemath.tiles_in_bbox(1.0, 0.0, -1.0, 1.0, 1)


def test_parent_and_children_are_inverse() -> None:
    key = TileKey(8, 133, 91)
    assert [tilemath.parent(child) for child in tilemath.children(key)] == [key] * 4
    assert tilemath.children(key)[0] == TileKey(9, 266, 182)
    with pytest.raises(ValueError):
        tilemath.parent(TileKey(0, 0, 0))


def test_all_tiles_count() -> None:
    assert len(tilemath.all_tiles(3)) == 64


@pytest.mark.parametrize(
    ("lat", "lon", "name"),
    [
        (46, 7, "Copernicus_DSM_COG_10_N46_00_E007_00_DEM"),
        (-33, -70, "Copernicus_DSM_COG_10_S33_00_W070_00_DEM"),
        (0, -1, "Copernicus_DSM_COG_10_N00_00_W001_00_DEM"),
    ],
)
def test_cell_name_uses_south_west_corner_and_round_trips(lat: int, lon: int, name: str) -> None:
    assert copernicus.cell_name(lat, lon) == name
    assert copernicus.parse_cell_name(name) == (lat, lon)


def test_glo90_cells_use_their_own_name_and_bucket() -> None:
    name = copernicus.cell_name(40, 44, copernicus.GLO90)
    assert name == "Copernicus_DSM_COG_30_N40_00_E044_00_DEM"
    assert copernicus.parse_cell_name(name) == (40, 44)
    assert copernicus.cell_url(name, copernicus.GLO90).startswith(copernicus.GLO90.base_url)
    with pytest.raises(ValueError):
        copernicus.parse_cell_name("N40E044")


def test_cells_for_bounds_includes_buffered_neighbors_that_exist() -> None:
    cells = {(46, 7): "n46e7", (45, 7): "n45e7", (46, 8): "n46e8"}
    assert copernicus.cells_for_bounds(7.2, 46.005, 7.8, 46.5, cells, buffer_deg=0.02) == {
        (45, 7): "n45e7",
        (46, 7): "n46e7",
    }


@pytest.mark.parametrize(
    ("bounds", "expected"),
    [
        # A job ending at 180 degrees east reads the cell west of 180 degrees west as lon 180.
        ((179.3, 0.1, 180.0, 0.7), {(0, 179): "e179", (0, 180): "w180"}),
        # A job starting at 180 degrees west reads the cell east of it as lon -181.
        ((-180.0, 0.1, -179.3, 0.7), {(0, -181): "e179", (0, -180): "w180"}),
    ],
)
def test_cells_for_bounds_wraps_across_the_antimeridian(
    bounds: tuple[float, float, float, float], expected: dict[tuple[int, int], str]
) -> None:
    cells = {(0, 179): "e179", (0, -180): "w180"}
    assert copernicus.cells_for_bounds(*bounds, cells, buffer_deg=0.02) == expected


def test_cells_for_bounds_clamps_latitude_to_the_globe() -> None:
    cells = {(-90, 0): "south", (89, 0): "north"}
    assert copernicus.cells_for_bounds(0.2, -90.0, 0.8, 90.0, cells, buffer_deg=0.02) == cells


@pytest.mark.parametrize(("lon", "wrapped"), [(179, 179), (180, -180), (-181, 179), (-180, -180)])
def test_wrap_lon_maps_into_the_globe(lon: int, wrapped: int) -> None:
    assert copernicus.wrap_lon(lon) == wrapped


def test_load_cells_prefers_glo30_and_fills_gaps_from_glo90(tmp_path: Path) -> None:
    glo30 = [copernicus.cell_name(46, 7)]
    glo90 = [
        copernicus.cell_name(46, 7, copernicus.GLO90),
        copernicus.cell_name(40, 44, copernicus.GLO90),
    ]
    (tmp_path / "tileList_10.txt").write_text("\n".join(glo30) + "\n", encoding="utf-8")
    (tmp_path / "tileList_30.txt").write_text("\n".join(glo90) + "\n", encoding="utf-8")
    cells = copernicus.load_cells(tmp_path)
    assert cells == {
        (46, 7): copernicus.cell_url(glo30[0]),
        (40, 44): copernicus.cell_url(glo90[1], copernicus.GLO90),
    }


@pytest.mark.parametrize(
    ("prefix", "root"),
    [(None, "s3://qgc-terrain-assets"), ("validation", "s3://qgc-terrain-assets/validation")],
)
def test_upload_targets_bucket_root_for_production_and_prefix_for_tests(
    prefix: str | None, root: str
) -> None:
    sync = upload.sync_command(Path("out"), prefix, mirror=False, profile=None, dryrun=False)
    assert sync[3:5] == [str(Path("out") / "terrarium"), f"{root}/terrarium"]
    invalidation = upload.invalidation_command(prefix, profile=None)
    path = invalidation[invalidation.index("--paths") + 1]
    assert f"s3://qgc-terrain-assets{path}" == f"{root}/terrarium/*"
    files = [metadata.MetadataFile(Path("out/tilejson.json"), "application/json")]
    assert upload.metadata_commands(files, prefix, profile=None, dryrun=False)[0][4] == (
        f"{root}/tilejson.json"
    )


def test_sync_command_uploads_pngs_without_acl_and_with_tile_caching() -> None:
    command = upload.sync_command(
        Path("out"), None, mirror=False, profile="qgc-terrain", dryrun=True
    )
    assert "--acl" not in command
    assert command[command.index("--content-type") + 1] == "image/png"
    assert command[command.index("--cache-control") + 1] == upload.TILE_CACHE_CONTROL
    assert "immutable" not in upload.TILE_CACHE_CONTROL
    assert command[command.index("--profile") + 1] == "qgc-terrain"
    assert command[-1] == "--dryrun"


def test_only_mirrored_test_prefixes_delete_remote_tiles() -> None:
    def sync(prefix: str | None, mirror: bool) -> list[str]:
        return upload.sync_command(Path("out"), prefix, mirror=mirror, profile=None, dryrun=False)

    assert "--delete" in sync("validation", mirror=True)
    assert "--delete" not in sync("validation", mirror=False)
    assert "--delete" not in sync(None, mirror=False)
    with pytest.raises(ValueError, match="--mirror"):
        sync(None, mirror=True)


def test_metadata_commands_use_short_caching_and_content_types() -> None:
    files = [
        metadata.MetadataFile(Path("out/tilejson.json"), "application/json"),
        metadata.MetadataFile(Path("out/ATTRIBUTION.txt"), "text/plain; charset=utf-8"),
    ]
    commands = upload.metadata_commands(files, None, profile="qgc-terrain", dryrun=True)
    for command, file in zip(commands, files, strict=True):
        assert command[command.index("--content-type") + 1] == file.content_type
        assert command[command.index("--cache-control") + 1] == upload.METADATA_CACHE_CONTROL
        assert "--acl" not in command
        assert command[-1] == "--dryrun"


@pytest.mark.parametrize("prefix", ["", "v1/terrarium", "../x", "V1", "-rf", "terrarium"])
def test_upload_rejects_unsafe_prefixes(prefix: str) -> None:
    with pytest.raises(ValueError):
        upload.sync_command(Path("out"), prefix, mirror=False, profile=None, dryrun=False)


@pytest.mark.parametrize("version", ["1.0.0", "2.13.0"])
def test_data_version_accepts_semver(version: str) -> None:
    upload.validate_data_version(version)


@pytest.mark.parametrize("version", ["", "1", "1.0", "v1.0.0", "1.0.0-rc1"])
def test_data_version_rejects_non_semver(version: str) -> None:
    with pytest.raises(ValueError):
        upload.validate_data_version(version)


def test_production_uploads_must_explain_the_update_to_users(tmp_path: Path) -> None:
    (tmp_path / "terrarium").mkdir()
    with pytest.raises(ValueError, match="notice"):
        upload.upload(
            tmp_path,
            None,
            data_version="1.0.0",
            notice=None,
            mirror=False,
            profile=None,
            dryrun=True,
        )


@pytest.mark.parametrize(
    ("severity", "text"), [("urgent", "Coastal heights fixed"), ("info", ""), ("critical", "  ")]
)
def test_notice_needs_known_severity_and_text(severity: str, text: str) -> None:
    with pytest.raises(ValueError):
        metadata.Notice(severity, text).validate()


@pytest.mark.parametrize(
    ("invalidation_returncode", "expected"),
    [
        # tilejson.json last: the new version appears only once everything else is in place.
        (0, ["sync", "create-invalidation", "wait", "ATTRIBUTION.txt", "tilejson.json"]),
        # A failed invalidation must not publish the new data version.
        (255, ["sync", "create-invalidation"]),
    ],
)
def test_upload_publishes_metadata_only_after_invalidation_completes(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    invalidation_returncode: int,
    expected: list[str],
) -> None:
    (tmp_path / "terrarium").mkdir()
    files = [
        metadata.MetadataFile(tmp_path / "ATTRIBUTION.txt", "text/plain; charset=utf-8"),
        metadata.MetadataFile(tmp_path / "tilejson.json", "application/json"),
    ]
    monkeypatch.setattr(upload, "write_metadata", lambda *_args: files)
    monkeypatch.setattr(upload, "check_complete_build", lambda _out: None)
    monkeypatch.setattr(upload, "published_version", lambda **_kwargs: None)
    steps: list[str] = []

    def run(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess[str]:
        steps.append(command[2] if command[2] != "cp" else Path(command[4]).name)
        returncode = invalidation_returncode if command[2] == "create-invalidation" else 0
        return subprocess.CompletedProcess(command, returncode, stdout="I123\n", stderr="")

    monkeypatch.setattr(upload.subprocess, "run", run)
    returncode = upload.upload(
        tmp_path,
        None,
        data_version="1.0.0",
        notice=metadata.Notice("info", "Initial terrain data"),
        mirror=False,
        profile=None,
        dryrun=False,
    )
    assert steps == expected
    assert returncode == invalidation_returncode


def job_key(x: int, y: int) -> TileKey:
    return TileKey(tilemath.JOB_ZOOM, x, y)


def test_completed_jobs_reads_marker_statuses(tmp_path: Path) -> None:
    jobs.mark_done(tmp_path, job_key(1, 2), "4 cells, 0 void px filled")
    jobs.mark_done(tmp_path, job_key(3, 4), jobs.OCEAN_STATUS)
    assert jobs.completed_jobs(tmp_path) == {
        job_key(1, 2): "4 cells, 0 void px filled",
        job_key(3, 4): jobs.OCEAN_STATUS,
    }
    assert jobs.completed_jobs(tmp_path / "missing") == {}


def test_parse_job_accepts_only_job_zoom_tiles() -> None:
    z = tilemath.JOB_ZOOM
    assert jobs.parse_job(f"{z}/267/182") == job_key(267, 182)
    for text in ("267/182", f"{z}/a/1", f"{z - 1}/1/1", f"{z}/{1 << z}/0", f"{z}/0/-1"):
        with pytest.raises(ValueError):
            jobs.parse_job(text)


def test_select_jobs_defaults_to_every_completed_job() -> None:
    completed = {job_key(2, 0): "1 cells", job_key(1, 0): jobs.OCEAN_STATUS}
    assert jobs.select_jobs(completed) == [job_key(1, 0), job_key(2, 0)]


def test_select_jobs_explicit_must_be_completed() -> None:
    completed = {job_key(1, 0): "1 cells", job_key(2, 0): "1 cells"}
    assert jobs.select_jobs(completed, explicit=[job_key(2, 0), job_key(2, 0)]) == [job_key(2, 0)]
    with pytest.raises(ValueError, match="Not completed"):
        jobs.select_jobs(completed, explicit=[job_key(9, 9)])


def test_select_jobs_samples_land_jobs_reproducibly() -> None:
    completed = {job_key(x, 0): "2 cells" for x in range(50)}
    completed.update({job_key(x, 1): jobs.OCEAN_STATUS for x in range(50)})
    sample = jobs.select_jobs(completed, sample=10, seed=7)
    assert len(sample) == 10
    assert sample == sorted(sample)
    assert all(completed[job] != jobs.OCEAN_STATUS for job in sample)
    assert jobs.select_jobs(completed, sample=10, seed=7) == sample
    assert jobs.select_jobs(completed, sample=10, seed=8) != sample
    assert len(jobs.select_jobs(completed, sample=500)) == 50
    with pytest.raises(ValueError):
        jobs.select_jobs(completed, sample=0)


def test_bounded_completion_keeps_in_flight_work_bounded() -> None:
    in_flight = 0
    peak = 0

    def submit(item: int) -> Future[int]:
        nonlocal in_flight, peak
        in_flight += 1
        peak = max(peak, in_flight)
        future: Future[int] = Future()
        if item == 3:
            future.set_exception(RuntimeError("boom"))
        else:
            future.set_result(item * 10)
        return future

    yielded = []
    for item, future in jobs.bounded_completion(submit, iter(range(10)), limit=3):
        in_flight -= 1
        yielded.append((item, None if future.exception() else future.result()))

    assert peak == 3
    assert sorted(yielded) == [(i, None if i == 3 else i * 10) for i in range(10)]
    with pytest.raises(ValueError):
        list(jobs.bounded_completion(submit, [1], limit=0))


def write_build(
    out: Path,
    jobs_done: int,
    *,
    overviews: bool,
    overviews_first: bool = False,
    checked: bool = True,
    checked_first: bool = False,
) -> None:
    root_tile = out / "terrarium" / "0" / "0" / "0.png"
    if overviews:
        root_tile.parent.mkdir(parents=True)
        root_tile.write_bytes(b"png")
    for x in range(jobs_done):
        jobs.mark_done(out, job_key(x, 0), jobs.OCEAN_STATUS)
    if not overviews:
        return
    newest = max(marker.stat().st_mtime for marker in jobs.done_markers(out))
    root_time = 0 if overviews_first else newest + 1
    os.utime(root_tile, (root_time, root_time))
    if checked:
        passed = jobs.check_passed_marker(out)
        passed.parent.mkdir(parents=True, exist_ok=True)
        passed.write_text("passed\n", encoding="utf-8")
        passed_time = root_time - 1 if checked_first else root_time + 1
        os.utime(passed, (passed_time, passed_time))


@pytest.mark.parametrize(
    ("jobs_done", "overviews", "overviews_first", "checked", "checked_first", "error"),
    [
        (2, True, False, True, False, None),
        (1, True, False, True, False, "1 of 2"),
        (2, False, False, True, False, "overviews"),
        (2, True, True, True, False, "older"),
        (2, True, False, False, False, "Run check"),
        (2, True, False, True, True, "rerun check"),
    ],
)
def test_production_uploads_need_a_complete_checked_build(
    tmp_path: Path,
    jobs_done: int,
    overviews: bool,
    overviews_first: bool,
    checked: bool,
    checked_first: bool,
    error: str | None,
) -> None:
    write_build(
        tmp_path,
        jobs_done,
        overviews=overviews,
        overviews_first=overviews_first,
        checked=checked,
        checked_first=checked_first,
    )
    if error is None:
        upload.check_complete_build(tmp_path, expected_jobs=2)
    else:
        with pytest.raises(ValueError, match=error):
            upload.check_complete_build(tmp_path, expected_jobs=2)


@pytest.mark.parametrize(("new", "published"), [("1.0.0", None), ("1.10.0", "1.9.3")])
def test_data_version_may_rise_above_the_published_one(new: str, published: str | None) -> None:
    upload.check_version_increases(new, published)


@pytest.mark.parametrize(("new", "published"), [("1.2.0", "1.2.0"), ("1.9.0", "1.10.0")])
def test_data_version_must_rise_above_the_published_one(new: str, published: str) -> None:
    with pytest.raises(ValueError, match="higher"):
        upload.check_version_increases(new, published)


@pytest.mark.parametrize(
    ("returncode", "stdout", "stderr", "expected"),
    [
        (0, '{"version": "1.2.0"}', "", "1.2.0"),
        (0, "{}", "", None),
        (1, "", "fatal error: An error occurred (404) when calling the HeadObject operation", None),
    ],
)
def test_published_version_reads_the_live_tilejson(
    monkeypatch: pytest.MonkeyPatch, returncode: int, stdout: str, stderr: str, expected: str | None
) -> None:
    result = subprocess.CompletedProcess([], returncode, stdout=stdout, stderr=stderr)
    monkeypatch.setattr(upload.subprocess, "run", lambda *_args, **_kwargs: result)
    assert upload.published_version(profile=None) == expected


def test_published_version_fails_when_the_bucket_cannot_be_read(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    result = subprocess.CompletedProcess([], 1, stdout="", stderr="An error occurred (403)")
    monkeypatch.setattr(upload.subprocess, "run", lambda *_args, **_kwargs: result)
    with pytest.raises(RuntimeError, match="403"):
        upload.published_version(profile=None)


class _Response(io.BytesIO):
    def __enter__(self) -> _Response:
        return self

    def __exit__(self, *_args: object) -> None:
        self.close()


def _http_error(code: int) -> urllib.error.HTTPError:
    return urllib.error.HTTPError("https://example.test", code, "error", {}, None)  # type: ignore[arg-type]


@pytest.mark.parametrize(
    ("outcomes", "calls", "raises"),
    [
        ([urllib.error.URLError("reset"), b"tile"], 2, None),
        ([_http_error(503), TimeoutError(), b"tile"], 3, None),
        ([_http_error(404)], 1, urllib.error.HTTPError),
        ([_http_error(503)] * 3, 3, RuntimeError),
    ],
)
def test_fetch_retries_only_transient_failures(
    monkeypatch: pytest.MonkeyPatch,
    outcomes: list[object],
    calls: int,
    raises: type[Exception] | None,
) -> None:
    remaining = iter(outcomes)
    made: list[str] = []

    def urlopen(url: str, timeout: float) -> _Response:
        made.append(url)
        outcome = next(remaining)
        if isinstance(outcome, Exception):
            raise outcome
        assert isinstance(outcome, bytes)
        return _Response(outcome)

    monkeypatch.setattr(fetch.urllib.request, "urlopen", urlopen)
    get = functools.partial(
        fetch.fetch_bytes, "https://example.test", timeout=1, attempts=3, backoff_seconds=0
    )
    if raises is None:
        assert get() == b"tile"
    else:
        with pytest.raises(raises):
            get()
    assert len(made) == calls


def test_notice_severity_needs_notice_text(tmp_path: Path) -> None:
    (tmp_path / "terrarium").mkdir()
    with pytest.raises(SystemExit, match="--notice"):
        cli.main(
            [
                "upload",
                "--out",
                str(tmp_path),
                "--prefix",
                "validation",
                "--data-version",
                "1.0.0",
                "--notice-severity",
                "critical",
                "--dryrun",
            ]
        )


@pytest.mark.parametrize(("value", "accepted"), [("1", True), ("0", False), ("-1", False)])
def test_validate_cli_requires_at_least_one_source_sample(value: str, accepted: bool) -> None:
    parse = functools.partial(
        cli.build_parser().parse_args, ["validate", "--out", "out", "--samples", value]
    )
    if accepted:
        assert parse().samples == int(value)
    else:
        with pytest.raises(SystemExit):
            parse()


def test_upload_needs_none_of_the_terrain_dependencies(tmp_path: Path) -> None:
    script = """
import sys
from pathlib import Path
for name in ("numpy", "rasterio", "PIL"):
    sys.modules[name] = None
from terrain import cli, metadata
cli.build_parser()
metadata.write_metadata(Path(sys.argv[1]), "https://example.test", None, "1.0.0")
"""
    tools_dir = Path(__file__).resolve().parents[1]
    result = subprocess.run(
        [sys.executable, "-c", script, str(tmp_path)],
        cwd=tools_dir,
        env={**os.environ, "PYTHONPATH": str(tools_dir)},
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
