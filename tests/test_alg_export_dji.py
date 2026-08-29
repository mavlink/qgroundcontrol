"""End-to-end headless processing tests for ExportDjiAlgorithm via processing.run."""

import xml.etree.ElementTree as ET
import zipfile
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

from qgc4qgis.core.wpml import KML_NS, WPML_NS
from qgc4qgis.processing.alg_export_dji import ExportDjiAlgorithm
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


def test_export_dji_processing_run_end_to_end(tmp_path):
    """Verify end-to-end execution of DJI export algorithm using processing.run.

    Tests memory layer input -> processing.run -> KMZ output on disk -> ZIP contents
    and XML structure/namespaces verification.
    """
    input_layer = create_sample_polygon_layer()
    output_kmz = str(tmp_path / "dji_mission_end_to_end.kmz")

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
        "FINISH_ACTION": 0,
        "RC_LOST_ACTION": 0,
        "TRANSITIONAL_SPEED": 5.0,
        "ZIP_LAYOUT": 0,  # wpmz/ subfolder
        "OUTPUT": output_kmz,
    }

    result = processing.run("qgc4qgis:exportar_dji_kmz", params)
    assert "OUTPUT" in result
    assert result["OUTPUT"] == output_kmz

    kmz_path = Path(output_kmz)
    assert kmz_path.exists()
    assert kmz_path.stat().st_size > 0

    # Open ZIP and verify expected internal structure
    with zipfile.ZipFile(kmz_path, "r") as zf:
        namelist = zf.namelist()
        assert "wpmz/template.kml" in namelist
        assert "wpmz/waylines.wpml" in namelist

        template_bytes = zf.read("wpmz/template.kml")
        waylines_bytes = zf.read("wpmz/waylines.wpml")

    # Validate template.kml XML
    template_root = ET.fromstring(template_bytes)
    assert template_root.tag == f"{{{KML_NS}}}kml"
    template_doc = template_root.find(f"{{{KML_NS}}}Document")
    assert template_doc is not None
    assert template_doc.find(f"{{{WPML_NS}}}author") is not None
    assert template_doc.find(f"{{{WPML_NS}}}missionConfig") is not None

    # Validate waylines.wpml XML
    waylines_root = ET.fromstring(waylines_bytes)
    assert waylines_root.tag == f"{{{KML_NS}}}kml"
    waylines_doc = waylines_root.find(f"{{{KML_NS}}}Document")
    assert waylines_doc is not None

    folder = waylines_doc.find(f"{{{KML_NS}}}Folder")
    assert folder is not None
    assert folder.find(f"{{{WPML_NS}}}templateId") is not None
    assert folder.find(f"{{{WPML_NS}}}executeHeightMode") is not None

    placemarks = folder.findall(f"{{{KML_NS}}}Placemark")
    assert len(placemarks) > 0

    # Verify waypoint placemarks
    for pm in placemarks:
        idx_elem = pm.find(f"{{{WPML_NS}}}index")
        assert idx_elem is not None
        coords_elem = pm.find(f"{{{KML_NS}}}Point/{f'{{{KML_NS}}}coordinates'}")
        assert coords_elem is not None
        height_elem = pm.find(f"{{{WPML_NS}}}executeHeight")
        assert height_elem is not None
        assert float(height_elem.text) == 100.0


def test_export_dji_processing_run_root_zip_layout(tmp_path):
    """Verify DJI export processing.run with root ZIP layout option."""
    input_layer = create_sample_polygon_layer()
    output_kmz = str(tmp_path / "dji_mission_root_layout.kmz")

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 80.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 75.0,
        "OVERLAP_FRONTAL": 75.0,
        "ANGLE": 45.0,
        "TURNAROUND": 5.0,
        "ENTRY_LOCATION": 1,
        "REFLY": False,
        "TRIGGER_MODE": 0,
        "SPEED": 6.0,
        "GIMBAL_PITCH": -90.0,
        "WAYPOINT_WAIT": 0.0,
        "FINISH_ACTION": 0,
        "RC_LOST_ACTION": 0,
        "TRANSITIONAL_SPEED": 5.0,
        "ZIP_LAYOUT": 1,  # Root layout
        "OUTPUT": output_kmz,
    }

    result = processing.run("qgc4qgis:exportar_dji_kmz", params)
    assert result["OUTPUT"] == output_kmz

    kmz_path = Path(output_kmz)
    assert kmz_path.exists()
    assert kmz_path.stat().st_size > 0

    with zipfile.ZipFile(kmz_path, "r") as zf:
        namelist = zf.namelist()
        assert "template.kml" in namelist
        assert "waylines.wpml" in namelist
        assert "wpmz/template.kml" not in namelist

        template_bytes = zf.read("template.kml")
        waylines_bytes = zf.read("waylines.wpml")

    t_root = ET.fromstring(template_bytes)
    w_root = ET.fromstring(waylines_bytes)
    assert t_root.tag == f"{{{KML_NS}}}kml"
    assert w_root.tag == f"{{{KML_NS}}}kml"


def test_export_dji_processing_run_custom_options(tmp_path):
    """Verify DJI export processing.run with custom options (refly, finish action, waypoint wait)."""
    input_layer = create_sample_polygon_layer()
    output_kmz = str(tmp_path / "dji_mission_custom_options.kmz")

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
        "TRIGGER_MODE": 1,  # POR_TEMPO
        "SPEED": 7.0,
        "GIMBAL_PITCH": -45.0,
        "WAYPOINT_WAIT": 3.0,
        "FINISH_ACTION": 1,  # noAction
        "RC_LOST_ACTION": 1,  # landing
        "TRANSITIONAL_SPEED": 8.0,
        "ZIP_LAYOUT": 0,
        "OUTPUT": output_kmz,
    }

    result = processing.run("qgc4qgis:exportar_dji_kmz", params)
    assert result["OUTPUT"] == output_kmz

    kmz_path = Path(output_kmz)
    assert kmz_path.exists()

    with zipfile.ZipFile(kmz_path, "r") as zf:
        template_bytes = zf.read("wpmz/template.kml")
        waylines_bytes = zf.read("wpmz/waylines.wpml")

    t_root = ET.fromstring(template_bytes)
    mc = t_root.find(f"{{{KML_NS}}}Document/{f'{{{WPML_NS}}}missionConfig'}")
    assert mc is not None
    assert mc.find(f"{{{WPML_NS}}}finishAction").text == "noAction"
    assert mc.find(f"{{{WPML_NS}}}executeRCLostAction").text == "landing"
    assert mc.find(f"{{{WPML_NS}}}globalTransitionalSpeed").text == "8.0"

    w_root = ET.fromstring(waylines_bytes)
    folder = w_root.find(f"{{{KML_NS}}}Document/{f'{{{KML_NS}}}Folder'}")
    assert folder is not None
    placemarks = folder.findall(f"{{{KML_NS}}}Placemark")
    assert len(placemarks) > 0


def create_sample_dem_layer(tmp_path) -> QgsRasterLayer:
    """Create a synthetic WGS84 GeoTIFF DEM raster layer covering (lat=0, lon=0)."""
    import numpy as np
    from osgeo import gdal, osr

    raster_file = str(tmp_path / "dem_sample.tif")
    driver = gdal.GetDriverByName("GTiff")
    ds = driver.Create(raster_file, 10, 10, 1, gdal.GDT_Float32)
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)
    ds.SetProjection(srs.ExportToWkt())
    ds.SetGeoTransform([-0.01, 0.005, 0, 0.01, 0, -0.005])

    band = ds.GetRasterBand(1)
    band.SetNoDataValue(-9999.0)
    data = np.full((10, 10), 100.0, dtype=np.float32)
    band.WriteArray(data)
    ds.FlushCache()
    del band
    ds = None

    layer = QgsRasterLayer(raster_file, "dem_sample")
    assert layer.isValid()
    return layer


def test_export_dji_takeoff_point_parameter_exists():
    """Verify that PONTO_DECOLAGEM parameter exists in ExportDjiAlgorithm and is optional."""
    alg = ExportDjiAlgorithm()
    alg.initAlgorithm()
    param = alg.parameterDefinition("PONTO_DECOLAGEM")
    assert param is not None
    assert bool(param.flags() & param.FlagOptional)


def test_export_dji_with_elevation_layer_succeeds(tmp_path):
    """Verify that exporting with an elevation layer does not raise D10 error and produces valid KMZ."""
    input_layer = create_sample_polygon_layer()
    dem_layer = create_sample_dem_layer(tmp_path)
    output_kmz = str(tmp_path / "dji_mission_terrain.kmz")

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
        "FINISH_ACTION": 0,
        "RC_LOST_ACTION": 0,
        "TRANSITIONAL_SPEED": 5.0,
        "ZIP_LAYOUT": 0,
        "ELEVATION_LAYER": dem_layer,
        "TOLERANCE": 10.0,
        "PONTO_DECOLAGEM": QgsPointXY(0.0005, 0.0005),
        "OUTPUT": output_kmz,
    }

    result = processing.run("qgc4qgis:exportar_dji_kmz", params)
    assert result["OUTPUT"] == output_kmz

    kmz_path = Path(output_kmz)
    assert kmz_path.exists()
    assert kmz_path.stat().st_size > 0

    with zipfile.ZipFile(kmz_path, "r") as zf:
        namelist = zf.namelist()
        assert "wpmz/template.kml" in namelist
        assert "wpmz/waylines.wpml" in namelist

        template_bytes = zf.read("wpmz/template.kml")
        waylines_bytes = zf.read("wpmz/waylines.wpml")

    template_root = ET.fromstring(template_bytes)
    assert template_root.tag == f"{{{KML_NS}}}kml"

    waylines_root = ET.fromstring(waylines_bytes)
    assert waylines_root.tag == f"{{{KML_NS}}}kml"
    folder = waylines_root.find(f"{{{KML_NS}}}Document/{f'{{{KML_NS}}}Folder'}")
    assert folder is not None
    placemarks = folder.findall(f"{{{KML_NS}}}Placemark")
    assert len(placemarks) > 0


def test_export_dji_takeoff_outside_dem_raises_exception(tmp_path):
    """Verify that taking off outside DEM extent raises QgsProcessingException."""
    input_layer = create_sample_polygon_layer()
    dem_layer = create_sample_dem_layer(tmp_path)
    output_kmz = str(tmp_path / "dji_mission_outside_dem.kmz")

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
        "FINISH_ACTION": 0,
        "RC_LOST_ACTION": 0,
        "TRANSITIONAL_SPEED": 5.0,
        "ZIP_LAYOUT": 0,
        "ELEVATION_LAYER": dem_layer,
        "TOLERANCE": 10.0,
        "PONTO_DECOLAGEM": QgsPointXY(10.0, 10.0),
        "OUTPUT": output_kmz,
    }

    with pytest.raises(
        QgsProcessingException, match="Could not sample elevation at the takeoff point"
    ):
        processing.run("qgc4qgis:exportar_dji_kmz", params)
