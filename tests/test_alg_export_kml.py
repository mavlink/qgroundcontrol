"""End-to-end headless processing tests for ExportLitchiKmlAlgorithm via processing.run."""

import xml.etree.ElementTree as ET
from pathlib import Path

import processing
import pytest
from processing.core.Processing import Processing
from qgis.core import (
    QgsApplication,
    QgsFeature,
    QgsGeometry,
    QgsPointXY,
    QgsVectorLayer,
)

from qgc4qgis.core.cameracalc import CameraCalc
from qgc4qgis.core.cameras import load_cameras
from qgc4qgis.core.geo import AEQDProjection
from qgc4qgis.core.survey import generate_survey_transects
from qgc4qgis.processing.alg_export_kml import ExportLitchiKmlAlgorithm
from qgc4qgis.processing.alg_survey_grid import extract_polygons
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


def test_export_litchi_kml_processing_run_photo_mode(tmp_path):
    """Verify execution of ExportLitchiKmlAlgorithm in 'Por foto' mode on a simple polygon.

    Asserts that generated .kml has > (num_transects * 2) vertices (proving photo centers
    are used instead of just endpoints) and all coordinates have 3 components (lon, lat, alt).
    """
    input_layer = create_sample_polygon_layer()
    output_kml = str(tmp_path / "litchi_mission_photo_mode.kml")

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
        "TRIGGER_MODE": 2,  # Por foto
        "SPEED": 5.0,
        "OUTPUT": output_kml,
    }

    result = processing.run("qgc4qgis:exportar_litchi_kml", params)
    assert "OUTPUT" in result
    assert result["OUTPUT"] == output_kml

    kml_path = Path(output_kml)
    assert kml_path.exists()
    assert kml_path.stat().st_size > 0

    # Parse KML and extract coordinates from LineString
    tree = ET.parse(output_kml)
    root = tree.getroot()

    # Handle KML namespace http://www.opengis.net/kml/2.2
    ns = {"kml": "http://www.opengis.net/kml/2.2"}
    coords_elem = root.find(".//kml:coordinates", ns)
    if coords_elem is None or not coords_elem.text:
        coords_elem = root.find(".//coordinates")
    assert coords_elem is not None and coords_elem.text is not None

    raw_coords = coords_elem.text.strip().split()
    assert len(raw_coords) > 0

    # Verify all coordinates have three components (lon, lat, alt)
    vertices = []
    for item in raw_coords:
        parts = item.split(",")
        assert len(parts) == 3, f"Expected 3 coordinate components (lon,lat,alt), got: {item}"
        vertices.append((float(parts[0]), float(parts[1]), float(parts[2])))

    # Compute number of transects for this polygon using camera and grid parameters
    cameras = load_cameras()
    calc = CameraCalc(
        spec=cameras[0],
        value_set_is_distance=True,
        distance_to_surface=100.0,
        frontal_overlap=70.0,
        side_overlap=70.0,
    )
    calc.recalculate()

    feat = next(input_layer.getFeatures())
    polygons = extract_polygons(feat.geometry())
    poly_pts = polygons[0]
    ref_lat, ref_lon = poly_pts[0]
    proj = AEQDProjection(ref_lat, ref_lon)
    planar_pts = [proj.forward(lat, lon) for lat, lon in poly_pts]

    transects = generate_survey_transects(
        polygon_points=planar_pts,
        grid_angle=0.0,
        grid_spacing=calc.adjusted_footprint_side,
        entry_location=0,
        turnaround_distance=0.0,
        refly_90_degrees=False,
    )
    num_transects = len(transects)
    assert num_transects > 0

    # Assert vertex count > num_transects * 2 (proves photo centers are used, not just line endpoints)
    assert len(vertices) > num_transects * 2


def test_export_litchi_kml_takeoff_point_parameter_exists():
    """Verify that PONTO_DECOLAGEM parameter exists in ExportLitchiKmlAlgorithm and is optional."""
    alg = ExportLitchiKmlAlgorithm()
    alg.initAlgorithm()
    param = alg.parameterDefinition("PONTO_DECOLAGEM")
    assert param is not None
    assert bool(param.flags() & param.FlagOptional)
