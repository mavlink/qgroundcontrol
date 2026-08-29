"""Flight preview manager for QGC4QGIS plugin."""

from typing import Any

from qgis.core import (
    QgsFeature,
    QgsProcessingContext,
    QgsProcessingFeedback,
    QgsProject,
    QgsVectorLayer,
)
from qgis.PyQt.QtCore import QCoreApplication

from qgc4qgis.log import log_warning
from qgc4qgis.processing.alg_survey_grid import SurveyGridAlgorithm


class FlightPreviewManager:
    """Manages temporary memory layers for flight plan preview in QGIS map canvas."""

    def __init__(self) -> None:
        """Initialize FlightPreviewManager."""
        self.area_layer_id: str | None = None
        self.line_layer_id: str | None = None
        self.area_layer: QgsVectorLayer | None = None
        self.line_layer: QgsVectorLayer | None = None

    def clear(self) -> None:
        """Remove preview layers from QgsProject if present."""
        project = QgsProject.instance()

        line_id = self.line_layer_id
        area_id = self.area_layer_id

        self.line_layer_id = None
        self.area_layer_id = None
        self.line_layer = None
        self.area_layer = None

        if project is not None:
            if line_id:
                project.removeMapLayer(line_id)
            if area_id:
                project.removeMapLayer(area_id)

    def update_preview(self, params: dict[str, Any]) -> None:
        """Recreate flight line and area polygon memory layers based on parameter dict."""
        self.clear()

        layer = params.get("INPUT")
        if not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            return

        feat_id = params.get("FEATURE_ID")
        crs_auth = layer.crs().authid() or "EPSG:4326"

        area_layer = QgsVectorLayer(
            f"Polygon?crs={crs_auth}",
            QCoreApplication.translate("FlightPreviewManager", "Preview - Flight Area"),
            "memory",
        )
        provider = area_layer.dataProvider()

        algo_input = layer
        if feat_id is not None:
            feat = layer.getFeature(feat_id)
            if feat.isValid() and not feat.geometry().isEmpty():
                out_feat = QgsFeature()
                out_feat.setGeometry(feat.geometry())
                provider.addFeatures([out_feat])
                area_layer.updateExtents()

                temp_layer = QgsVectorLayer(f"Polygon?crs={crs_auth}", "temp_area", "memory")
                temp_layer.dataProvider().addFeatures([feat])
                temp_layer.updateExtents()
                algo_input = temp_layer
        else:
            features = []
            for f in layer.getFeatures():
                if not f.geometry().isEmpty():
                    out_f = QgsFeature()
                    out_f.setGeometry(f.geometry())
                    features.append(out_f)
            if features:
                provider.addFeatures(features)
                area_layer.updateExtents()

        if area_layer.featureCount() == 0:
            self.clear()
            return

        project = QgsProject.instance()
        if project is not None:
            project.addMapLayer(area_layer)
            self.area_layer_id = area_layer.id()
            self.area_layer = area_layer

        alg = SurveyGridAlgorithm()
        alg.initAlgorithm()

        algo_params = {
            alg.INPUT: algo_input,
            alg.CAMERA: params.get("CAMERA", 0),
            alg.ALTITUDE: params.get("ALTITUDE", 100.0),
            alg.GSD: params.get("GSD", 0.0),
            alg.OVERLAP_SIDE: params.get("OVERLAP_SIDE", 70.0),
            alg.OVERLAP_FRONTAL: params.get("OVERLAP_FRONTAL", 70.0),
            alg.ANGLE: params.get("ANGLE", 0.0),
            alg.TURNAROUND: params.get("TURNAROUND", 0.0),
            alg.ENTRY_LOCATION: params.get("ENTRY_LOCATION", 0),
            alg.REFLY: params.get("REFLY", False),
            alg.SENSOR_WIDTH: params.get("SENSOR_WIDTH", 35.9),
            alg.SENSOR_HEIGHT: params.get("SENSOR_HEIGHT", 24.0),
            alg.IMAGE_WIDTH: params.get("IMAGE_WIDTH", 7952),
            alg.IMAGE_HEIGHT: params.get("IMAGE_HEIGHT", 5304),
            alg.FOCAL_LENGTH: params.get("FOCAL_LENGTH", 35.0),
            alg.OUTPUT: "memory:preview_flight_lines",
        }

        context = QgsProcessingContext()
        if project is not None:
            context.setProject(project)

        feedback = QgsProcessingFeedback()

        try:
            results = alg.processAlgorithm(algo_params, context, feedback)
            if alg.OUTPUT in results:
                line_layer = context.getMapLayer(results[alg.OUTPUT])
                if line_layer is not None and line_layer.isValid():
                    line_layer.setName(
                        QCoreApplication.translate("FlightPreviewManager", "Preview - Flight Lines")
                    )
                    if project is not None:
                        project.addMapLayer(line_layer)
                        self.line_layer_id = line_layer.id()
                        self.line_layer = line_layer
        except Exception as e:
            log_warning(f"Erro ao gerar pré-visualização de linhas de voo: {e}")
