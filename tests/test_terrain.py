"""Tests for terrain sampling in qgc4qgis.core.terrain."""

import pytest
from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsPointXY,
    QgsProject,
    QgsRasterLayer,
)

from qgc4qgis.core.terrain import (
    TerrainPoint,
    adjust_for_max_rates,
    adjust_for_tolerance,
    adjust_terrain_flight_path,
    generate_interstitial_points,
    sample_terrain,
    sample_terrain_point,
)


@pytest.fixture
def dem_raster_utm(tmp_path):
    """Fixture providing a temporary 10x10 GTiff DEM raster in EPSG:32623 (UTM 23N).

    Raster bounds: x in [500000, 500100], y in [7999900, 8000000].
    Cell (0,0) (top-left, x=[500000, 500010], y=[7999990, 8000000]) contains NoData (-9999.0).
    All other cells contain 150.0m elevation.
    """
    import numpy as np
    from osgeo import gdal, osr

    raster_file = str(tmp_path / "dem_utm.tif")

    driver = gdal.GetDriverByName("GTiff")
    ds = driver.Create(raster_file, 10, 10, 1, gdal.GDT_Float32)
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(32623)
    ds.SetProjection(srs.ExportToWkt())
    ds.SetGeoTransform([500000, 10, 0, 8000000, 0, -10])

    band = ds.GetRasterBand(1)
    band.SetNoDataValue(-9999.0)

    data = np.full((10, 10), 150.0, dtype=np.float32)
    data[0, 0] = -9999.0
    band.WriteArray(data)
    ds.FlushCache()
    del band
    ds = None

    layer = QgsRasterLayer(raster_file, "dem_utm")
    assert layer.isValid()
    QgsProject.instance().addMapLayer(layer, False)
    yield layer
    QgsProject.instance().removeMapLayer(layer.id())


def test_sample_terrain_valid_point(dem_raster_utm):
    """Test sampling a valid point inside the raster extent."""
    raster_crs = dem_raster_utm.crs()
    wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")

    # Center of pixel (5, 5) in UTM -> x=500055, y=7999945
    ct_inv = QgsCoordinateTransform(raster_crs, wgs84, QgsProject.instance())
    pt_wgs = ct_inv.transform(QgsPointXY(500055, 7999945))
    lat_valid, lon_valid = pt_wgs.y(), pt_wgs.x()

    elevations = sample_terrain(dem_raster_utm, [(lat_valid, lon_valid)])
    assert len(elevations) == 1
    assert elevations[0] == pytest.approx(150.0)


def test_sample_terrain_nodata_and_out_of_bounds(dem_raster_utm):
    """Test sampling NoData pixel and out-of-bounds point."""
    raster_crs = dem_raster_utm.crs()
    wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")

    # Pixel (0,0) in UTM -> x=500005, y=7999995 (NoData)
    ct_inv = QgsCoordinateTransform(raster_crs, wgs84, QgsProject.instance())
    pt_nodata = ct_inv.transform(QgsPointXY(500005, 7999995))
    lat_nodata, lon_nodata = pt_nodata.y(), pt_nodata.x()

    # Out of bounds point
    lat_out, lon_out = 0.0, 0.0

    elevations = sample_terrain(
        dem_raster_utm,
        [(lat_nodata, lon_nodata), (lat_out, lon_out)],
        nodata_value=None,
    )
    assert elevations == [None, None]

    # Test with custom nodata value
    elevations_custom = sample_terrain(
        dem_raster_utm,
        [(lat_nodata, lon_nodata), (lat_out, lon_out)],
        nodata_value=-999.0,
    )
    assert elevations_custom == [-999.0, -999.0]


def test_sample_terrain_qgspointxy_and_tuples(dem_raster_utm):
    """Test sampling using QgsPointXY objects in raster's native CRS."""
    raster_crs = dem_raster_utm.crs()

    pt_valid = QgsPointXY(500055, 7999945)
    pt_nodata = QgsPointXY(500005, 7999995)
    pt_outside = QgsPointXY(100000, 100000)

    elevations = sample_terrain(
        dem_raster_utm,
        [pt_valid, pt_nodata, pt_outside],
        points_crs=raster_crs,
        nodata_value=-1.0,
    )
    assert elevations[0] == pytest.approx(150.0)
    assert elevations[1] == -1.0
    assert elevations[2] == -1.0


def test_sample_terrain_point_wrapper(dem_raster_utm):
    """Test sample_terrain_point single-point wrapper."""
    raster_crs = dem_raster_utm.crs()
    wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")

    ct_inv = QgsCoordinateTransform(raster_crs, wgs84, QgsProject.instance())
    pt_wgs = ct_inv.transform(QgsPointXY(500055, 7999945))

    val = sample_terrain_point(dem_raster_utm, (pt_wgs.y(), pt_wgs.x(), 100.0))
    assert val == pytest.approx(150.0)

    val_out = sample_terrain_point(dem_raster_utm, (0.0, 0.0))
    assert val_out is None


def test_sample_terrain_invalid_layer_and_empty_points():
    """Test graceful handling of invalid/None layer and empty points list."""
    assert sample_terrain(None, [(0.0, 0.0)]) == [None]

    layer_invalid = QgsRasterLayer("non_existent_file.tif", "invalid")
    assert sample_terrain(layer_invalid, [(0.0, 0.0)], nodata_value=-9999.0) == [-9999.0]

    assert sample_terrain(None, []) == []


def test_generate_interstitial_points_without_dem():
    """Test generating interstitial points along a segment without DEM (linear fallback)."""
    # 2 points separated by ~100m along latitude (0.0009 degrees lat ~= 100m)
    p1 = (0.0, 0.0, 50.0)
    p2 = (0.0009, 0.0, 100.0)

    pts = generate_interstitial_points([p1, p2], target_altitude=50.0, step_distance=20.0)
    assert len(pts) > 2
    assert not pts[0].is_interstitial
    assert not pts[-1].is_interstitial
    assert all(p.is_interstitial for p in pts[1:-1])

    # Check altitude linear interpolation fallback
    assert pts[0].altitude == pytest.approx(50.0)
    assert pts[-1].altitude == pytest.approx(100.0)


def test_generate_interstitial_points_with_dem(dem_raster_utm):
    """Test generating interstitial points with DEM raster sampling."""
    raster_crs = dem_raster_utm.crs()
    wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")
    ct_inv = QgsCoordinateTransform(raster_crs, wgs84, QgsProject.instance())

    pt1_wgs = ct_inv.transform(QgsPointXY(500020, 7999950))
    pt2_wgs = ct_inv.transform(QgsPointXY(500080, 7999950))

    p1 = (pt1_wgs.y(), pt1_wgs.x())
    p2 = (pt2_wgs.y(), pt2_wgs.x())

    pts = generate_interstitial_points(
        [p1, p2],
        raster_layer=dem_raster_utm,
        target_altitude=50.0,
        step_distance=15.0,
        points_crs="EPSG:4326",
    )
    assert len(pts) >= 4
    # All sampled cells in dem_raster_utm have 150.0m elevation, so altitude should be 150 + 50 = 200m
    for pt in pts:
        assert pt.altitude == pytest.approx(200.0)


def test_adjust_for_tolerance():
    """Test decimation of interstitial points by altitude tolerance (TerrainAdjustTolerance)."""
    # Create endpoints and interstitial points with small and large altitude variations
    pts = [
        TerrainPoint(0.0, 0.0, 100.0, is_interstitial=False),  # Keep (endpoint)
        TerrainPoint(0.0001, 0.0, 105.0, is_interstitial=True),  # diff 5m <= 10m -> drop
        TerrainPoint(0.0002, 0.0, 108.0, is_interstitial=True),  # diff 8m relative to 100m -> drop
        TerrainPoint(0.0003, 0.0, 112.0, is_interstitial=True),  # diff 12m > 10m -> KEEP (alt=112)
        TerrainPoint(0.0004, 0.0, 115.0, is_interstitial=True),  # diff 3m relative to 112m -> drop
        TerrainPoint(0.0005, 0.0, 120.0, is_interstitial=False),  # Keep (endpoint)
    ]

    decimated = adjust_for_tolerance(pts, tolerance=10.0)
    assert len(decimated) == 3
    assert decimated[0].altitude == 100.0
    assert decimated[1].altitude == 112.0
    assert decimated[2].altitude == 120.0


def test_adjust_for_max_rates_climb_and_descent():
    """Test max climb and descent rate limits with 0.1 m/s slack."""
    # 2 points separated by ~100m (lat 0.0 -> 0.0009, dist ~= 100m)
    # At flight_speed = 10 m/s, time = 10 s.
    # Max climb rate = 2.0 m/s -> max altitude increase = 20m.
    # If endpoint target alt = 150m and start alt = 100m (diff = 50m, climb rate = 5 m/s > 2 m/s + 0.1):
    # start_pt altitude will be raised to 150 - 20 = 130m.

    start_pt = TerrainPoint(0.0, 0.0, 100.0, is_interstitial=False)
    end_pt = TerrainPoint(0.0009, 0.0, 150.0, is_interstitial=False)

    adjusted_climb = adjust_for_max_rates(
        [start_pt, end_pt],
        max_climb_rate=2.0,
        max_descent_rate=0.0,
        flight_speed=10.0,
    )
    assert len(adjusted_climb) == 2
    assert adjusted_climb[1].altitude == 150.0
    assert adjusted_climb[0].altitude == pytest.approx(130.0, abs=1.0)

    # Max descent rate = 2.0 m/s -> max altitude decrease = 20m.
    # If start alt = 150m and end alt = 100m (diff = -50m, descent rate = -5 m/s < -2 m/s - 0.1):
    # end_pt altitude will be raised to 150 - 20 = 130m.
    start_pt_d = TerrainPoint(0.0, 0.0, 150.0, is_interstitial=False)
    end_pt_d = TerrainPoint(0.0009, 0.0, 100.0, is_interstitial=False)

    adjusted_descent = adjust_for_max_rates(
        [start_pt_d, end_pt_d],
        max_climb_rate=0.0,
        max_descent_rate=2.0,
        flight_speed=10.0,
    )
    assert len(adjusted_descent) == 2
    assert adjusted_descent[0].altitude == 150.0
    assert adjusted_descent[1].altitude == pytest.approx(130.0, abs=1.0)


def test_adjust_terrain_flight_path_full_pipeline():
    """Test composite adjust_terrain_flight_path function."""
    p1 = (0.0, 0.0, 50.0)
    p2 = (0.0009, 0.0, 100.0)

    res = adjust_terrain_flight_path(
        points=[p1, p2],
        target_altitude=50.0,
        step_distance=20.0,
        tolerance=10.0,
        max_climb_rate=2.0,
        max_descent_rate=2.0,
        flight_speed=10.0,
    )
    assert len(res) >= 2
    assert isinstance(res[0], TerrainPoint)


@pytest.fixture
def dem_ramp_raster(tmp_path):
    """Fixture providing a temporary 10x10 GTiff DEM raster with a known linear elevation ramp in EPSG:32623.

    Raster bounds: x in [500000, 500100], y in [7999900, 8000000].
    Cell size: 10m x 10m.
    Ramp formula: elevation at col `c` (0 <= c < 10) is 100.0 + c * 10.0 meters.
    Column 0 center (x=500005) -> 100.0 m
    Column 1 center (x=500015) -> 110.0 m
    ...
    Column 9 center (x=500095) -> 190.0 m
    """
    import numpy as np
    from osgeo import gdal, osr

    raster_file = str(tmp_path / "dem_ramp.tif")
    driver = gdal.GetDriverByName("GTiff")
    ds = driver.Create(raster_file, 10, 10, 1, gdal.GDT_Float32)
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(32623)
    ds.SetProjection(srs.ExportToWkt())
    ds.SetGeoTransform([500000, 10, 0, 8000000, 0, -10])

    band = ds.GetRasterBand(1)
    band.SetNoDataValue(-9999.0)

    data = np.zeros((10, 10), dtype=np.float32)
    for col in range(10):
        data[:, col] = 100.0 + col * 10.0

    band.WriteArray(data)
    ds.FlushCache()
    del band
    ds = None

    layer = QgsRasterLayer(raster_file, "dem_ramp")
    assert layer.isValid()
    QgsProject.instance().addMapLayer(layer, False)
    yield layer
    QgsProject.instance().removeMapLayer(layer.id())


def test_sample_terrain_synthetic_known_ramp(dem_ramp_raster):
    """Test sampling terrain elevation across a synthetic raster with a known linear ramp."""
    raster_crs = dem_ramp_raster.crs()

    # Sample centers of columns 0, 2, 5, 9 in native CRS QgsPointXY
    points = [
        QgsPointXY(500005, 7999950),  # col 0 -> expected 100.0m
        QgsPointXY(500025, 7999950),  # col 2 -> expected 120.0m
        QgsPointXY(500055, 7999950),  # col 5 -> expected 150.0m
        QgsPointXY(500095, 7999950),  # col 9 -> expected 190.0m
    ]

    elevations = sample_terrain(
        dem_ramp_raster,
        points,
        points_crs=raster_crs,
    )
    assert len(elevations) == 4
    assert elevations[0] == pytest.approx(100.0)
    assert elevations[1] == pytest.approx(120.0)
    assert elevations[2] == pytest.approx(150.0)
    assert elevations[3] == pytest.approx(190.0)


def test_generate_interstitial_points_synthetic_ramp(dem_ramp_raster):
    """Test sampling elevation and generating interstitial points over a synthetic known ramp."""
    raster_crs = dem_ramp_raster.crs()
    wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")
    ct_inv = QgsCoordinateTransform(raster_crs, wgs84, QgsProject.instance())

    # Path from col 1 center (500015, 7999950) to col 7 center (500075, 7999950)
    pt1_wgs = ct_inv.transform(QgsPointXY(500015, 7999950))
    pt2_wgs = ct_inv.transform(QgsPointXY(500075, 7999950))

    p1 = (pt1_wgs.y(), pt1_wgs.x())
    p2 = (pt2_wgs.y(), pt2_wgs.x())

    target_alt = 50.0
    pts = generate_interstitial_points(
        [p1, p2],
        raster_layer=dem_ramp_raster,
        target_altitude=target_alt,
        step_distance=10.0,
        points_crs="EPSG:4326",
    )

    assert len(pts) >= 6
    # Start point elevation is 110.0 + 50.0 = 160.0
    assert pts[0].altitude == pytest.approx(160.0, abs=0.5)
    # End point elevation is 170.0 + 50.0 = 220.0
    assert pts[-1].altitude == pytest.approx(220.0, abs=0.5)

    # Verify that altitudes along the ramp are monotonically non-decreasing
    for i in range(len(pts) - 1):
        assert pts[i + 1].altitude >= pts[i].altitude
    assert pts[-1].altitude > pts[0].altitude
