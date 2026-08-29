"""Unit tests for flight preview manager and dock preview integration."""

from qgis.core import (
    QgsFeature,
    QgsField,
    QgsGeometry,
    QgsPointXY,
    QgsProject,
    QgsVectorLayer,
)
from qgis.PyQt.QtCore import QVariant

from qgc4qgis.gui.dock import QgcPlanningDockWidget
from qgc4qgis.gui.preview import FlightPreviewManager


def create_sample_polygon_layer() -> QgsVectorLayer:
    """Create in-memory vector polygon layer for test cases."""
    layer = QgsVectorLayer("Polygon?crs=EPSG:4326", "Test Polygons", "memory")
    provider = layer.dataProvider()
    provider.addAttributes([QgsField("name", QVariant.String)])
    layer.updateFields()

    feat = QgsFeature(layer.fields())
    feat.setAttribute("name", "Area 1")
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


def test_preview_manager_initialization_and_clear(qgis_app):
    """Test FlightPreviewManager initialization and empty clear call."""
    pm = FlightPreviewManager()
    assert pm.area_layer is None
    assert pm.line_layer is None

    # Clearing without active layers should execute safely
    pm.clear()
    assert pm.area_layer is None
    assert pm.line_layer is None


def test_preview_manager_update_and_removal(qgis_app):
    """Test memory preview layers creation and removal from QgsProject."""
    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    pm = FlightPreviewManager()
    params = {
        "INPUT": layer,
        "FEATURE_ID": None,
        "CAMERA": 0,
        "ALTITUDE": 120.0,
        "GSD": 0.0,
        "OVERLAP_SIDE": 75.0,
        "OVERLAP_FRONTAL": 75.0,
        "ANGLE": 30.0,
        "TURNAROUND": 5.0,
        "ENTRY_LOCATION": 0,
        "REFLY": False,
        "SENSOR_WIDTH": 35.9,
        "SENSOR_HEIGHT": 24.0,
        "IMAGE_WIDTH": 7952,
        "IMAGE_HEIGHT": 5304,
        "FOCAL_LENGTH": 35.0,
    }

    pm.update_preview(params)

    assert pm.area_layer is not None
    assert pm.line_layer is not None
    assert pm.area_layer.featureCount() == 1
    assert pm.line_layer.featureCount() > 0

    area_layer_id = pm.area_layer.id()
    line_layer_id = pm.line_layer.id()

    assert QgsProject.instance().mapLayer(area_layer_id) is not None
    assert QgsProject.instance().mapLayer(line_layer_id) is not None

    # Clear should remove both memory layers from QgsProject
    pm.clear()
    assert pm.area_layer is None
    assert pm.line_layer is None
    assert QgsProject.instance().mapLayer(area_layer_id) is None
    assert QgsProject.instance().mapLayer(line_layer_id) is None

    QgsProject.instance().removeMapLayer(layer)


def test_dock_preview_recreation_on_param_change_and_unload(qgis_app):
    """Test preview recreation on dock parameter changes and removal on dock unload."""
    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)
    dock._on_layer_changed()

    pm = dock.preview_manager
    assert pm.area_layer is not None
    assert pm.line_layer is not None

    assert pm.line_layer.featureCount() > 0
    old_line_layer_id = pm.line_layer.id()

    # Changing angle should recreate preview layers
    dock.spn_angle.setValue(90.0)

    assert pm.area_layer is not None
    assert pm.line_layer is not None
    # Recreated layer should have a new ID
    new_line_layer_id = pm.line_layer.id()
    assert new_line_layer_id != old_line_layer_id
    assert QgsProject.instance().mapLayer(new_line_layer_id) is not None

    # Unload dock widget -> preview layers removed from project
    dock.unload()
    assert pm.area_layer is None
    assert pm.line_layer is None
    assert QgsProject.instance().mapLayer(new_line_layer_id) is None

    QgsProject.instance().removeMapLayer(layer)


def test_preview_manager_algorithm_exception_handling(qgis_app, monkeypatch):
    """Test that algorithm exception in preview manager calls log_warning."""
    from unittest.mock import patch

    from qgc4qgis.processing.alg_survey_grid import SurveyGridAlgorithm

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    pm = FlightPreviewManager()
    params = {"INPUT": layer}

    def mock_process_algorithm(self, params, context, feedback):
        raise RuntimeError("Algorithm failure simulation")

    monkeypatch.setattr(SurveyGridAlgorithm, "processAlgorithm", mock_process_algorithm)

    with patch("qgc4qgis.gui.preview.log_warning") as mock_log:
        pm.update_preview(params)
        mock_log.assert_called_once()
        assert (
            "Erro ao gerar pré-visualização de linhas de voo: Algorithm failure simulation"
            in mock_log.call_args[0][0]
        )

    QgsProject.instance().removeMapLayer(layer)
