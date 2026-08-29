"""Tests for Copernicus DEM elevation provider core logic in qgc4qgis.core.elevation."""

import re
import subprocess
import sys
from pathlib import Path

import pytest
from qgis.core import QgsRasterLayer

from qgc4qgis.core import elevation
from qgc4qgis.core.elevation import (
    CarpetTile,
    build_dem_geotiff,
    carpet_url,
    download_dem,
    fetch_carpet,
    parse_carpet,
    tile_grid,
)
from qgc4qgis.core.terrain import sample_terrain


def test_carpet_url_formatting():
    """Verify carpet_url produces query parameter with sw_lat, sw_lon, ne_lat, ne_lon in order."""
    sw_lat, sw_lon = -23.56, -46.64
    ne_lat, ne_lon = -23.55, -46.63
    url = carpet_url(sw_lat, sw_lon, ne_lat, ne_lon)
    expected_points = "-23.5600000000,-46.6400000000,-23.5500000000,-46.6300000000"
    assert url == f"https://terrain-ce.suite.auterion.com/api/v1/carpet?points={expected_points}"


def test_tile_grid_partitioning_and_limits():
    """Verify tile_grid returns 1 tile for 0.01°, 4 tiles for 0.02°, and raises ValueError for 1°."""
    grid_1 = tile_grid(-23.56, -46.64, -23.55, -46.63)
    assert len(grid_1) == 1

    grid_4 = tile_grid(-23.56, -46.64, -23.54, -46.62)
    assert len(grid_4) == 4

    with pytest.raises(ValueError):
        tile_grid(0.0, 0.0, 1.0, 1.0)


def test_parse_carpet_valid_payload():
    """Verify parse_carpet extracts n_rows, n_cols, and lat_step from a real inline payload."""
    payload = {
        "status": "success",
        "data": {
            "bounds": {"sw": [-23.56, -46.64], "ne": [-23.55, -46.63]},
            "carpet": [[10.0, 11.0, 12.0], [20.0, 21.0, 22.0]],
        },
    }
    tile = parse_carpet(payload)
    assert tile.n_rows == 2
    assert tile.n_cols == 3
    assert pytest.approx(tile.lat_step) == 0.01


def test_parse_carpet_invalid_payloads():
    """Verify parse_carpet raises ValueError for error status, empty carpet, and mismatched row lengths."""
    with pytest.raises(ValueError):
        parse_carpet({"status": "error", "message": "Failed"})

    empty_carpet_payload = {
        "status": "success",
        "data": {
            "bounds": {"sw": [-23.56, -46.64], "ne": [-23.55, -46.63]},
            "carpet": [],
        },
    }
    with pytest.raises(ValueError):
        parse_carpet(empty_carpet_payload)

    mismatched_rows_payload = {
        "status": "success",
        "data": {
            "bounds": {"sw": [-23.56, -46.64], "ne": [-23.55, -46.63]},
            "carpet": [[10.0, 11.0, 12.0], [20.0, 21.0]],
        },
    }
    with pytest.raises(ValueError):
        parse_carpet(mismatched_rows_payload)


def test_build_dem_geotiff_adjacent_tiles_and_row_inversion(tmp_path):
    """Verify build_dem_geotiff merges synthetic tiles and inverts row order correctly."""
    tile_west = CarpetTile(
        sw_lat=-23.56,
        sw_lon=-46.64,
        ne_lat=-23.55,
        ne_lon=-46.63,
        rows=[
            [100.0, 101.0, 102.0],
            [200.0, 201.0, 202.0],
            [300.0, 301.0, 302.0],
        ],
    )
    tile_east = CarpetTile(
        sw_lat=-23.56,
        sw_lon=-46.63,
        ne_lat=-23.55,
        ne_lon=-46.62,
        rows=[
            [110.0, 111.0, 112.0],
            [210.0, 211.0, 212.0],
            [310.0, 311.0, 312.0],
        ],
    )

    out_path = str(tmp_path / "dem_composite.tif")

    with pytest.raises(ValueError):
        build_dem_geotiff([], out_path)

    result_path = build_dem_geotiff([tile_west, tile_east], out_path)
    assert result_path == out_path

    layer = QgsRasterLayer(out_path, "dem_composite")
    assert layer.isValid()

    # Point near north corner (lat = -23.55) returns the value from the last row (row 2: 300.0)
    sampled_north = sample_terrain(layer, [(-23.55, -46.64)])
    assert sampled_north[0] == pytest.approx(300.0)

    # Point near south corner (lat = -23.56) returns value from row 0 (100.0)
    sampled_south = sample_terrain(layer, [(-23.56, -46.64)])
    assert sampled_south[0] == pytest.approx(100.0)

    # East tile north-east point returns value from east last row
    sampled_east_north = sample_terrain(layer, [(-23.55, -46.62)])
    assert sampled_east_north[0] == pytest.approx(312.0)


def test_download_dem_with_fake_fetch(tmp_path):
    """Verify download_dem calls fake fetch once per tile, reports progress, and creates valid GeoTIFF."""
    calls = []

    def fake_fetch(url: str) -> dict:
        calls.append(url)
        query = url.split("points=")[1]
        sw_lat_str, sw_lon_str, ne_lat_str, ne_lon_str = query.split(",")
        sw_l_lat, sw_l_lon = float(sw_lat_str), float(sw_lon_str)
        ne_l_lat, ne_l_lon = float(ne_lat_str), float(ne_lon_str)
        return {
            "status": "success",
            "data": {
                "bounds": {
                    "sw": [sw_l_lat, sw_l_lon],
                    "ne": [ne_l_lat, ne_l_lon],
                },
                "carpet": [
                    [100.0, 101.0],
                    [102.0, 103.0],
                ],
            },
        }

    class MockFeedback:
        def __init__(self):
            self.progress_history = []

        def setProgress(self, val: int):
            self.progress_history.append(val)

        def isCanceled(self) -> bool:
            return False

    feedback = MockFeedback()
    out_path = str(tmp_path / "synthetic_dem.tif")

    # 4 tiles grid (0.02 x 0.02 box)
    res_path = download_dem(
        -23.56,
        -46.64,
        -23.54,
        -46.62,
        out_path,
        fetch=fake_fetch,
        feedback=feedback,
    )

    assert res_path == out_path
    assert len(calls) == 4
    assert feedback.progress_history == [25, 50, 75, 100]

    layer = QgsRasterLayer(out_path, "synthetic_dem")
    assert layer.isValid()


def test_download_dem_fetch_runtime_error_propagation(tmp_path):
    """Verify download_dem propagates RuntimeError raised by fetch."""

    def failing_fetch(url: str) -> dict:
        raise RuntimeError("Server network error 500")

    out_path = str(tmp_path / "failed_dem.tif")
    with pytest.raises(RuntimeError, match="Server network error 500"):
        download_dem(
            -23.56,
            -46.64,
            -23.55,
            -46.63,
            out_path,
            fetch=failing_fetch,
        )


def test_fetch_carpet_invalid_url():
    """Verify fetch_carpet raises RuntimeError when network request fails."""
    with pytest.raises(RuntimeError):
        fetch_carpet("http://invalid.domain.that.does.not.exist.12345/api")


def test_elevation_module_has_no_numpy_dependency():
    """Verify qgc4qgis.core.elevation has no dependency on numpy or WriteArray."""
    source_code = Path(elevation.__file__).read_text(encoding="utf-8")
    assert "numpy" not in source_code
    assert "WriteArray" not in source_code


def test_elevation_module_does_not_import_osgeo_on_load():
    """Verify qgc4qgis.core.elevation does not import osgeo or gdal at module level."""
    source_code = Path(elevation.__file__).read_text(encoding="utf-8")
    pattern = re.compile(r"^(from osgeo|import osgeo|import gdal|gdal\.UseExceptions)")
    for line in source_code.splitlines():
        assert not pattern.search(line)

    assert not re.search(r"\b(gdal|osr)\.UseExceptions\s*\(", source_code)
    assert "ExceptionMgr" not in source_code


def test_build_dem_geotiff_does_not_import_gdal_array(tmp_path):
    """Verify build_dem_geotiff does not load osgeo.gdal_array or numpy beyond the import baseline (newer QGIS images import numpy via qgis.core itself)."""
    repo_root = str(Path(__file__).resolve().parents[1])
    geotiff_path = str(tmp_path / "dem_test.tif")
    script = f"""import sys
sys.path.insert(0, {repo_root!r})
from qgc4qgis.core.elevation import CarpetTile, build_dem_geotiff

baseline = set(sys.modules)
tile = CarpetTile(0.0, 0.0, 0.01, 0.01, [[1.0, 2.0], [3.0, 4.0]])
build_dem_geotiff([tile], {geotiff_path!r})

novos = set(sys.modules) - baseline
print("osgeo.gdal_array" in sys.modules)
print("numpy" in novos)
"""
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        check=True,
    )
    assert Path(geotiff_path).exists(), (
        f"GeoTIFF file missing. stdout={result.stdout!r}, stderr={result.stderr!r}"
    )
    lines = [line.strip() for line in result.stdout.strip().splitlines() if line.strip()]
    assert len(lines) >= 2, (
        f"Expected at least 2 lines of output. stdout={result.stdout!r}, stderr={result.stderr!r}"
    )
    assert lines[0] == "False", (
        f"osgeo.gdal_array was imported! stdout={result.stdout!r}, stderr={result.stderr!r}"
    )
    assert lines[1] == "False", (
        f"numpy was imported! stdout={result.stdout!r}, stderr={result.stderr!r}"
    )


