"""Unit tests for QgsProject parameter persistence functions."""

import pytest
from qgis.core import QgsFeature, QgsField, QgsGeometry, QgsPointXY, QgsProject, QgsVectorLayer
from qgis.PyQt.QtCore import QVariant

from qgc4qgis.core.settings import (
    DEFAULT_SETTINGS,
    clear_project_settings,
    load_project_settings,
    save_project_settings,
)
from qgc4qgis.gui.dock import QgcPlanningDockWidget


@pytest.fixture(autouse=True)
def cleanup_project_settings():
    """Ensure clean QgsProject settings state before and after each test."""
    clear_project_settings()
    yield
    clear_project_settings()


def create_test_polygon_layer() -> QgsVectorLayer:
    """Create in-memory vector polygon layer for test cases."""
    layer = QgsVectorLayer("Polygon?crs=EPSG:4326", "Settings Test Area", "memory")
    provider = layer.dataProvider()
    provider.addAttributes([QgsField("name", QVariant.String)])
    layer.updateFields()

    feat = QgsFeature(layer.fields())
    feat.setAttribute("name", "Zone A")
    poly = [
        QgsPointXY(-47.0, -22.0),
        QgsPointXY(-46.99, -22.0),
        QgsPointXY(-46.99, -21.99),
        QgsPointXY(-47.0, -21.99),
        QgsPointXY(-47.0, -22.0),
    ]
    feat.setGeometry(QgsGeometry.fromPolygonXY([poly]))
    provider.addFeature(feat)
    layer.updateExtents()
    return layer


def test_load_settings_defaults(qgis_app):
    """Test loading parameters from an uninitialized QgsProject returns DEFAULT_SETTINGS."""
    project = QgsProject()
    loaded = load_project_settings(project)
    assert loaded == DEFAULT_SETTINGS


def test_save_and_load_settings_round_trip(qgis_app):
    """Test round-trip saving and loading of all parameters to QgsProject."""
    project = QgsProject()

    params_to_save = {
        "LAYER_ID": "polygon_layer_999",
        "FEATURE_ID": 42,
        "CAMERA": 1,
        "ALTITUDE": 150.0,
        "GSD": 3.5,
        "MODE_ALTITUDE": False,
        "OVERLAP_SIDE": 80.0,
        "OVERLAP_FRONTAL": 85.0,
        "ANGLE": 90.0,
        "TURNAROUND": 15.0,
        "ENTRY_LOCATION": 3,
        "REFLY": True,
        "SENSOR_WIDTH": 23.5,
        "SENSOR_HEIGHT": 15.6,
        "IMAGE_WIDTH": 6000,
        "IMAGE_HEIGHT": 4000,
        "FOCAL_LENGTH": 50.0,
        "TRIGGER_MODE": 1,
        "SPEED": 7.5,
        "GIMBAL_PITCH": -45.0,
        "WAYPOINT_WAIT": 2.5,
    }

    save_project_settings(params_to_save, project)
    loaded = load_project_settings(project)

    assert loaded["LAYER_ID"] == "polygon_layer_999"
    assert loaded["FEATURE_ID"] == 42
    assert loaded["CAMERA"] == 1
    assert loaded["ALTITUDE"] == 150.0
    assert loaded["GSD"] == 3.5
    assert loaded["MODE_ALTITUDE"] is False
    assert loaded["OVERLAP_SIDE"] == 80.0
    assert loaded["OVERLAP_FRONTAL"] == 85.0
    assert loaded["ANGLE"] == 90.0
    assert loaded["TURNAROUND"] == 15.0
    assert loaded["ENTRY_LOCATION"] == 3
    assert loaded["REFLY"] is True
    assert loaded["SENSOR_WIDTH"] == 23.5
    assert loaded["SENSOR_HEIGHT"] == 15.6
    assert loaded["IMAGE_WIDTH"] == 6000
    assert loaded["IMAGE_HEIGHT"] == 4000
    assert loaded["FOCAL_LENGTH"] == 50.0
    assert loaded["TRIGGER_MODE"] == 1
    assert loaded["SPEED"] == 7.5
    assert loaded["GIMBAL_PITCH"] == -45.0
    assert loaded["WAYPOINT_WAIT"] == 2.5


def test_save_and_load_none_feature_id(qgis_app):
    """Test round-trip when FEATURE_ID is None (all features selected)."""
    project = QgsProject()
    params = {"FEATURE_ID": None}
    save_project_settings(params, project)

    loaded = load_project_settings(project)
    assert loaded["FEATURE_ID"] is None


def test_dock_settings_persistence(qgis_app):
    """Test that QgcPlanningDockWidget reloads settings from QgsProject on creation."""
    layer = create_test_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    # 1. Open dock widget and set custom values
    dock1 = QgcPlanningDockWidget()
    dock1.cmb_layer.setLayer(layer)
    dock1.spn_altitude.setValue(200.0)
    dock1.spn_overlap_side.setValue(85.0)
    dock1.spn_angle.setValue(30.0)
    dock1.chk_refly.setChecked(True)
    dock1.save_settings()
    dock1.unload()

    # 2. Open a new dock widget instance and verify values are reloaded from QgsProject
    dock2 = QgcPlanningDockWidget()
    assert dock2.spn_altitude.value() == 200.0
    assert dock2.spn_overlap_side.value() == 85.0
    assert dock2.spn_angle.value() == 30.0
    assert dock2.chk_refly.isChecked()
    dock2.unload()

    QgsProject.instance().removeMapLayer(layer)
