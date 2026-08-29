"""End-to-end headless processing tests for ExportLitchiAlgorithm via processing.run."""

import csv
from pathlib import Path

import processing
import pytest
from processing.core.Processing import Processing
from qgis.core import (
    QgsApplication,
    QgsFeature,
    QgsGeometry,
    QgsPointXY,
    QgsProcessingException,
    QgsRasterLayer,
    QgsVectorLayer,
)

from qgc4qgis.processing.alg_export_litchi import ExportLitchiAlgorithm
from qgc4qgis.processing.provider import Qgc4QgisProvider


@pytest.fixture(scope="module", autouse=True)
def setup_processing(qgis_app):
    """Initialize QGIS Processing framework and register Qgc4QgisProvider for test module."""
    Processing.initialize()
    registry = QgsApplication.processingRegistry()
    provider = Qgc4QgisProvider()
    registry.addProvider(provider)
    yield provider
    registry.removeProvider(provider)


def create_sample_polygon_layer() -> QgsVectorLayer:
    """Create an in-memory WGS84 polygon layer (~111m x 111m square near lat=0, lon=0)."""
    layer = QgsVectorLayer("Polygon?crs=EPSG:4326", "poly_input", "memory")
    pr = layer.dataProvider()
    poly_pts = [
        QgsPointXY(0.0, 0.0),
        QgsPointXY(0.001, 0.0),
        QgsPointXY(0.001, 0.001),
        QgsPointXY(0.0, 0.001),
    ]
    feat = QgsFeature()
    feat.setGeometry(QgsGeometry.fromPolygonXY([poly_pts]))
    pr.addFeatures([feat])
    layer.updateExtents()
    return layer


def test_export_litchi_processing_run_end_to_end(tmp_path):
    """Verify end-to-end execution of Litchi export algorithm using processing.run.

    Tests memory layer input -> processing.run -> CSV file read back -> row count and
    coordinate containment verification within the polygon.
    """
    input_layer = create_sample_polygon_layer()
    output_csv = str(tmp_path / "litchi_mission_end_to_end.csv")

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 100.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 0.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "TRIGGER_MODE": 0,
        "SPEED": 5.0,
        "GIMBAL_PITCH": -90.0,
        "WAYPOINT_WAIT": 0.0,
        "OUTPUT": output_csv,
    }

    result = processing.run("qgc4qgis:exportar_litchi_csv", params)
    assert "OUTPUT" in result
    assert result["OUTPUT"] == output_csv

    csv_path = Path(output_csv)
    assert csv_path.exists()
    assert csv_path.stat().st_size > 0

    # Read back the CSV file and check headers, row count, and coordinates
    with open(output_csv, encoding="utf-8") as f:
        reader = csv.DictReader(f)
        headers = reader.fieldnames
        rows = list(reader)

    # Check headers
    expected_headers = [
        "latitude",
        "longitude",
        "altitude(m)",
        "heading(deg)",
        "curvesize(m)",
        "rotationdir",
        "gimbalmode",
        "gimbalpitchangle",
        "altitudemode",
        "speed(m/s)",
        "poi_latitude",
        "poi_longitude",
        "poi_altitude(m)",
        "poi_altitudemode",
        "photo_timeinterval",
        "photo_distinterval",
    ]
    for h in expected_headers:
        assert h in headers

    # Row count verification
    assert len(rows) > 0

    # Polygon geometry for containment checks
    input_feat = next(input_layer.getFeatures())
    poly_geom = input_feat.geometry()
    # Add small buffer tolerance (~1m in degrees, 1e-5) for boundary floating point precision
    buffered_poly = poly_geom.buffer(1e-5, 8)

    # Check waypoint coordinates inside polygon
    for row in rows:
        lat = float(row["latitude"])
        lon = float(row["longitude"])
        alt = float(row["altitude(m)"])

        assert alt == 100.0
        pt = QgsPointXY(lon, lat)
        assert buffered_poly.contains(QgsGeometry.fromPointXY(pt)), (
            f"Waypoint coordinate ({lat}, {lon}) is outside the polygon envelope"
        )


def test_export_litchi_processing_run_options(tmp_path):
    """Verify Litchi export processing.run with refly, waypoint wait, and turnaround."""
    input_layer = create_sample_polygon_layer()
    output_csv = str(tmp_path / "litchi_mission_options.csv")

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 120.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 10.0,
        "ENTRY_LOCATION": 0,
        "REFLY": True,
        "TRIGGER_MODE": 0,
        "SPEED": 6.0,
        "GIMBAL_PITCH": -90.0,
        "WAYPOINT_WAIT": 2.5,
        "OUTPUT": output_csv,
    }

    result = processing.run("qgc4qgis:exportar_litchi_csv", params)
    assert result["OUTPUT"] == output_csv

    with open(output_csv, encoding="utf-8") as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    assert len(rows) > 0
    first_row = rows[0]
    assert float(first_row["altitude(m)"]) == 120.0
    assert float(first_row["speed(m/s)"]) == 6.0
    assert first_row["actiontype1"] == "0"
    assert float(first_row["actionparam1"]) == 2500.0


def create_varying_dem_layer(tmp_path) -> QgsRasterLayer:
    """Create a synthetic WGS84 GeoTIFF DEM raster layer with varying elevation across (lat=0, lon=0)."""
    import numpy as np
    from osgeo import gdal, osr

    raster_file = str(tmp_path / "dem_varying_sample.tif")
    driver = gdal.GetDriverByName("GTiff")
    ds = driver.Create(raster_file, 10, 10, 1, gdal.GDT_Float32)
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)
    ds.SetProjection(srs.ExportToWkt())
    ds.SetGeoTransform([-0.01, 0.005, 0, 0.01, 0, -0.005])

    band = ds.GetRasterBand(1)
    band.SetNoDataValue(-9999.0)
    data = np.linspace(100.0, 200.0, 100, dtype=np.float32).reshape((10, 10))
    band.WriteArray(data)
    ds.FlushCache()
    del band
    ds = None

    layer = QgsRasterLayer(raster_file, "dem_varying_sample")
    assert layer.isValid()
    return layer


def test_export_litchi_takeoff_point_parameter_exists():
    """Verify that PONTO_DECOLAGEM parameter exists in ExportLitchiAlgorithm and is optional."""
    alg = ExportLitchiAlgorithm()
    alg.initAlgorithm()
    param = alg.parameterDefinition("PONTO_DECOLAGEM")
    assert param is not None
    assert bool(param.flags() & param.FlagOptional)


def test_export_litchi_with_elevation_layer_relative_altitudes(tmp_path):
    """Verify that exporting with elevation layer outputs altitudemode=0 and varying altitudes across waypoints."""
    input_layer = create_sample_polygon_layer()
    dem_layer = create_varying_dem_layer(tmp_path)
    output_csv = str(tmp_path / "litchi_mission_elevation.csv")

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 100.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 0.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "TRIGGER_MODE": 0,
        "SPEED": 5.0,
        "GIMBAL_PITCH": -90.0,
        "WAYPOINT_WAIT": 0.0,
        "ELEVATION_LAYER": dem_layer,
        "TOLERANCE": 10.0,
        "OUTPUT": output_csv,
    }

    result = processing.run("qgc4qgis:exportar_litchi_csv", params)
    assert result["OUTPUT"] == output_csv

    with open(output_csv, encoding="utf-8") as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    assert len(rows) > 0
    altitudes = []
    for row in rows:
        assert row["altitudemode"] == "0"
        altitudes.append(float(row["altitude(m)"]))

    # Confere que as alturas variam entre waypoints (o relevo continua sendo seguido)
    assert len(set(altitudes)) > 1, f"Expected varying altitudes across waypoints, got {altitudes}"


def test_export_litchi_takeoff_outside_dem_raises_exception(tmp_path):
    """Verify that taking off outside DEM extent raises QgsProcessingException."""
    input_layer = create_sample_polygon_layer()
    dem_layer = create_varying_dem_layer(tmp_path)
    output_csv = str(tmp_path / "litchi_mission_outside_dem.csv")

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 100.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 0.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "TRIGGER_MODE": 0,
        "SPEED": 5.0,
        "GIMBAL_PITCH": -90.0,
        "WAYPOINT_WAIT": 0.0,
        "ELEVATION_LAYER": dem_layer,
        "TOLERANCE": 10.0,
        "PONTO_DECOLAGEM": QgsPointXY(10.0, 10.0),
        "OUTPUT": output_csv,
    }

    with pytest.raises(
        QgsProcessingException, match="Could not sample elevation at the takeoff point"
    ):
        processing.run("qgc4qgis:exportar_litchi_csv", params)
