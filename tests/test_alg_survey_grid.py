"""End-to-end headless processing tests for SurveyGridAlgorithm via processing.run."""

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


def test_survey_grid_processing_run_end_to_end():
    """Verify end-to-end execution of survey grid algorithm using processing.run."""
    input_layer = create_sample_polygon_layer()

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 100.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 10.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "OUTPUT": "memory:",
    }

    result = processing.run("qgc4qgis:gerar_grade_voo", params)
    assert "OUTPUT" in result

    out_layer = result["OUTPUT"]
    assert isinstance(out_layer, QgsVectorLayer)
    assert out_layer.isValid()

    # Check feature count
    feature_count = out_layer.featureCount()
    assert feature_count > 0

    # Check total length calculation
    features = list(out_layer.getFeatures())
    assert len(features) == feature_count

    total_length_m = sum(f["length_m"] for f in features)
    assert total_length_m > 0.0

    # Verify attribute fields and expected values on first transect feature
    first_feat = features[0]
    expected_fields = [
        "id",
        "length_m",
        "altitude_m",
        "gsd_cm",
        "side_overlap",
        "frontal_overlap",
        "trigger_dist_m",
        "spacing_m",
        "camera",
        "photo_count",
    ]
    field_names = [field.name() for field in out_layer.fields()]
    for field in expected_fields:
        assert field in field_names

    assert first_feat["altitude_m"] == 100.0
    assert first_feat["side_overlap"] == 70.0
    assert first_feat["frontal_overlap"] == 70.0
    assert first_feat["length_m"] > 0.0
    assert first_feat["photo_count"] > 0

    # Verify geometry validity
    for feat in features:
        geom = feat.geometry()
        assert not geom.isEmpty()
        assert geom.isGeosValid()


def test_survey_grid_processing_run_refly():
    """Verify that enabling refly (cross grid) increases feature count and total length."""
    input_layer = create_sample_polygon_layer()

    base_params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 100.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 10.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "OUTPUT": "memory:",
    }

    res_normal = processing.run("qgc4qgis:gerar_grade_voo", base_params)
    out_normal = res_normal["OUTPUT"]
    count_normal = out_normal.featureCount()
    len_normal = sum(f["length_m"] for f in out_normal.getFeatures())

    refly_params = dict(base_params)
    refly_params["REFLY"] = True

    res_refly = processing.run("qgc4qgis:gerar_grade_voo", refly_params)
    out_refly = res_refly["OUTPUT"]
    count_refly = out_refly.featureCount()
    len_refly = sum(f["length_m"] for f in out_refly.getFeatures())

    assert count_refly > count_normal
    assert len_refly > len_normal


def test_survey_grid_processing_run_gsd_mode():
    """Verify algorithm execution when GSD mode is active (altitude = 0, gsd > 0)."""
    input_layer = create_sample_polygon_layer()

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 0.0,
        "GSD": 2.0,
        "OVERLAP_SIDE": 70.0,
        "OVERLAP_FRONTAL": 70.0,
        "ANGLE": 0.0,
        "TURNAROUND": 5.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "OUTPUT": "memory:",
    }

    result = processing.run("qgc4qgis:gerar_grade_voo", params)
    out_layer = result["OUTPUT"]
    features = list(out_layer.getFeatures())

    assert len(features) > 0
    first_feat = features[0]
    assert first_feat["gsd_cm"] == 2.0
    assert first_feat["altitude_m"] > 0.0
    assert sum(f["length_m"] for f in features) > 0.0


def test_survey_grid_processing_run_custom_camera():
    """Verify algorithm execution with custom camera specifications."""
    input_layer = create_sample_polygon_layer()

    params = {
        "INPUT": input_layer,
        "CAMERA": 0,
        "ALTITUDE": 120.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 75.0,
        "OVERLAP_FRONTAL": 80.0,
        "ANGLE": 45.0,
        "TURNAROUND": 15.0,
        "ENTRY_LOCATION": 1,
        "REFLY": False,
        "SENSOR_WIDTH": 36.0,
        "SENSOR_HEIGHT": 24.0,
        "IMAGE_WIDTH": 6000,
        "IMAGE_HEIGHT": 4000,
        "FOCAL_LENGTH": 25.0,
        "OUTPUT": "memory:",
    }

    result = processing.run("qgc4qgis:gerar_grade_voo", params)
    out_layer = result["OUTPUT"]
    features = list(out_layer.getFeatures())

    assert len(features) > 0
    assert sum(f["length_m"] for f in features) > 0.0
    assert features[0]["altitude_m"] == 120.0
