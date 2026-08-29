"""Unit tests for QGC Planning dock widget and plugin GUI integration."""

from unittest.mock import MagicMock

from qgis.core import QgsFeature, QgsField, QgsGeometry, QgsPointXY, QgsVectorLayer
from qgis.PyQt.QtCore import QVariant
from qgis.PyQt.QtWidgets import QPushButton, QScrollArea

from qgc4qgis.core.cameras import CUSTOM_CAMERA_NAME
from qgc4qgis.gui.dock import QgcPlanningDockWidget
from qgc4qgis.plugin import Qgc4QgisPlugin


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


def test_dock_widget_initialization(qgis_app):
    """Test dock widget title, object name, and default controls state."""
    dock = QgcPlanningDockWidget()
    assert dock.windowTitle() == "QGC Flight Planning"
    assert dock.objectName() == "QgcPlanningDockWidget"
    assert dock.rad_mode_altitude.isChecked()
    assert dock.spn_altitude.isEnabled()
    assert not dock.spn_gsd.isEnabled()


def test_altitude_gsd_mode_lock(qgis_app):
    """Test altitude vs GSD pair locking and automatic recalculation."""
    dock = QgcPlanningDockWidget()

    # Altura mode active: change altitude -> GSD updates automatically
    dock.rad_mode_altitude.setChecked(True)
    dock.spn_altitude.setValue(120.0)
    calc_gsd = dock.spn_gsd.value()
    assert calc_gsd > 0.0

    # Switch to GSD mode: GSD becomes enabled, Altura disabled
    dock.rad_mode_gsd.setChecked(True)
    assert not dock.spn_altitude.isEnabled()
    assert dock.spn_gsd.isEnabled()

    # Change GSD -> Altitude updates automatically
    dock.spn_gsd.setValue(3.0)
    calc_alt = dock.spn_altitude.value()
    assert calc_alt > 0.0


def test_live_adjusted_footprint_recalculation(qgis_app):
    """Test live recalculation of adjusted footprints and overlap signal handling."""
    dock = QgcPlanningDockWidget()
    assert dock.camera_calc is not None
    assert dock.camera_calc.adjusted_footprint_side > 0.0
    assert dock.camera_calc.adjusted_footprint_frontal > 0.0
    initial_text = dock.lbl_footprint.text()
    assert "m x" in initial_text

    initial_side_adj = dock.camera_calc.adjusted_footprint_side

    # Change side overlap percentage -> adjusted footprint updates live
    dock.spn_overlap_side.setValue(80.0)
    assert dock.camera_calc.adjusted_footprint_side < initial_side_adj
    assert dock.lbl_footprint.text() != initial_text


def test_custom_camera_toggle(qgis_app):
    """Test manual camera visibility toggle and custom specs resolution."""
    dock = QgcPlanningDockWidget()

    # Find CUSTOM_CAMERA_NAME index
    custom_idx = -1
    for i in range(dock.cmb_camera.count()):
        if dock.cmb_camera.itemText(i) == CUSTOM_CAMERA_NAME:
            custom_idx = i
            break

    assert custom_idx >= 0
    dock.cmb_camera.setCurrentIndex(custom_idx)
    assert not dock.grp_custom_camera.isHidden()

    spec = dock.get_current_camera_spec()
    assert spec is not None
    assert spec.canonicalName == CUSTOM_CAMERA_NAME
    assert spec.sensorWidth == dock.spn_sensor_w.value()


def test_layer_and_feature_selection(qgis_app):
    """Test feature combo box population when polygon layer is assigned."""
    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)
    dock._on_layer_changed()

    assert dock.cmb_feature.count() == 2
    assert dock.cmb_feature.itemText(0) == "All features"
    assert dock.cmb_feature.itemData(0) is None
    assert "Area 1" in dock.cmb_feature.itemText(1)

    QgsProject.instance().removeMapLayer(layer)


def test_get_parameters_and_signal_emission(qgis_app):
    """Test parameter collection and signal emission on generate button click."""
    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)
    dock.spn_altitude.setValue(150.0)
    dock.spn_overlap_side.setValue(80.0)
    dock.spn_angle.setValue(45.0)

    received_params = []
    dock.gridGenerated.connect(lambda p: received_params.append(p))

    dock.btn_generate.click()

    assert len(received_params) == 1
    params = received_params[0]
    assert params["INPUT"] == layer
    assert params["ALTITUDE"] == 150.0
    assert params["OVERLAP_SIDE"] == 80.0
    assert params["ANGLE"] == 45.0

    QgsProject.instance().removeMapLayer(layer)


def test_plugin_gui_dock_lifecycle(qgis_app):
    """Test plugin initialization, dock creation on run(), and unload cleanup."""
    mock_iface = MagicMock()
    mock_iface.mainWindow.return_value = None

    plugin = Qgc4QgisPlugin(mock_iface)
    plugin.initGui()
    assert plugin.action is not None

    plugin.run()
    assert plugin.dock_widget is not None
    mock_iface.addDockWidget.assert_called_once()

    plugin.unload()
    assert plugin.dock_widget is None
    mock_iface.removeDockWidget.assert_called_once()


def test_export_plan_and_add_layers_buttons(qgis_app, tmp_path):
    """Test 'Export .plan...' and 'Add layers to project' buttons and handlers."""
    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    assert dock.btn_export_plan.text() == "Export .plan…"
    assert dock.btn_add_layers.text() == "Add layers to project"

    dock.cmb_layer.setLayer(layer)

    exported_signal = []
    dock.planExported.connect(lambda p: exported_signal.append(p))

    plan_file = str(tmp_path / "test_out.plan")
    result_path = dock.export_plan(file_path=plan_file)
    assert result_path == plan_file
    assert len(exported_signal) == 1
    assert exported_signal[0]["INPUT"] == layer

    grid_signal = []
    dock.gridGenerated.connect(lambda p: grid_signal.append(p))
    dock.btn_add_layers.click()
    assert len(grid_signal) == 1

    QgsProject.instance().removeMapLayer(layer)


def test_dock_elevation_layer_and_tolerance(qgis_app):
    """Test elevation layer combo box, tolerance spin box, and parameters dictionary."""
    dock = QgcPlanningDockWidget()
    assert dock.cmb_elevation_layer is not None
    assert dock.spn_tolerance is not None
    assert dock.spn_tolerance.value() == 10.0

    dock.spn_tolerance.setValue(15.5)
    params = dock.get_parameters()

    assert params["TOLERANCE"] == 15.5
    assert params["TERRAIN_TOLERANCE"] == 15.5
    assert params["ELEVATION_LAYER"] is None


def test_dock_flight_statistics_update(qgis_app):
    """Test flight statistics labels update when polygon layer and parameters change."""
    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)
    dock._on_layer_changed()

    assert dock.lbl_stat_area.text() != "-"
    assert "m²" in dock.lbl_stat_area.text()
    assert dock.lbl_stat_distance.text() != "-"
    assert "m" in dock.lbl_stat_distance.text()
    assert dock.lbl_stat_photos.text() != "-"
    assert int(dock.lbl_stat_photos.text()) > 0
    assert dock.lbl_stat_time.text() != "-"
    assert dock.lbl_stat_wp_qgc.text() != "-"
    assert dock.lbl_stat_wp_litchi.text() != "-"
    assert dock.lbl_stat_wp_dji.text() != "-"

    old_photos = dock.lbl_stat_photos.text()
    dock.spn_overlap_frontal.setValue(85.0)
    assert dock.lbl_stat_photos.text() != old_photos

    QgsProject.instance().removeMapLayer(layer)


def test_dock_waypoint_statistics_and_limits(qgis_app):
    """Test waypoint counts display per destination and warnings when exceeding Litchi (99) or DJI (200) limits (Step 55)."""
    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)
    dock._on_layer_changed()

    assert dock.lbl_stat_wp_qgc.text().isdigit()
    assert dock.lbl_stat_wp_litchi.text().isdigit()
    assert dock.lbl_stat_wp_dji.text().isdigit()

    # High overlap to generate many transects/waypoints exceeding limits
    dock.spn_overlap_side.setValue(98.0)
    dock.spn_overlap_frontal.setValue(98.0)

    wp_qgc = int(dock.lbl_stat_wp_qgc.text())
    assert wp_qgc > 99

    assert "exceeds limit: 99" in dock.lbl_stat_wp_litchi.text()
    assert "Litchi limit (99)" in dock.lbl_stat_warning.text()
    assert not dock.lbl_stat_warning.isHidden()

    QgsProject.instance().removeMapLayer(layer)


def test_dock_export_to_group_and_buttons(qgis_app, tmp_path):
    """Test 'Export to' panel controls, parameters, and export buttons (Step 54)."""
    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    assert dock.cmb_trigger_mode.count() == 3
    assert dock.spn_speed.value() == 5.0
    assert dock.spn_gimbal_pitch.value() == -90.0
    assert dock.spn_waypoint_wait.value() == 0.0
    assert dock.btn_export_litchi.text() == "Export Litchi (.csv)…"
    assert dock.btn_export_kml.text() == "Export classic Litchi Hub (.kml)…"
    assert "flylitchi.com/hub" in dock.btn_export_kml.toolTip()
    assert dock.btn_export_dji.text() == "Export DJI Fly / Litchi Hub 2 (.kmz)…"
    assert "hub.flylitchi.com" in dock.btn_export_dji.toolTip()
    assert "flylitchi.com/hub" in dock.btn_export_dji.toolTip()

    dock.cmb_layer.setLayer(layer)
    dock.cmb_trigger_mode.setCurrentIndex(1)
    dock.spn_speed.setValue(8.0)
    dock.spn_gimbal_pitch.setValue(-45.0)
    dock.spn_waypoint_wait.setValue(2.5)

    params = dock.get_parameters()
    assert params["TRIGGER_MODE"] == 1
    assert params["SPEED"] == 8.0
    assert params["GIMBAL_PITCH"] == -45.0
    assert params["WAYPOINT_WAIT"] == 2.5

    litchi_signals = []
    dock.litchiExported.connect(lambda p: litchi_signals.append(p))
    out_csv = str(tmp_path / "test_litchi.csv")
    res_csv = dock.export_litchi(file_path=out_csv)
    assert res_csv == out_csv
    assert len(litchi_signals) == 1
    assert litchi_signals[0]["SPEED"] == 8.0

    kml_signals = []
    dock.kmlExported.connect(lambda p: kml_signals.append(p))
    out_kml = str(tmp_path / "test_kml.kml")
    res_kml = dock.export_kml(file_path=out_kml)
    assert res_kml == out_kml
    assert len(kml_signals) == 1
    assert kml_signals[0]["SPEED"] == 8.0

    dji_signals = []
    dock.djiExported.connect(lambda p: dji_signals.append(p))
    out_kmz = str(tmp_path / "test_dji.kmz")
    res_kmz = dock.export_dji(file_path=out_kmz)
    assert res_kmz == out_kmz
    assert len(dji_signals) == 1
    assert dji_signals[0]["GIMBAL_PITCH"] == -45.0

    QgsProject.instance().removeMapLayer(layer)


def test_dock_export_kml_button_and_without_layer(qgis_app, tmp_path, monkeypatch):
    """Test dock.btn_export_kml properties and export_kml without a valid layer."""
    from qgis.core import QgsProject
    from qgis.PyQt.QtWidgets import QMessageBox

    QgsProject.instance().removeAllMapLayers()
    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(None)

    assert hasattr(dock, "btn_export_kml")
    assert isinstance(dock.btn_export_kml, QPushButton)
    assert dock.btn_export_kml.text() == "Export classic Litchi Hub (.kml)…"
    assert "flylitchi.com/hub" in dock.btn_export_kml.toolTip()

    monkeypatch.setattr(QMessageBox, "warning", lambda *args, **kwargs: None)
    result = dock.export_kml(str(tmp_path / "m.kml"))
    assert result is None


def test_dock_widget_is_scrollable(qgis_app):
    """Test dock widget scroll area container, widget hierarchy, and height constraints."""
    dock = QgcPlanningDockWidget()
    assert isinstance(dock.widget(), QScrollArea)
    assert dock.widget().widgetResizable()

    parent = dock.btn_generate.parent()
    while parent is not None and parent != dock.widget().widget():
        parent = parent.parent()
    assert parent == dock.widget().widget()

    assert dock.minimumSizeHint().height() < dock.widget().widget().sizeHint().height()


def test_dock_download_dem_button_and_without_layer(qgis_app, monkeypatch):
    """Test dock.btn_download_dem exists, is QPushButton, is inside scroll area main_widget, and download_dem() without layer handles warning safely."""
    from qgis.core import QgsProject
    from qgis.PyQt.QtWidgets import QMessageBox

    QgsProject.instance().removeAllMapLayers()
    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(None)

    assert hasattr(dock, "btn_download_dem")
    assert isinstance(dock.btn_download_dem, QPushButton)
    assert dock.btn_download_dem.text() == "Download area DEM…"

    scroll_main = dock.scroll_area.widget()
    parent = dock.btn_download_dem.parent()
    while parent is not None and parent != scroll_main:
        parent = parent.parent()
    assert parent == scroll_main

    warning_called = False

    def mock_warning(*args, **kwargs):
        nonlocal warning_called
        warning_called = True

    monkeypatch.setattr(QMessageBox, "warning", mock_warning)

    dock.download_dem()
    assert warning_called


def test_dock_exporters_headless_import_error_logs_warning(qgis_app, tmp_path, monkeypatch):
    """Test that dock exporters log warning when processing module is missing in headless environment (Step 114)."""
    import sys

    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)

    logged_warnings = []

    def mock_log_warning(msg, tag="QGC4QGIS"):
        logged_warnings.append(msg)

    monkeypatch.setattr("qgc4qgis.gui.dock.log_warning", mock_log_warning)
    monkeypatch.setitem(sys.modules, "processing", None)

    res1 = dock.export_plan(str(tmp_path / "test.plan"))
    res2 = dock.export_litchi(str(tmp_path / "test.csv"))
    res3 = dock.export_kml(str(tmp_path / "test.kml"))
    res4 = dock.export_dji(str(tmp_path / "test.kmz"))

    assert res1 == str(tmp_path / "test.plan")
    assert res2 == str(tmp_path / "test.csv")
    assert res3 == str(tmp_path / "test.kml")
    assert res4 == str(tmp_path / "test.kmz")

    assert len(logged_warnings) == 4
    assert "exportar_plano_qgc" in logged_warnings[0]
    assert "exportar_litchi_csv" in logged_warnings[1]
    assert "exportar_litchi_kml" in logged_warnings[2]
    assert "exportar_dji_kmz" in logged_warnings[3]

    QgsProject.instance().removeMapLayer(layer)


def test_dock_exporters_logging_on_processing_exception(qgis_app, tmp_path, monkeypatch):
    """Test that dock exporters log errors via log_error when processing.run raises Exception (Step 114)."""
    import sys
    import types

    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)

    logged_errors = []

    def mock_log_error(msg, tag="QGC4QGIS"):
        logged_errors.append(msg)

    monkeypatch.setattr("qgc4qgis.gui.dock.log_error", mock_log_error)

    # Create dummy module for processing
    fake_proc = types.ModuleType("processing")

    def mock_run(*args, **kwargs):
        raise RuntimeError("Test processing failure")

    fake_proc.run = mock_run

    monkeypatch.setitem(sys.modules, "processing", fake_proc)

    dock.export_plan(str(tmp_path / "test.plan"))
    dock.export_litchi(str(tmp_path / "test.csv"))
    dock.export_kml(str(tmp_path / "test.kml"))
    dock.export_dji(str(tmp_path / "test.kmz"))

    assert len(logged_errors) == 4
    assert "exportar_plano_qgc" in logged_errors[0]
    assert "exportar_litchi_csv" in logged_errors[1]
    assert "exportar_litchi_kml" in logged_errors[2]
    assert "exportar_dji_kmz" in logged_errors[3]

    QgsProject.instance().removeMapLayer(layer)


def test_dock_remaining_methods_headless_import_error_logs_warning(qgis_app, monkeypatch):
    """Test that generate_grid, add_layers_to_project, and download_dem log warnings on missing processing module (Step 115)."""
    import sys

    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)

    logged_warnings = []

    def mock_log_warning(msg, tag="QGC4QGIS"):
        logged_warnings.append(msg)

    monkeypatch.setattr("qgc4qgis.gui.dock.log_warning", mock_log_warning)
    monkeypatch.setitem(sys.modules, "processing", None)

    dock.generate_grid()
    dock.add_layers_to_project()
    dock.download_dem()

    assert len(logged_warnings) == 3
    assert "gerar_grade_voo" in logged_warnings[0]
    assert "gerar_grade_voo" in logged_warnings[1]
    assert "baixar_dem_copernicus" in logged_warnings[2]

    QgsProject.instance().removeMapLayer(layer)


def test_dock_remaining_methods_logging_on_processing_exception(qgis_app, monkeypatch):
    """Test that generate_grid, add_layers_to_project, and download_dem log errors via log_error on processing failure (Step 115)."""
    import sys
    import types

    from qgis.core import QgsProject

    layer = create_sample_polygon_layer()
    QgsProject.instance().addMapLayer(layer)

    dock = QgcPlanningDockWidget()
    dock.cmb_layer.setLayer(layer)

    logged_errors = []

    def mock_log_error(msg, tag="QGC4QGIS"):
        logged_errors.append(msg)

    monkeypatch.setattr("qgc4qgis.gui.dock.log_error", mock_log_error)

    fake_proc = types.ModuleType("processing")

    def mock_run(*args, **kwargs):
        raise RuntimeError("Test processing failure")

    fake_proc.run = mock_run
    fake_proc.runAndLoadResults = mock_run

    monkeypatch.setitem(sys.modules, "processing", fake_proc)

    dock.generate_grid()
    dock.add_layers_to_project()
    dock.download_dem()

    assert len(logged_errors) == 3
    assert "gerar_grade_voo" in logged_errors[0]
    assert "gerar_grade_voo" in logged_errors[1]
    assert "baixar_dem_copernicus" in logged_errors[2]

    QgsProject.instance().removeMapLayer(layer)
