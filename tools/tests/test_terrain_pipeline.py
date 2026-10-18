"""Tests for terrarium encoding, generation and validation (requires the terrain dependency group)."""

from __future__ import annotations

import io
import json
import zipfile
from concurrent.futures import Executor, Future, ThreadPoolExecutor
from concurrent.futures.process import BrokenProcessPool
from typing import TYPE_CHECKING

import pytest

np = pytest.importorskip("numpy")
pytest.importorskip("PIL")
pytest.importorskip("rasterio")

import rasterio  # noqa: E402
from PIL import Image  # noqa: E402
from rasterio.transform import from_origin  # noqa: E402
from terrain import (  # noqa: E402
    ardupilot,
    check,
    cli,
    compare,
    copernicus,
    generate,
    jobs,
    metadata,
    terrarium,
    tilemath,
    validate,
)

TILE_SIZE = tilemath.TILE_SIZE
TileKey = tilemath.TileKey
# Each straddles a 1-degree corner: Alps 8E/46N, Oslo 10E and the 60N spacing band.
ALPS_JOB = TileKey(generate.JOB_ZOOM, 267, 182)
OSLO_JOB = TileKey(generate.JOB_ZOOM, 270, 148)

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable
    from pathlib import Path


def ramp(lon, lat):
    """Linear in lon/lat, so bilinear interpolation must reproduce it exactly."""
    return 100.0 * lon + 10.0 * lat


def write_cell(
    directory: Path,
    lat: int,
    lon: int,
    res_x: float,
    res_y: float,
    void: tuple[slice, slice] | None = None,
    ramp_lon: int | None = None,
    field: Callable = ramp,
) -> str:
    """A Copernicus-style cell: pixel centers on whole multiples of the resolution, area-referenced.

    *ramp_lon* is the longitude the ramp values start at, when it differs from *lon* (a cell
    across the antimeridian holds the ramp continued past 180 degrees). *field* gives the heights.
    """
    width, height = round(1 / res_x), round(1 / res_y)
    lons = (lon if ramp_lon is None else ramp_lon) + np.arange(width) * res_x
    lats = lat + 1 - np.arange(height) * res_y
    data = field(lons[None, :], lats[:, None]).astype(np.float32)
    if void is not None:
        data[void] = copernicus.NODATA
    path = directory / f"cell_{lat}_{lon}.tif"
    with rasterio.open(
        path,
        "w",
        driver="GTiff",
        width=width,
        height=height,
        count=1,
        dtype="float32",
        crs="EPSG:4326",
        transform=from_origin(lon - res_x / 2, lat + 1 + res_y / 2, res_x, res_y),
    ) as dataset:
        dataset.write(data, 1)
    return str(path)


def expected_job_ramp(job: TileKey):
    lons, lats = validate.pixel_center_lonlats(job)
    return ramp(lons[None, :], lats[:, None])


def alps_cells(
    directory: Path,
    skip: tuple[int, int] | None = None,
    void_in_45_7: tuple[slice, slice] | None = None,
) -> dict[tuple[int, int], str]:
    """The four 0.01-degree cells under ALPS_JOB, optionally one missing or one with a void."""
    return {
        (lat, lon): write_cell(
            directory, lat, lon, 0.01, 0.01, void_in_45_7 if (lat, lon) == (45, 7) else None
        )
        for lat in (45, 46)
        for lon in (7, 8)
        if (lat, lon) != skip
    }


def test_encode_decode_round_trip_within_half_a_quantum() -> None:
    heights = np.array([[-432.7, 0.0, 0.001953], [8848.86, 1234.5678, -32768.0]], dtype=np.float32)
    decoded = terrarium.decode(terrarium.encode(heights))
    assert np.abs(decoded - heights).max() <= 0.5 / 256 + 1e-4


def test_encode_matches_the_terrarium_formula() -> None:
    rgb = terrarium.encode(np.array([[0.0, 1.5]], dtype=np.float32))
    assert rgb[0, 0].tolist() == [128, 0, 0]
    assert rgb[0, 1].tolist() == [128, 1, 128]


def test_encode_clamps_out_of_range_heights() -> None:
    rgb = terrarium.encode(np.array([[-40000.0, 40000.0]], dtype=np.float32))
    assert rgb[0, 0].tolist() == [0, 0, 0]
    assert rgb[0, 1].tolist() == [255, 255, 255]


def test_downsample_averages_heights_not_rgb() -> None:
    heights = np.array([[255.0, 257.0], [255.0, 257.0]], dtype=np.float32)
    assert terrarium.downsample(heights).tolist() == [[256.0]]


def test_png_round_trip_and_sea_tile(tmp_path: Path) -> None:
    rng = np.random.default_rng(0)
    heights = rng.uniform(-100, 5000, size=(TILE_SIZE, TILE_SIZE)).astype(np.float32)
    path = tmp_path / "tile.png"
    path.write_bytes(terrarium.png_bytes(heights))
    assert terrarium.tile_png_bytes(heights) == path.read_bytes()
    decoded = terrarium.read_heights(path)
    assert np.abs(decoded - heights).max() <= terrarium.HEIGHT_STEP / 2 + 1e-3
    steps = decoded / terrarium.HEIGHT_STEP
    assert np.array_equal(steps, np.rint(steps))

    sea = np.zeros((TILE_SIZE, TILE_SIZE), dtype=np.float32)
    assert terrarium.tile_png_bytes(sea) is terrarium.sea_png_bytes()
    assert not terrarium.heights_from_bytes(terrarium.sea_png_bytes()).any()
    near_sea = np.full((TILE_SIZE, TILE_SIZE), terrarium.HEIGHT_STEP / 4, dtype=np.float32)
    assert terrarium.tile_png_bytes(near_sea) is terrarium.sea_png_bytes()


def test_quantize_rounds_to_nearest_step_without_bias() -> None:
    step = terrarium.HEIGHT_STEP
    heights = np.array([0.1, 0.13, -0.13, 1234.374, 1234.376, -0.0], dtype=np.float32)
    assert terrarium.quantize(heights).tolist() == [0.0, 0.25, -0.25, 1234.25, 1234.5, 0.0]
    rng = np.random.default_rng(0)
    samples = rng.uniform(-500, 9000, 100_000)
    error = terrarium.quantize(samples).astype(np.float64) - samples
    assert np.abs(error).max() <= step / 2 + 1e-3
    assert abs(error.mean()) < 1e-3


@pytest.mark.parametrize(
    ("prefix", "tiles"),
    [
        (None, "https://example.test/terrarium/{z}/{x}/{y}.png"),
        ("validation", "https://example.test/validation/terrarium/{z}/{x}/{y}.png"),
    ],
)
def test_tilejson_describes_the_published_tiles(prefix: str | None, tiles: str) -> None:
    doc = metadata.tilejson("https://example.test", prefix, "1.2.0")
    assert doc["tiles"] == [tiles]
    assert doc["version"] == "1.2.0"
    assert (doc["minzoom"], doc["maxzoom"]) == (0, tilemath.MAX_ZOOM)
    assert doc["height_step_m"] == terrarium.HEIGHT_STEP
    assert doc["vertical_datum"] == "EGM2008"
    assert "WorldDEM-30" in str(doc["attribution"])
    assert "WorldDEM-90" in str(doc["attribution"])


def test_tilejson_carries_the_update_notice_for_users() -> None:
    notice = metadata.Notice("critical", "Coastal heights were too low; re-plan missions.")
    doc = metadata.tilejson("https://example.test", None, "1.1.0", notice)
    assert doc["notice"] == {"severity": "critical", "text": notice.text}
    assert "notice" not in metadata.tilejson("https://example.test", "validation", "0.1.0")


def test_write_metadata_files_carry_their_content_types(tmp_path: Path) -> None:
    files = metadata.write_metadata(tmp_path, "https://example.test", None, "1.0.0")
    # tilejson.json last, so uploading in order publishes the new version last.
    assert [(file.path.name, file.content_type) for file in files] == [
        (metadata.ATTRIBUTION_NAME, "text/plain; charset=utf-8"),
        (metadata.TILEJSON_NAME, "application/json"),
    ]
    assert json.loads(files[1].path.read_text(encoding="utf-8"))["tiles"]


def test_png_bytes_rejects_wrong_tile_size() -> None:
    wrong_size = np.zeros((128, 128), dtype=np.float32)
    with pytest.raises(ValueError):
        terrarium.png_bytes(wrong_size)
    with pytest.raises(ValueError):
        terrarium.tile_png_bytes(wrong_size)


def test_ocean_job_writes_full_pyramid_and_marker(tmp_path: Path) -> None:
    job = TileKey(generate.JOB_ZOOM, 10, 20)
    assert generate.run_job(tmp_path, job, {}, force=False) == "ocean"
    tiles = sorted((tmp_path / "terrarium").glob("*/*/*.png"))
    assert len(tiles) == 256 + 64 + 16 + 4 + 1
    assert generate.tile_path(
        tmp_path, TileKey(tilemath.MAX_ZOOM, 10 * 16 + 15, 20 * 16 + 15)
    ).is_file()
    assert jobs.completed_jobs(tmp_path) == {job: "ocean"}
    assert generate.run_job(tmp_path, job, {}, force=False) == "skipped"


def test_interrupted_force_regeneration_drops_the_done_marker(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    job = TileKey(generate.JOB_ZOOM, 10, 20)
    generate.run_job(tmp_path, job, {}, force=False)

    def fail(*_args: object) -> None:
        raise OSError("disk full")

    monkeypatch.setattr(generate, "write_tile", fail)
    with pytest.raises(OSError, match="disk full"):
        generate.run_job(tmp_path, job, {}, force=True)
    assert jobs.completed_jobs(tmp_path) == {}


def test_job_pyramid_places_tiles_by_position(tmp_path: Path) -> None:
    job = TileKey(generate.JOB_ZOOM, 1, 2)
    heights = np.zeros((generate.JOB_PIXELS, generate.JOB_PIXELS), dtype=np.float32)
    heights[:TILE_SIZE, -TILE_SIZE:] = 100.0
    generate.write_job_pyramid(tmp_path, job, heights)

    north_east = terrarium.read_heights(
        generate.tile_path(tmp_path, TileKey(tilemath.MAX_ZOOM, 16 + 15, 32))
    )
    assert np.allclose(north_east, 100.0)
    assert not terrarium.read_heights(
        generate.tile_path(tmp_path, TileKey(tilemath.MAX_ZOOM, 16, 32))
    ).any()
    job_tile = terrarium.read_heights(generate.tile_path(tmp_path, job))
    assert np.allclose(job_tile[:16, -16:], 100.0)
    assert not job_tile[16:, :].any()


def test_build_overviews_fills_missing_children_with_sea(tmp_path: Path) -> None:
    generate.write_tile(
        tmp_path,
        TileKey(generate.JOB_ZOOM, 0, 0),
        np.full((TILE_SIZE, TILE_SIZE), 400.0, np.float32),
    )
    generate.build_overviews(tmp_path)

    parent = TileKey(generate.JOB_ZOOM - 1, 0, 0)
    overview = terrarium.read_heights(generate.tile_path(tmp_path, parent))
    assert np.allclose(overview[: TILE_SIZE // 2, : TILE_SIZE // 2], 400.0)
    assert not overview[TILE_SIZE // 2 :, :].any()
    assert generate.tile_path(tmp_path, TileKey(0, 0, 0)).is_file()


def test_interrupted_overview_rebuild_removes_the_root_tile(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    generate.write_tile(
        tmp_path, TileKey(generate.JOB_ZOOM, 0, 0), np.zeros((TILE_SIZE, TILE_SIZE), np.float32)
    )
    generate.build_overviews(tmp_path)
    root = generate.tile_path(tmp_path, TileKey(0, 0, 0))
    assert root.is_file()

    def fail(*_args: object) -> None:
        raise OSError("disk full")

    monkeypatch.setattr(generate, "write_tile", fail)
    with pytest.raises(OSError, match="disk full"):
        generate.build_overviews(tmp_path)
    assert not root.exists()


def test_lattice_bounds_land_on_pixel_edges() -> None:
    res = 1 / 3600
    west, south, east, north = generate.lattice_bounds(7.0001, 46.0001, 7.9999, 46.9999, res, res)
    for edge in (west, south, east, north):
        assert (edge / res - 0.5) == pytest.approx(round(edge / res - 0.5), abs=1e-6)
    assert west <= 7.0001 and south <= 46.0001 and east >= 7.9999 and north >= 46.9999
    assert 7.0001 - west < res and east - 7.9999 < res


def test_job_matches_source_exactly_across_cell_borders(tmp_path: Path) -> None:
    cells = alps_cells(tmp_path)
    heights, voids = generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))
    assert voids == 0
    assert np.abs(heights - expected_job_ramp(ALPS_JOB)).max() < 0.01


def test_job_at_the_antimeridian_reads_the_cell_across_it(tmp_path: Path) -> None:
    # Ends at 180 degrees east, just north of the equator; W180 continues the ramp past 180.
    job = TileKey(
        generate.JOB_ZOOM, (1 << generate.JOB_ZOOM) - 1, (1 << generate.JOB_ZOOM) // 2 - 1
    )
    cells = {
        (lat, lon): write_cell(
            tmp_path, lat, lon, 0.01, 0.01, ramp_lon=lon + 360 if lon == -180 else None
        )
        for lat in (-1, 0)
        for lon in (179, -180)
    }
    heights, voids = generate.warp_job(job, generate.job_cells(job, cells))
    assert voids == 0
    assert np.abs(heights - expected_job_ramp(job)).max() < 0.01


@pytest.mark.parametrize(
    ("key_lon", "cell_lon"),
    [(180, -180), (-181, 179)],
    ids=["east-of-180", "west-of-minus-180"],
)
def test_open_cell_places_a_cell_across_the_antimeridian(
    tmp_path: Path, key_lon: int, cell_lon: int
) -> None:
    url = write_cell(tmp_path, 0, cell_lon, 0.01, 0.01)
    with generate._open_cell(key_lon, url) as shifted, rasterio.open(url) as source:
        assert shifted.bounds.left == pytest.approx(source.bounds.left + (key_lon - cell_lon))
        assert np.array_equal(shifted.read(1), source.read(1))


def test_open_cell_shifts_cells_of_any_data_type(tmp_path: Path) -> None:
    path = tmp_path / "int32.tif"
    data = np.arange(100, dtype=np.int32).reshape(10, 10)
    with rasterio.open(
        path,
        "w",
        driver="GTiff",
        width=10,
        height=10,
        count=1,
        dtype="int32",
        crs="EPSG:4326",
        transform=from_origin(-180.05, 1.05, 0.1, 0.1),
    ) as dataset:
        dataset.write(data, 1)
    with generate._open_cell(180, str(path)) as shifted:
        assert np.array_equal(shifted.read(1), data)


def test_job_spanning_a_longitude_spacing_band_has_no_gaps(tmp_path: Path) -> None:
    res_y = 1 / 120
    cells = {
        (lat, lon): write_cell(tmp_path, lat, lon, (1.5 if lat == 59 else 2.0) * res_y, res_y)
        for lat in (59, 60)
        for lon in (9, 10, 11)
    }
    heights, voids = generate.warp_job(OSLO_JOB, generate.job_cells(OSLO_JOB, cells))
    assert voids == 0
    assert np.abs(heights - expected_job_ramp(OSLO_JOB)).max() < 0.01


def test_missing_cells_are_sea_level_and_land_is_untouched(tmp_path: Path) -> None:
    cells = alps_cells(tmp_path, skip=(45, 8))
    heights, _ = generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))
    lons, lats = validate.pixel_center_lonlats(ALPS_JOB)
    in_missing = (lats[:, None] < 45.95) & (lons[None, :] > 8.05)
    in_land = (lats[:, None] > 46.05) | (lons[None, :] < 7.95)
    assert in_missing.any() and not heights[in_missing].any()
    assert np.abs(heights - expected_job_ramp(ALPS_JOB))[in_land].max() < 0.01


def test_void_filling_leaves_neighboring_ocean_at_sea_level(tmp_path: Path) -> None:
    cells = alps_cells(tmp_path, skip=(45, 8), void_in_45_7=(slice(18, 21), slice(85, 88)))
    heights, voids = generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))
    assert voids == 9
    lons, lats = validate.pixel_center_lonlats(ALPS_JOB)
    ocean = (lats[:, None] < 45.95) & (lons[None, :] > 8.05)
    assert ocean.any() and not heights[ocean].any()


def test_small_voids_are_interpolated_not_sea_level(tmp_path: Path) -> None:
    void = (slice(18, 21), slice(85, 88))
    cells = alps_cells(tmp_path, void_in_45_7=void)
    heights, voids = generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))
    assert voids == 9
    assert np.abs(heights - expected_job_ramp(ALPS_JOB)).max() < 2.0


def test_gap_between_cells_on_different_lattices_is_filled_not_sea(tmp_path: Path) -> None:
    # GLO-90 (3 arc-second) north of GLO-30 (1 arc-second): their pixel areas leave a gap.
    cells = {
        (45, 7): write_cell(tmp_path, 45, 7, 0.01, 0.01),
        (45, 8): write_cell(tmp_path, 45, 8, 0.01, 0.01),
        (46, 7): write_cell(tmp_path, 46, 7, 0.04, 0.04),
        (46, 8): write_cell(tmp_path, 46, 8, 0.04, 0.04),
    }
    heights, _ = generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))
    assert heights.min() > 1000.0
    assert np.abs(heights - expected_job_ramp(ALPS_JOB)).max() < 0.5


def test_void_across_a_job_edge_fills_the_same_in_both_jobs(tmp_path: Path) -> None:
    # Lon 8.4375 is the edge between ALPS_JOB and its east neighbor; the void spans it.
    void = (slice(18, 21), slice(42, 47))
    cells = {
        (lat, lon): write_cell(
            tmp_path, lat, lon, 0.01, 0.01, void if (lat, lon) == (45, 8) else None
        )
        for lat in (45, 46)
        for lon in (7, 8)
    }
    east_job = TileKey(ALPS_JOB.z, ALPS_JOB.x + 1, ALPS_JOB.y)
    west, west_voids = generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))
    east, east_voids = generate.warp_job(east_job, generate.job_cells(east_job, cells))
    assert west_voids > 0 and east_voids > 0
    # Neighboring pixels across the edge differ by a few centimeters on this ramp.
    assert np.abs(west[:, -1] - east[:, 0]).max() < 0.1


def test_job_heights_ignore_cells_on_other_lattices_beyond_the_job(tmp_path: Path) -> None:
    res_y = 1 / 120

    def bumpy(lon, lat):
        return 300.0 + 200.0 * np.sin(lon * 150) * np.cos(lat * 130)

    cells = {
        (lat, lon): write_cell(
            tmp_path, lat, lon, (1.5 if lat == 59 else 2.0) * res_y, res_y, field=bumpy
        )
        for lat in (59, 60)
        for lon in (9, 10, 11)
    }
    # 60.24 to 60.59 N: wholly in the 2.0 band, but its cell buffer reaches the 59 N row.
    job = TileKey(generate.JOB_ZOOM, 270, 147)
    buffered = generate.job_cells(job, cells)
    assert any(lat == 59 for lat, _ in buffered)
    own = {key: url for key, url in cells.items() if key[0] == 60}
    with_neighbors, _ = generate.warp_job(job, buffered)
    alone, _ = generate.warp_job(job, own)
    assert np.allclose(with_neighbors, alone, atol=1e-3)


def test_unfillable_voids_fail_the_job(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(generate, "VOID_FILL_PIXELS", 1)
    cells = alps_cells(tmp_path, void_in_45_7=(slice(5, 35), slice(78, 98)))
    with pytest.raises(RuntimeError, match="could not be filled"):
        generate.warp_job(ALPS_JOB, generate.job_cells(ALPS_JOB, cells))


def test_source_sampler_interpolates_across_east_and_south_edges(tmp_path: Path) -> None:
    sampler = validate.SourceSampler(alps_cells(tmp_path))
    try:
        for lon, lat in ((7.9951, 46.5), (7.5, 46.0049), (7.9951, 46.0049), (7.123, 45.456)):
            assert sampler.height(lon, lat) == pytest.approx(ramp(lon, lat), abs=1e-3)
        assert sampler.height(20.5, 46.5) == terrarium.SEA_LEVEL
    finally:
        sampler.close()


def test_source_sampler_interpolates_across_the_antimeridian(tmp_path: Path) -> None:
    cells = {
        (0, 179): write_cell(tmp_path, 0, 179, 0.01, 0.01),
        (0, -180): write_cell(tmp_path, 0, -180, 0.01, 0.01, ramp_lon=180),
    }
    sampler = validate.SourceSampler(cells)
    try:
        assert sampler.height(179.995, 0.5) == pytest.approx(ramp(179.995, 0.5), abs=1e-3)
    finally:
        sampler.close()


@pytest.mark.parametrize(("col", "row"), [(-1, 0), (0, -1), (-1, -1)])
def test_source_sampler_leaves_out_neighbors_before_the_west_or_north_edge(
    tmp_path: Path, col: int, row: int
) -> None:
    # Only floating-point rounding at a whole-degree edge can produce these indices.
    sampler = validate.SourceSampler(alps_cells(tmp_path))
    try:
        assert sampler._lattice_value((46, 7), col, row) is None
    finally:
        sampler.close()


# Crosses no whole-degree line, so it has no cell borders to sample.
NO_BORDER_JOB = TileKey(generate.JOB_ZOOM, 10, 200)


@pytest.mark.parametrize(
    ("job", "samples", "border_samples"),
    [
        (ALPS_JOB, 400, 200),
        (ALPS_JOB, 3, 1),
        (ALPS_JOB, 1, 0),
        (NO_BORDER_JOB, 400, 0),
        (NO_BORDER_JOB, 1, 0),
    ],
)
def test_source_samples_honor_the_requested_count(
    job: TileKey, samples: int, border_samples: int
) -> None:
    pixels = validate._sample_pixels(job, samples, np.random.default_rng(0))
    assert len(pixels) == samples
    assert sum(near_border for _, _, near_border in pixels) == border_samples
    assert all(
        0 <= row < generate.JOB_PIXELS and 0 <= col < generate.JOB_PIXELS for row, col, _ in pixels
    )


def test_source_samples_reject_an_empty_request() -> None:
    with pytest.raises(ValueError):
        validate._sample_pixels(ALPS_JOB, 0, np.random.default_rng(0))


def test_source_check_reports_no_samples_instead_of_failing(tmp_path: Path) -> None:
    everywhere = (slice(None), slice(None))
    cells = {
        (lat, lon): write_cell(tmp_path, lat, lon, 0.01, 0.01, everywhere)
        for lat in (45, 46)
        for lon in (7, 8)
    }
    ours = np.zeros((generate.JOB_PIXELS, generate.JOB_PIXELS), dtype=np.float32)
    result = validate.check_source(ALPS_JOB, ours, cells, 20, np.random.default_rng(0))
    assert result.samples == 0


class _BrokenPool(Executor):
    """Stands in for a process pool whose workers died: queued jobs fail, then submit raises."""

    def __init__(self, max_workers: int) -> None:
        self.submitted = 0

    def submit(self, fn: Callable[..., object], /, *args: object, **kwargs: object) -> Future:
        self.submitted += 1
        if self.submitted > 2:
            raise BrokenProcessPool("a worker died")
        future: Future[object] = Future()
        future.set_exception(BrokenProcessPool("a worker died"))
        return future


def test_a_broken_worker_pool_counts_every_unfinished_job_as_failed(tmp_path: Path) -> None:
    job_list = [TileKey(generate.JOB_ZOOM, x, 0) for x in range(5)]
    failures = generate.generate(tmp_path, job_list, {}, 1, False, executor_factory=_BrokenPool)
    assert failures == len(job_list)


def test_fit_offset_recovers_a_known_shift() -> None:
    rows, cols = np.mgrid[0:200, 0:200].astype(np.float64)

    def field(x, y):
        return 300 * np.sin(x / 17) * np.cos(y / 23)

    ours = field(cols, rows)
    reference = field(cols + 0.3, rows - 0.2) + 5.0
    offset_x, offset_y, bias = validate.fit_offset(ours, reference)
    assert offset_x == pytest.approx(0.3, abs=0.02)
    assert offset_y == pytest.approx(-0.2, abs=0.02)
    assert bias == pytest.approx(5.0, abs=0.5)


def test_fit_offset_reports_nan_when_a_plane_cannot_separate_shift_from_bias() -> None:
    # On a plane a shift along the slope and a vertical bias are indistinguishable.
    cols = np.tile(np.arange(200, dtype=np.float64), (200, 1))
    offsets = validate.fit_offset(cols, cols + 5.0)
    assert np.isnan(offsets).all()


def test_validate_records_unreadable_job_tiles_and_checks_the_remaining_jobs(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr(cli, "load_cells", lambda _cache: {})
    for x in (1, 2):
        jobs.mark_done(tmp_path, TileKey(generate.JOB_ZOOM, x, 1), "ocean")
    status = cli.main(["validate", "--out", str(tmp_path), "--job", "9/1/1", "--job", "9/2/1"])
    output = capsys.readouterr().out
    assert status == 1
    assert "9/1/1: tiles unreadable" in output
    assert "9/2/1: tiles unreadable" in output


def test_parse_hgt_marks_voids_and_rejects_non_square() -> None:
    data = np.array([[1, 2], [ardupilot.HGT_VOID, 4]], dtype=">i2").tobytes()
    grid = ardupilot.parse_hgt(data)
    assert grid[0].tolist() == [1.0, 2.0]
    assert np.isnan(grid[1, 0])
    with pytest.raises(ValueError):
        ardupilot.parse_hgt(b"\x00" * 6)


def test_hgt_sampling_is_pixel_is_point_with_shared_edges(tmp_path: Path) -> None:
    cells = ardupilot.HgtCells(tmp_path)
    spacing = 4
    for lat, lon in ((46, 7), (46, 8)):
        offsets = np.arange(spacing + 1) / spacing
        lons = lon + offsets
        lats = lat + 1 - offsets
        cells._cells[(lat, lon)] = ramp(lons[None, :], lats[:, None]).astype(np.float32)
    cells._cells[(45, 7)] = None
    lons = np.array([7.0, 7.3, 7.999, 8.0, 8.6])
    lats = np.array([46.999, 46.55, 46.0])
    assert np.allclose(cells.sample_grid(lons, lats), ramp(lons[None, :], lats[:, None]), atol=1e-3)
    assert np.isnan(cells.sample_grid(np.array([7.5]), np.array([45.5]))).all()
    assert ardupilot.hgt_name(-33, -70) == "S33W070"


def test_fit_offset_strides_large_grids_without_losing_accuracy() -> None:
    side = 2100
    assert side * side // validate.MAX_FIT_POINTS >= 4
    rows, cols = np.mgrid[0:side, 0:side].astype(np.float32)

    def field(x, y):
        return 300 * np.sin(x / 17) * np.cos(y / 23)

    offset_x, offset_y, bias = validate.fit_offset(
        field(cols, rows), field(cols - 0.25, rows + 0.4) - 3.0
    )
    assert offset_x == pytest.approx(-0.25, abs=0.02)
    assert offset_y == pytest.approx(0.4, abs=0.02)
    assert bias == pytest.approx(-3.0, abs=0.5)


def test_hgt_sampling_is_exact_across_row_chunks(tmp_path: Path) -> None:
    cells = ardupilot.HgtCells(tmp_path)
    spacing = 1200
    offsets = np.arange(spacing + 1) / spacing
    cells._cells[(46, 7)] = ramp(7 + offsets[None, :], 47 - offsets[:, None]).astype(np.float32)
    lats = 47 - (np.arange(1500) + 0.5) / 1500
    lons = np.array([7.001, 7.5, 7.999])
    assert np.allclose(cells.sample_grid(lons, lats), ramp(lons[None, :], lats[:, None]), atol=1e-2)


def test_hgt_cells_keep_a_bounded_lru(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    def archive(name: str) -> bytes:
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w") as bundle:
            bundle.writestr(f"{name}.hgt", np.zeros((3, 3), dtype=">i2").tobytes())
        return buffer.getvalue()

    cells = ardupilot.HgtCells(tmp_path)
    monkeypatch.setattr(cells, "_download", archive)
    keys = [(46, lon) for lon in range(ardupilot.MAX_CACHED_CELLS + 2)]
    for lat, lon in keys[: ardupilot.MAX_CACHED_CELLS]:
        cells.cell(lat, lon)
    cells.cell(*keys[0])
    for lat, lon in keys[ardupilot.MAX_CACHED_CELLS :]:
        cells.cell(lat, lon)
    assert len(cells._cells) == ardupilot.MAX_CACHED_CELLS
    assert keys[0] in cells._cells
    assert keys[1] not in cells._cells and keys[2] not in cells._cells


def ramp_tile(base: float = 100.0, slope: float = 0.5):
    columns = np.arange(TILE_SIZE, dtype=np.float32)
    return np.tile(base + slope * columns, (TILE_SIZE, 1))


def tile_bytes(heights) -> bytes:
    return terrarium.png_bytes(heights)


def test_decode_tile_accepts_a_valid_tile() -> None:
    heights, problems = check.decode_tile(tile_bytes(ramp_tile()))
    assert problems == []
    assert heights is not None
    assert np.allclose(heights, ramp_tile())


def _rgb_png(rgb) -> bytes:
    buffer = io.BytesIO()
    Image.fromarray(rgb).save(buffer, format="PNG")
    return buffer.getvalue()


@pytest.mark.parametrize(
    ("data", "problem"),
    [
        (b"not a png", "readable"),
        (_rgb_png(np.zeros((128, 128, 3), np.uint8)), "256x256"),
        (_rgb_png(terrarium.encode(np.full((TILE_SIZE, TILE_SIZE), 100.1, np.float32))), "steps"),
        (tile_bytes(np.full((TILE_SIZE, TILE_SIZE), 9500.0, np.float32)), "outside"),
        (_rgb_png(terrarium.encode(np.zeros((TILE_SIZE, TILE_SIZE), np.float32))), "shared sea"),
    ],
    ids=["not-png", "wrong-size", "unquantized", "out-of-range", "unshared-sea"],
)
def test_decode_tile_reports_structural_problems(data: bytes, problem: str) -> None:
    _, problems = check.decode_tile(data)
    assert any(problem in text for text in problems), problems


@pytest.mark.parametrize(
    ("offset", "broken"),
    [(0.0, False), (0.25, False), (60.0, True), (-60.0, True)],
)
def test_seam_is_broken_only_when_the_jump_dwarfs_the_neighboring_steps(
    offset: float, broken: bool
) -> None:
    left = ramp_tile()
    right = ramp_tile(base=100.0 + 0.5 * TILE_SIZE + offset)
    assert check.seam_is_broken(left[:, -2], left[:, -1], right[:, 0], right[:, 1]) == broken


def test_seam_on_a_steep_slope_is_not_broken() -> None:
    left = ramp_tile(slope=40.0)
    right = ramp_tile(base=100.0 + 40.0 * TILE_SIZE, slope=40.0)
    assert not check.seam_is_broken(left[:, -2], left[:, -1], right[:, 0], right[:, 1])


@pytest.mark.parametrize(("boundary_height", "flagged"), [(2500.0, True), (3000.0, False)])
def test_jump_across_a_tile_boundary_is_held_to_the_steep_limit(
    boundary_height: float, flagged: bool
) -> None:
    # Steps beside the boundary are at the limit, so the seam ratio test passes either way.
    key = TileKey(tilemath.MAX_ZOOM, 5, 5)
    heights = np.full((TILE_SIZE, TILE_SIZE), 5000.0, np.float32)
    heights[:, -1] = 4000.0
    east = np.full((TILE_SIZE, TILE_SIZE), 3500.0, np.float32)
    east[:, 0] = boundary_height
    problems = check._check_seams(key, heights, east, None)
    assert not any("seam" in found for found in problems), problems
    assert any("jump to its east neighbor" in found for found in problems) == flagged, problems


def test_steep_limit_never_drops_below_the_absolute_jump_limit() -> None:
    assert check.steep_limit_m(TileKey(tilemath.MAX_ZOOM, 4000, 4000)) == check.STEEP_JUMP_M
    assert check.steep_limit_m(TileKey(2, 1, 1)) > check.STEEP_JUMP_M


def ocean_build(out: Path, xs: Iterable[int]) -> list[TileKey]:
    built = [TileKey(generate.JOB_ZOOM, x, 0) for x in xs]
    for job in built:
        generate.run_job(out, job, {}, force=False)
    generate.build_overviews(out)
    return built


def test_check_build_finds_a_seam_across_the_antimeridian(tmp_path: Path) -> None:
    last_job_x = (1 << generate.JOB_ZOOM) - 1
    ocean_build(tmp_path, [0, last_job_x])
    _plant(
        tmp_path,
        TileKey(tilemath.MAX_ZOOM, 0, 0),
        np.full((TILE_SIZE, TILE_SIZE), 500.0, np.float32),
    )
    problems = check.check_build(tmp_path, workers=2, executor_factory=ThreadPoolExecutor)
    last_tile_x = (1 << tilemath.MAX_ZOOM) - 1
    assert f"13/{last_tile_x}/0: seam with its east neighbor" in problems, problems


def test_check_build_passes_a_clean_build_and_marks_it(tmp_path: Path) -> None:
    ocean_build(tmp_path, range(2))
    assert check.check_build(tmp_path, workers=1) == []
    assert jobs.check_passed_marker(tmp_path).is_file()


def _plant(out: Path, key: TileKey, heights) -> None:
    generate.write_tile(out, key, heights)


@pytest.mark.parametrize(
    ("fault", "expected"),
    [
        ("missing", "missing"),
        ("seam-inside-job", "13/4/5: seam with its east neighbor"),
        ("seam-between-jobs", "13/15/3: seam with its east neighbor"),
        ("steep", "jump"),
        ("missing-parent", "parent"),
    ],
)
def test_check_build_finds_planted_faults(tmp_path: Path, fault: str, expected: str) -> None:
    ocean_build(tmp_path, range(2))
    z13 = tilemath.MAX_ZOOM
    if fault == "missing":
        generate.tile_path(tmp_path, TileKey(z13, 5, 5)).unlink()
    elif fault == "seam-inside-job":
        _plant(tmp_path, TileKey(z13, 5, 5), np.full((TILE_SIZE, TILE_SIZE), 500.0, np.float32))
    elif fault == "seam-between-jobs":
        # First column of z13 tiles in job x=1, next to job x=0's last column.
        _plant(tmp_path, TileKey(z13, 16, 3), np.full((TILE_SIZE, TILE_SIZE), 500.0, np.float32))
    elif fault == "steep":
        heights = np.zeros((TILE_SIZE, TILE_SIZE), np.float32)
        heights[100:110, 100:110] = 2000.0
        _plant(tmp_path, TileKey(z13, 5, 5), heights)
    elif fault == "missing-parent":
        generate.tile_path(tmp_path, TileKey(generate.JOB_ZOOM - 1, 0, 0)).unlink()
    problems = check.check_build(tmp_path, workers=2, executor_factory=ThreadPoolExecutor)
    assert any(expected in problem for problem in problems), problems
    assert not jobs.check_passed_marker(tmp_path).exists()


def _plant_below(out: Path, key: TileKey, children: int, deep: bool) -> None:
    for child in tilemath.children(key)[:children]:
        _plant(out, child, np.zeros((TILE_SIZE, TILE_SIZE), np.float32))
        if deep and child.z < generate.JOB_ZOOM:
            _plant_below(out, child, 4, deep)


@pytest.mark.parametrize(
    ("levels_above_jobs", "children", "deep", "flagged"),
    [(1, 1, False, False), (1, 4, False, True), (2, 4, False, False), (2, 4, True, True)],
    ids=["one-child", "every-child", "grandchildren-missing", "every-descendant"],
)
def test_overview_jumps_count_only_when_every_job_below_exists(
    tmp_path: Path, levels_above_jobs: int, children: int, deep: bool, flagged: bool
) -> None:
    # In a partial build, overviews fill missing descendants with sea level by design, so a
    # land quadrant next to that fill is not a fault; with every job below present it would be.
    key = TileKey(generate.JOB_ZOOM - levels_above_jobs, 0, 0)
    heights = np.zeros((TILE_SIZE, TILE_SIZE), np.float32)
    heights[: TILE_SIZE // 2, : TILE_SIZE // 2] = 3000.0
    _plant(tmp_path, key, heights)
    _plant_below(tmp_path, key, children, deep)
    problems = check.check_overview_tile(tmp_path, key)
    assert any("jump" in problem for problem in problems) == flagged, problems


def test_size_report_counts_only_true_sea_tiles(tmp_path: Path) -> None:
    z13 = tilemath.MAX_ZOOM
    sea_size = len(terrarium.sea_png_bytes())
    _plant(tmp_path, TileKey(z13, 0, 0), np.zeros((TILE_SIZE, TILE_SIZE), np.float32))
    # A flattened lake can compress to exactly the sea tile's size.
    lake = np.full((TILE_SIZE, TILE_SIZE), 130.0, np.float32)
    _plant(tmp_path, TileKey(z13, 1, 0), lake)
    assert generate.tile_path(tmp_path, TileKey(z13, 1, 0)).stat().st_size == sea_size
    _plant(tmp_path, TileKey(z13, 2, 0), ramp_tile())

    (row,) = validate.size_report(tmp_path)
    assert (row.count, row.sea_count) == (3, 1)


def test_compare_builds_reports_identical_changed_added_and_removed(tmp_path: Path) -> None:
    old, new = tmp_path / "old", tmp_path / "new"
    z13 = tilemath.MAX_ZOOM
    for out in (old, new):
        _plant(out, TileKey(z13, 0, 0), ramp_tile())
        _plant(out, TileKey(z13, 1, 0), ramp_tile())
    _plant(new, TileKey(z13, 1, 0), ramp_tile(base=103.0))
    _plant(old, TileKey(z13, 2, 0), ramp_tile())
    _plant(new, TileKey(z13, 3, 0), ramp_tile())

    listed = io.StringIO()
    report = compare.compare_builds(new, old, workers=1, top=5, list_file=listed)
    (zoom,) = report.zooms
    assert (zoom.zoom, zoom.identical, zoom.changed, zoom.added, zoom.removed) == (z13, 1, 1, 1, 1)
    assert zoom.max_change_m == pytest.approx(3.0)
    assert [change.key for change in report.top_changes] == [TileKey(z13, 1, 0)]
    assert sorted(listed.getvalue().split()) == ["13/1/0.png", "13/3/0.png"]
    assert report.listed == 2


def test_compare_builds_reports_unreadable_tiles_instead_of_failing(tmp_path: Path) -> None:
    old, new = tmp_path / "old", tmp_path / "new"
    key = TileKey(tilemath.MAX_ZOOM, 0, 0)
    _plant(old, key, ramp_tile())
    path = generate.tile_path(new, key)
    path.parent.mkdir(parents=True)
    path.write_bytes(b"not a png")
    # A read error (here a directory in place of the file) is unreadable too, not fatal.
    io_error_key = TileKey(key.z, key.x, 1)
    _plant(old, io_error_key, ramp_tile())
    generate.tile_path(new, io_error_key).mkdir()
    report = compare.compare_builds(new, old, workers=1, top=5)
    (zoom,) = report.zooms
    assert (zoom.changed, zoom.unreadable) == (2, 2)
    assert {change.key for change in report.top_changes} == {key, io_error_key}
