"""Tests for processing provider and survey grid algorithm."""

from qgis.core import (
    Qgis,
    QgsApplication,
    QgsFeature,
    QgsGeometry,
    QgsPointXY,
    QgsProcessingContext,
    QgsProcessingFeedback,
    QgsVectorLayer,
)

from qgc4qgis.plugin import Qgc4QgisPlugin
from qgc4qgis.processing.alg_download_dem import DownloadDemAlgorithm
from qgc4qgis.processing.alg_export_dji import ExportDjiAlgorithm
from qgc4qgis.processing.alg_export_kml import ExportLitchiKmlAlgorithm
from qgc4qgis.processing.alg_export_litchi import ExportLitchiAlgorithm
from qgc4qgis.processing.alg_export_plan import ExportPlanAlgorithm
from qgc4qgis.processing.alg_photo_centers import PhotoCentersAlgorithm
from qgc4qgis.processing.alg_survey_grid import SurveyGridAlgorithm
from qgc4qgis.processing.provider import Qgc4QgisProvider


def test_provider_metadata():
    """Verify processing provider ID, name, and algorithm registration."""
    provider = Qgc4QgisProvider()
    assert provider.id() == "qgc4qgis"
    assert provider.name() == "QGC4QGIS"

    provider.loadAlgorithms()
    algs = provider.algorithms()
    assert len(algs) >= 7
    assert any(alg.name() == "gerar_grade_voo" for alg in algs)
    assert any(alg.name() == "exportar_plano_qgc" for alg in algs)
    assert any(alg.name() == "exportar_litchi_csv" for alg in algs)
    assert any(alg.name() == "exportar_litchi_kml" for alg in algs)
    assert any(alg.name() == "exportar_dji_kmz" for alg in algs)
    assert any(alg.name() == "gerar_centros_foto" for alg in algs)
    assert any(alg.name() == "baixar_dem_copernicus" for alg in algs)


def test_plugin_processing_registration():
    """Verify plugin registers and unregisters provider in QgsApplication processingRegistry."""
    plugin = Qgc4QgisPlugin(iface=None)
    assert plugin.provider is None

    plugin.initProcessing()
    assert plugin.provider is not None
    registry = QgsApplication.processingRegistry()
    assert registry.providerById("qgc4qgis") is not None

    plugin.unload()
    assert plugin.provider is None
    assert registry.providerById("qgc4qgis") is None


def test_algorithm_metadata():
    """Verify SurveyGridAlgorithm metadata."""
    alg = SurveyGridAlgorithm()
    assert alg.name() == "gerar_grade_voo"
    assert alg.displayName() == "Generate flight grid"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "Generates photogrammetric flight grid" in alg.shortHelpString()


def test_algorithm_execution():
    """Verify SurveyGridAlgorithm execution on a polygon vector layer."""
    # Create an in-memory WGS84 polygon layer (100m x 100m near lat=0, lon=0)
    layer = QgsVectorLayer("Polygon?crs=EPSG:4326", "poly_input", "memory")
    pr = layer.dataProvider()

    # Square ~0.001 deg x 0.001 deg (~111m x 111m)
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

    alg = SurveyGridAlgorithm()
    alg.initAlgorithm()

    parameters = {
        alg.INPUT: layer,
        alg.CAMERA: 0,  # Custom camera
        alg.ALTITUDE: 100.0,
        alg.GSD: 0.0,
        alg.OVERLAP_SIDE: 70.0,
        alg.OVERLAP_FRONTAL: 70.0,
        alg.ANGLE: 0.0,
        alg.TURNAROUND: 10.0,
        alg.ENTRY_LOCATION: 0,
        alg.REFLY: False,
        alg.SENSOR_WIDTH: 35.9,
        alg.SENSOR_HEIGHT: 24.0,
        alg.IMAGE_WIDTH: 7952,
        alg.IMAGE_HEIGHT: 5304,
        alg.FOCAL_LENGTH: 35.0,
        alg.OUTPUT: "memory:grid_output",
    }

    context = QgsProcessingContext()
    feedback = QgsProcessingFeedback()

    results = alg.processAlgorithm(parameters, context, feedback)
    assert alg.OUTPUT in results

    out_layer = context.getMapLayer(results[alg.OUTPUT])
    assert out_layer is not None
    assert out_layer.featureCount() > 0

    field_names = [f.name() for f in out_layer.fields()]
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
    for field in expected_fields:
        assert field in field_names

    out_features = list(out_layer.getFeatures())
    assert len(out_features) > 0

    first_feat = out_features[0]
    assert first_feat["altitude_m"] == 100.0
    assert first_feat["side_overlap"] == 70.0
    assert first_feat["frontal_overlap"] == 70.0
    assert first_feat["length_m"] > 0
    assert first_feat["photo_count"] > 0


def test_export_plan_metadata():
    """Verify ExportPlanAlgorithm metadata."""
    alg = ExportPlanAlgorithm()
    assert alg.name() == "exportar_plano_qgc"
    assert alg.displayName() == "Export QGC plan (.plan)"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "Exports a mission plan" in alg.shortHelpString()


def test_export_plan_execution(tmp_path):
    """Verify ExportPlanAlgorithm execution writing a .plan file."""
    import json

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

    output_path = str(tmp_path / "test_mission.plan")

    alg = ExportPlanAlgorithm()
    alg.initAlgorithm()

    parameters = {
        alg.INPUT: layer,
        alg.CAMERA: 0,
        alg.ALTITUDE: 100.0,
        alg.GSD: 0.0,
        alg.OVERLAP_SIDE: 70.0,
        alg.OVERLAP_FRONTAL: 70.0,
        alg.ANGLE: 0.0,
        alg.TURNAROUND: 5.0,
        alg.ENTRY_LOCATION: 0,
        alg.REFLY: False,
        alg.SENSOR_WIDTH: 35.9,
        alg.SENSOR_HEIGHT: 24.0,
        alg.IMAGE_WIDTH: 7952,
        alg.IMAGE_HEIGHT: 5304,
        alg.FOCAL_LENGTH: 35.0,
        alg.CRUISE_SPEED: 15.0,
        alg.HOVER_SPEED: 5.0,
        alg.FIRMWARE_TYPE: 0,  # PX4
        alg.VEHICLE_TYPE: 0,  # Multirotor
        alg.OUTPUT: output_path,
    }

    context = QgsProcessingContext()
    feedback = QgsProcessingFeedback()

    results = alg.processAlgorithm(parameters, context, feedback)
    assert alg.OUTPUT in results
    assert results[alg.OUTPUT] == output_path

    with open(output_path, encoding="utf-8") as f:
        data = json.load(f)

    assert data["fileType"] == "Plan"
    assert data["version"] == 1
    assert data["groundStation"] == "QGroundControl"
    assert "mission" in data
    assert len(data["mission"]["items"]) > 0
    assert data["mission"]["firmwareType"] == 12
    assert data["mission"]["vehicleType"] == 2


def test_photo_centers_metadata():
    """Verify PhotoCentersAlgorithm metadata."""
    alg = PhotoCentersAlgorithm()
    assert alg.name() == "gerar_centros_foto"
    assert alg.displayName() == "Generate photo centers and footprints"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "Generates point layers" in alg.shortHelpString()


def test_photo_centers_execution():
    """Verify PhotoCentersAlgorithm execution creating point photo centers and footprint polygons."""
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

    alg = PhotoCentersAlgorithm()
    alg.initAlgorithm()

    parameters = {
        alg.INPUT: layer,
        alg.CAMERA: 0,
        alg.ALTITUDE: 100.0,
        alg.GSD: 0.0,
        alg.OVERLAP_SIDE: 70.0,
        alg.OVERLAP_FRONTAL: 70.0,
        alg.ANGLE: 0.0,
        alg.TURNAROUND: 0.0,
        alg.ENTRY_LOCATION: 0,
        alg.REFLY: False,
        alg.OUTPUT_CENTERS: "memory:photo_centers",
        alg.OUTPUT_FOOTPRINTS: "memory:photo_footprints",
    }

    context = QgsProcessingContext()
    feedback = QgsProcessingFeedback()

    results = alg.processAlgorithm(parameters, context, feedback)
    assert alg.OUTPUT_CENTERS in results
    assert alg.OUTPUT_FOOTPRINTS in results

    centers_layer = context.getMapLayer(results[alg.OUTPUT_CENTERS])
    footprints_layer = context.getMapLayer(results[alg.OUTPUT_FOOTPRINTS])

    assert centers_layer is not None
    assert footprints_layer is not None

    centers_count = centers_layer.featureCount()
    footprints_count = footprints_layer.featureCount()

    assert centers_count > 0
    assert footprints_count == centers_count

    center_feats = list(centers_layer.getFeatures())
    footprint_feats = list(footprints_layer.getFeatures())

    first_center = center_feats[0]
    assert first_center["altitude_m"] == 100.0
    assert first_center["transect_id"] == 1
    assert first_center["photo_id"] == 1
    assert "azimuth_deg" in [f.name() for f in centers_layer.fields()]

    first_footprint = footprint_feats[0]
    assert first_footprint["altitude_m"] == 100.0
    assert first_footprint["area_m2"] > 0.0
    assert first_footprint.geometry().isGeosValid()
    assert first_footprint.geometry().type() == Qgis.GeometryType.Polygon


def test_export_litchi_metadata():
    """Verify ExportLitchiAlgorithm metadata."""
    alg = ExportLitchiAlgorithm()
    assert alg.name() == "exportar_litchi_csv"
    assert alg.displayName() == "Export Litchi mission (.csv)"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "Exports a flight mission" in alg.shortHelpString()


def test_export_litchi_execution(tmp_path):
    """Verify ExportLitchiAlgorithm execution writing a Litchi CSV file."""
    import csv

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

    output_path = str(tmp_path / "test_litchi.csv")

    alg = ExportLitchiAlgorithm()
    alg.initAlgorithm()

    parameters = {
        alg.INPUT: layer,
        alg.CAMERA: 0,
        alg.ALTITUDE: 100.0,
        alg.GSD: 0.0,
        alg.OVERLAP_SIDE: 70.0,
        alg.OVERLAP_FRONTAL: 70.0,
        alg.ANGLE: 0.0,
        alg.TURNAROUND: 5.0,
        alg.ENTRY_LOCATION: 0,
        alg.REFLY: False,
        alg.TRIGGER_MODE: 0,  # By distance
        alg.SPEED: 5.0,
        alg.GIMBAL_PITCH: -90.0,
        alg.WAYPOINT_WAIT: 2.0,  # 2s wait -> 2000ms
        alg.OUTPUT: output_path,
    }

    context = QgsProcessingContext()
    feedback = QgsProcessingFeedback()

    results = alg.processAlgorithm(parameters, context, feedback)
    assert alg.OUTPUT in results
    assert results[alg.OUTPUT] == output_path

    with open(output_path, encoding="utf-8") as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    assert len(rows) > 0
    assert "latitude" in rows[0]
    assert "longitude" in rows[0]
    assert "altitude(m)" in rows[0]
    assert "actiontype1" in rows[0]
    # Check wait action (actiontype 0 = Stay, actionparam = 2000ms)
    assert rows[0]["actiontype1"] == "0"
    assert rows[0]["actionparam1"] == "2000.0"


def test_export_dji_metadata():
    """Verify ExportDjiAlgorithm metadata."""
    alg = ExportDjiAlgorithm()
    assert alg.name() == "exportar_dji_kmz"
    assert alg.displayName() == "Export DJI Fly mission (.kmz)"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "Exports a flight mission" in alg.shortHelpString()


def test_export_dji_execution(tmp_path):
    """Verify ExportDjiAlgorithm execution writing a DJI WPML KMZ file."""
    import zipfile

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

    output_path = str(tmp_path / "test_dji.kmz")

    alg = ExportDjiAlgorithm()
    alg.initAlgorithm()

    parameters = {
        alg.INPUT: layer,
        alg.CAMERA: 0,
        alg.ALTITUDE: 100.0,
        alg.GSD: 0.0,
        alg.OVERLAP_SIDE: 70.0,
        alg.OVERLAP_FRONTAL: 70.0,
        alg.ANGLE: 0.0,
        alg.TURNAROUND: 5.0,
        alg.ENTRY_LOCATION: 0,
        alg.REFLY: False,
        alg.TRIGGER_MODE: 0,  # By distance
        alg.SPEED: 5.0,
        alg.GIMBAL_PITCH: -90.0,
        alg.WAYPOINT_WAIT: 1.0,
        alg.FINISH_ACTION: 0,  # goHome
        alg.RC_LOST_ACTION: 0,  # goBack
        alg.TRANSITIONAL_SPEED: 8.0,
        alg.ZIP_LAYOUT: 0,  # subfolder wpmz/
        alg.OUTPUT: output_path,
    }

    context = QgsProcessingContext()
    feedback = QgsProcessingFeedback()

    results = alg.processAlgorithm(parameters, context, feedback)
    assert alg.OUTPUT in results
    assert results[alg.OUTPUT] == output_path

    with zipfile.ZipFile(output_path, "r") as zf:
        namelist = zf.namelist()
        assert "wpmz/template.kml" in namelist
        assert "wpmz/waylines.wpml" in namelist

        template_content = zf.read("wpmz/template.kml").decode("utf-8")
        assert "<wpml:finishAction>goHome</wpml:finishAction>" in template_content
        assert "<wpml:executeRCLostAction>goBack</wpml:executeRCLostAction>" in template_content
        assert (
            "<wpml:globalTransitionalSpeed>8.0</wpml:globalTransitionalSpeed>" in template_content
        )


def test_export_litchi_kml_metadata():
    """Verify ExportLitchiKmlAlgorithm metadata."""
    alg = ExportLitchiKmlAlgorithm()
    alg.initAlgorithm()

    assert alg.name() == "exportar_litchi_kml"
    assert alg.displayName() == "Export Litchi Mission Hub mission (.kml)"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "flylitchi.com/hub" in alg.shortHelpString()

    assert alg.parameterDefinition("INPUT") is not None
    assert alg.parameterDefinition("OUTPUT") is not None

    trig_param = alg.parameterDefinition("TRIGGER_MODE")
    assert trig_param is not None
    assert trig_param.defaultValue() == 2

    assert alg.parameterDefinition("GIMBAL_PITCH") is None
    assert alg.parameterDefinition("WAYPOINT_WAIT") is None
    assert not hasattr(alg, "GIMBAL_PITCH")
    assert not hasattr(alg, "WAYPOINT_WAIT")


def test_download_dem_metadata():
    """Verify DownloadDemAlgorithm metadata and parameters."""
    alg = DownloadDemAlgorithm()
    alg.initAlgorithm()

    assert alg.name() == "baixar_dem_copernicus"
    assert alg.displayName() == "Download Copernicus DEM for area"
    assert alg.group() == "Flight Planning"
    assert alg.groupId() == "planejamento_voo"
    assert alg.createInstance().name() == alg.name()
    assert "Copernicus (GLO-30)" in alg.shortHelpString()

    assert alg.parameterDefinition("INPUT") is not None
    assert alg.parameterDefinition("MARGEM") is not None
    assert alg.parameterDefinition("OUTPUT") is not None
