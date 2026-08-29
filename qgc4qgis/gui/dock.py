"""QGC Flight Planning dock widget implementation."""

from typing import Any

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsMapLayerProxyModel,
    QgsProcessingException,
    QgsProcessingFeatureSourceDefinition,
    QgsProject,
    QgsRasterLayer,
    QgsVectorLayer,
)
from qgis.gui import QgsDockWidget, QgsMapLayerComboBox
from qgis.PyQt.QtCore import QCoreApplication, pyqtSignal
from qgis.PyQt.QtWidgets import (
    QButtonGroup,
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFileDialog,
    QFormLayout,
    QFrame,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QPushButton,
    QRadioButton,
    QScrollArea,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from qgc4qgis.core.cameracalc import CameraCalc
from qgc4qgis.core.cameras import CUSTOM_CAMERA_NAME, CameraSpec, load_cameras
from qgc4qgis.core.elevation import ATTRIBUTION
from qgc4qgis.core.route import route_from_transects
from qgc4qgis.core.settings import load_project_settings, save_project_settings
from qgc4qgis.core.stats import calculate_flight_stats, calculate_polygon_area
from qgc4qgis.gui.preview import FlightPreviewManager
from qgc4qgis.log import log_error, log_warning

try:
    NO_FRAME = QFrame.Shape.NoFrame  # Qt6 (PyQt6)
except AttributeError:
    NO_FRAME = QFrame.NoFrame  # Qt5 (PyQt5)


class QgcPlanningDockWidget(QgsDockWidget):
    """Dock widget for QGC Flight Planning in QGIS."""

    gridGenerated = pyqtSignal(dict)
    planExported = pyqtSignal(dict)
    litchiExported = pyqtSignal(dict)
    kmlExported = pyqtSignal(dict)
    djiExported = pyqtSignal(dict)

    def __init__(self, parent: QWidget | None = None):
        """Constructor for QgcPlanningDockWidget."""
        super().__init__(
            QCoreApplication.translate("QgcPlanningDockWidget", "QGC Flight Planning"), parent
        )
        self.setObjectName("QgcPlanningDockWidget")

        self.cameras: list[CameraSpec] = load_cameras()
        self._updating_lock: bool = True
        self.camera_calc: CameraCalc = CameraCalc()
        self.preview_manager: FlightPreviewManager = FlightPreviewManager()

        self._init_ui()
        self.load_settings()
        self._updating_lock = False
        self._recalculate_locked_pair()

    def _init_ui(self) -> None:
        """Build layout and initialize widgets."""
        main_widget = QWidget()
        main_layout = QVBoxLayout(main_widget)
        main_layout.setContentsMargins(8, 8, 8, 8)
        main_layout.setSpacing(10)

        # 1. Layer and Feature
        grp_layer = QGroupBox(self.tr("Layer / Polygon Feature"), main_widget)
        lay_layer = QFormLayout(grp_layer)

        self.cmb_layer = QgsMapLayerComboBox(grp_layer)
        self.cmb_layer.setFilters(QgsMapLayerProxyModel.PolygonLayer)
        self.cmb_layer.layerChanged.connect(self._on_layer_changed)
        lay_layer.addRow(self.tr("Layer:"), self.cmb_layer)

        self.cmb_feature = QComboBox(grp_layer)
        self.cmb_feature.currentIndexChanged.connect(self._on_feature_changed)
        lay_layer.addRow(self.tr("Feature:"), self.cmb_feature)

        main_layout.addWidget(grp_layer)

        # 2. Camera Selector
        grp_camera = QGroupBox(self.tr("Camera"), main_widget)
        lay_camera = QVBoxLayout(grp_camera)

        self.cmb_camera = QComboBox(grp_camera)
        for spec in self.cameras:
            self.cmb_camera.addItem(spec.canonicalName)
        self.cmb_camera.currentIndexChanged.connect(self._on_camera_changed)
        lay_camera.addWidget(self.cmb_camera)

        # Manual Camera Settings
        self.grp_custom_camera = QGroupBox(self.tr("Manual Camera Specifications"), grp_camera)
        lay_custom = QFormLayout(self.grp_custom_camera)

        self.spn_sensor_w = QDoubleSpinBox(self.grp_custom_camera)
        self.spn_sensor_w.setRange(0.1, 100.0)
        self.spn_sensor_w.setValue(35.9)
        self.spn_sensor_w.setSuffix(" mm")
        self.spn_sensor_w.setDecimals(1)
        self.spn_sensor_w.valueChanged.connect(self._on_camera_param_changed)
        lay_custom.addRow(self.tr("Sensor width:"), self.spn_sensor_w)

        self.spn_sensor_h = QDoubleSpinBox(self.grp_custom_camera)
        self.spn_sensor_h.setRange(0.1, 100.0)
        self.spn_sensor_h.setValue(24.0)
        self.spn_sensor_h.setSuffix(" mm")
        self.spn_sensor_h.setDecimals(1)
        self.spn_sensor_h.valueChanged.connect(self._on_camera_param_changed)
        lay_custom.addRow(self.tr("Sensor height:"), self.spn_sensor_h)

        self.spn_img_w = QSpinBox(self.grp_custom_camera)
        self.spn_img_w.setRange(1, 100000)
        self.spn_img_w.setValue(7952)
        self.spn_img_w.setSuffix(" px")
        self.spn_img_w.valueChanged.connect(self._on_camera_param_changed)
        lay_custom.addRow(self.tr("Image width:"), self.spn_img_w)

        self.spn_img_h = QSpinBox(self.grp_custom_camera)
        self.spn_img_h.setRange(1, 100000)
        self.spn_img_h.setValue(5304)
        self.spn_img_h.setSuffix(" px")
        self.spn_img_h.valueChanged.connect(self._on_camera_param_changed)
        lay_custom.addRow(self.tr("Image height:"), self.spn_img_h)

        self.spn_focal = QDoubleSpinBox(self.grp_custom_camera)
        self.spn_focal.setRange(0.1, 1000.0)
        self.spn_focal.setValue(35.0)
        self.spn_focal.setSuffix(" mm")
        self.spn_focal.setDecimals(1)
        self.spn_focal.valueChanged.connect(self._on_camera_param_changed)
        lay_custom.addRow(self.tr("Focal length:"), self.spn_focal)

        lay_camera.addWidget(self.grp_custom_camera)
        self.grp_custom_camera.setVisible(False)

        main_layout.addWidget(grp_camera)

        # 3. Altitude / GSD (with locked mode)
        grp_alt_gsd = QGroupBox(self.tr("Flight Altitude / GSD"), main_widget)
        lay_alt_gsd = QVBoxLayout(grp_alt_gsd)

        lay_modes = QHBoxLayout()
        self.rad_mode_altitude = QRadioButton(self.tr("Flight Altitude"), grp_alt_gsd)
        self.rad_mode_gsd = QRadioButton(self.tr("GSD"), grp_alt_gsd)
        self.rad_mode_altitude.setChecked(True)

        self.mode_group = QButtonGroup(grp_alt_gsd)
        self.mode_group.addButton(self.rad_mode_altitude, 0)
        self.mode_group.addButton(self.rad_mode_gsd, 1)
        self.mode_group.idToggled.connect(self._on_mode_changed)

        lay_modes.addWidget(self.rad_mode_altitude)
        lay_modes.addWidget(self.rad_mode_gsd)
        lay_alt_gsd.addLayout(lay_modes)

        lay_spin_form = QFormLayout()

        self.spn_altitude = QDoubleSpinBox(grp_alt_gsd)
        self.spn_altitude.setRange(0.1, 10000.0)
        self.spn_altitude.setValue(100.0)
        self.spn_altitude.setSuffix(" m")
        self.spn_altitude.setDecimals(2)
        self.spn_altitude.valueChanged.connect(self._on_altitude_changed)
        lay_spin_form.addRow(self.tr("Flight Altitude:"), self.spn_altitude)

        self.spn_gsd = QDoubleSpinBox(grp_alt_gsd)
        self.spn_gsd.setRange(0.01, 1000.0)
        self.spn_gsd.setValue(2.0)
        self.spn_gsd.setSuffix(" cm/px")
        self.spn_gsd.setDecimals(2)
        self.spn_gsd.setEnabled(False)
        self.spn_gsd.valueChanged.connect(self._on_gsd_changed)
        lay_spin_form.addRow(self.tr("GSD:"), self.spn_gsd)

        self.lbl_footprint = QLabel(grp_alt_gsd)
        lay_spin_form.addRow(self.tr("Adjusted footprint:"), self.lbl_footprint)

        lay_alt_gsd.addLayout(lay_spin_form)
        main_layout.addWidget(grp_alt_gsd)

        # 4. Overlaps, Angle, Turnaround, and Entry Location
        grp_grid_params = QGroupBox(self.tr("Flight Parameters"), main_widget)
        lay_grid_params = QFormLayout(grp_grid_params)

        self.spn_overlap_side = QDoubleSpinBox(grp_grid_params)
        self.spn_overlap_side.setRange(0.0, 99.0)
        self.spn_overlap_side.setValue(70.0)
        self.spn_overlap_side.setSuffix(" %")
        self.spn_overlap_side.valueChanged.connect(self._on_overlap_changed)
        lay_grid_params.addRow(self.tr("Side overlap:"), self.spn_overlap_side)

        self.spn_overlap_frontal = QDoubleSpinBox(grp_grid_params)
        self.spn_overlap_frontal.setRange(0.0, 99.0)
        self.spn_overlap_frontal.setValue(70.0)
        self.spn_overlap_frontal.setSuffix(" %")
        self.spn_overlap_frontal.valueChanged.connect(self._on_overlap_changed)
        lay_grid_params.addRow(self.tr("Frontal overlap:"), self.spn_overlap_frontal)

        self.spn_angle = QDoubleSpinBox(grp_grid_params)
        self.spn_angle.setRange(-180.0, 180.0)
        self.spn_angle.setValue(0.0)
        self.spn_angle.setSuffix("°")
        self.spn_angle.valueChanged.connect(self._on_grid_param_changed)
        lay_grid_params.addRow(self.tr("Grid angle:"), self.spn_angle)

        self.spn_turnaround = QDoubleSpinBox(grp_grid_params)
        self.spn_turnaround.setRange(0.0, 1000.0)
        self.spn_turnaround.setValue(0.0)
        self.spn_turnaround.setSuffix(" m")
        self.spn_turnaround.valueChanged.connect(self._on_grid_param_changed)
        lay_grid_params.addRow(self.tr("Turnaround:"), self.spn_turnaround)

        self.cmb_entry_location = QComboBox(grp_grid_params)
        self.cmb_entry_location.addItems(["Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right"])
        self.cmb_entry_location.currentIndexChanged.connect(self._on_grid_param_changed)
        lay_grid_params.addRow(self.tr("Entry location:"), self.cmb_entry_location)

        self.chk_refly = QCheckBox(self.tr("Cross grid (Refly 90°)"), grp_grid_params)
        self.chk_refly.toggled.connect(self._on_grid_param_changed)
        lay_grid_params.addRow(self.chk_refly)

        main_layout.addWidget(grp_grid_params)

        # 5. Terrain / Elevation
        grp_terrain = QGroupBox(self.tr("Terrain / Elevation"), main_widget)
        lay_terrain = QFormLayout(grp_terrain)

        self.cmb_elevation_layer = QgsMapLayerComboBox(grp_terrain)
        self.cmb_elevation_layer.setFilters(QgsMapLayerProxyModel.RasterLayer)
        self.cmb_elevation_layer.setAllowEmptyLayer(True)
        self.cmb_elevation_layer.layerChanged.connect(self._on_grid_param_changed)
        lay_terrain.addRow(self.tr("Elevation layer:"), self.cmb_elevation_layer)

        self.btn_download_dem = QPushButton(self.tr("Download area DEM…"), grp_terrain)
        self.btn_download_dem.clicked.connect(self.download_dem)
        lay_terrain.addRow(self.btn_download_dem)

        self.spn_tolerance = QDoubleSpinBox(grp_terrain)
        self.spn_tolerance.setRange(0.1, 1000.0)
        self.spn_tolerance.setValue(10.0)
        self.spn_tolerance.setSuffix(" m")
        self.spn_tolerance.setDecimals(1)
        self.spn_tolerance.valueChanged.connect(self._on_grid_param_changed)
        lay_terrain.addRow(self.tr("Terrain tolerance:"), self.spn_tolerance)

        main_layout.addWidget(grp_terrain)

        # 6. Flight Statistics
        grp_stats = QGroupBox(self.tr("Flight Statistics"), main_widget)
        lay_stats = QFormLayout(grp_stats)

        self.lbl_stat_area = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("Flight area:"), self.lbl_stat_area)

        self.lbl_stat_distance = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("Total distance:"), self.lbl_stat_distance)

        self.lbl_stat_photos = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("Total photos:"), self.lbl_stat_photos)

        self.lbl_stat_time = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("Estimated flight time:"), self.lbl_stat_time)

        self.lbl_stat_interval = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("Interval between photos:"), self.lbl_stat_interval)

        self.lbl_stat_wp_qgc = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("QGC waypoints:"), self.lbl_stat_wp_qgc)

        self.lbl_stat_wp_litchi = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("Litchi waypoints:"), self.lbl_stat_wp_litchi)

        self.lbl_stat_wp_dji = QLabel("-", grp_stats)
        lay_stats.addRow(self.tr("DJI waypoints:"), self.lbl_stat_wp_dji)

        self.lbl_stat_warning = QLabel("", grp_stats)
        self.lbl_stat_warning.setStyleSheet("color: red; font-weight: bold;")
        self.lbl_stat_warning.setWordWrap(True)
        self.lbl_stat_warning.setVisible(False)
        lay_stats.addRow(self.lbl_stat_warning)

        main_layout.addWidget(grp_stats)

        # 7. Export to
        grp_export = QGroupBox(self.tr("Export to"), main_widget)
        lay_export = QFormLayout(grp_export)

        self.cmb_trigger_mode = QComboBox(grp_export)
        self.cmb_trigger_mode.addItems(
            [self.tr("By distance"), self.tr("By time"), self.tr("By photo")]
        )
        self.cmb_trigger_mode.currentIndexChanged.connect(self._on_grid_param_changed)
        lay_export.addRow(self.tr("Trigger mode:"), self.cmb_trigger_mode)

        self.spn_speed = QDoubleSpinBox(grp_export)
        self.spn_speed.setRange(0.1, 100.0)
        self.spn_speed.setValue(5.0)
        self.spn_speed.setSuffix(" m/s")
        self.spn_speed.setDecimals(1)
        self.spn_speed.valueChanged.connect(self._on_grid_param_changed)
        lay_export.addRow(self.tr("Flight speed:"), self.spn_speed)

        self.spn_gimbal_pitch = QDoubleSpinBox(grp_export)
        self.spn_gimbal_pitch.setRange(-90.0, 20.0)
        self.spn_gimbal_pitch.setValue(-90.0)
        self.spn_gimbal_pitch.setSuffix("°")
        self.spn_gimbal_pitch.setDecimals(1)
        self.spn_gimbal_pitch.valueChanged.connect(self._on_grid_param_changed)
        lay_export.addRow(self.tr("Gimbal angle:"), self.spn_gimbal_pitch)

        self.spn_waypoint_wait = QDoubleSpinBox(grp_export)
        self.spn_waypoint_wait.setRange(0.0, 3600.0)
        self.spn_waypoint_wait.setValue(0.0)
        self.spn_waypoint_wait.setSuffix(" s")
        self.spn_waypoint_wait.setDecimals(1)
        self.spn_waypoint_wait.valueChanged.connect(self._on_grid_param_changed)
        lay_export.addRow(self.tr("Waypoint wait:"), self.spn_waypoint_wait)

        self.btn_export_litchi = QPushButton(self.tr("Export Litchi (.csv)…"), grp_export)
        self.btn_export_litchi.clicked.connect(self.export_litchi)
        lay_export.addRow(self.btn_export_litchi)

        self.btn_export_kml = QPushButton(self.tr("Export classic Litchi Hub (.kml)…"), grp_export)
        self.btn_export_kml.setToolTip(
            self.tr(
                'KML for flylitchi.com/hub → Import, with "Add take photo action" checked. '
                "Does not carry heading, gimbal, or speed — use the .csv for that."
            )
        )
        self.btn_export_kml.clicked.connect(self.export_kml)
        lay_export.addRow(self.btn_export_kml)

        self.btn_export_dji = QPushButton(
            self.tr("Export DJI Fly / Litchi Hub 2 (.kmz)…"), grp_export
        )
        self.btn_export_dji.setToolTip(
            self.tr(
                "KMZ WPML: opens in DJI Fly and is imported as a mission by Litchi Hub 2 "
                "(hub.flylitchi.com → Import mission). The classic hub (flylitchi.com/hub) "
                "does NOT read .kmz — for that use the .csv or the .kml."
            )
        )
        self.btn_export_dji.clicked.connect(self.export_dji)
        lay_export.addRow(self.btn_export_dji)

        main_layout.addWidget(grp_export)

        # 8. Action Buttons
        self.btn_generate = QPushButton(self.tr("Generate Flight Grid"), main_widget)
        self.btn_generate.clicked.connect(self.generate_grid)
        main_layout.addWidget(self.btn_generate)

        self.btn_export_plan = QPushButton(self.tr("Export .plan…"), main_widget)
        self.btn_export_plan.clicked.connect(self.export_plan)
        main_layout.addWidget(self.btn_export_plan)

        self.btn_add_layers = QPushButton(self.tr("Add layers to project"), main_widget)
        self.btn_add_layers.clicked.connect(self.add_layers_to_project)
        main_layout.addWidget(self.btn_add_layers)

        main_layout.addStretch()

        self.scroll_area = QScrollArea(self)
        self.scroll_area.setWidgetResizable(True)
        self.scroll_area.setFrameShape(NO_FRAME)
        self.scroll_area.setWidget(main_widget)
        self.setWidget(self.scroll_area)

        # Initialize layer and initial calculation
        self._on_layer_changed()
        self._on_camera_changed()

    def get_current_camera_spec(self) -> CameraSpec | None:
        """Return currently selected CameraSpec, resolving custom manual camera if active."""
        idx = self.cmb_camera.currentIndex()
        if idx < 0 or idx >= len(self.cameras):
            return None

        spec = self.cameras[idx]
        if spec.canonicalName == CUSTOM_CAMERA_NAME:
            return CameraSpec(
                canonicalName=CUSTOM_CAMERA_NAME,
                sensorWidth=self.spn_sensor_w.value(),
                sensorHeight=self.spn_sensor_h.value(),
                imageWidth=self.spn_img_w.value(),
                imageHeight=self.spn_img_h.value(),
                focalLength=self.spn_focal.value(),
            )
        return spec

    def _on_layer_changed(self) -> None:
        """Update feature combo box when selected map layer changes."""
        self.cmb_feature.blockSignals(True)
        self.cmb_feature.clear()
        self.cmb_feature.addItem(self.tr("All features"), None)

        layer = self.cmb_layer.currentLayer()
        if isinstance(layer, QgsVectorLayer) and layer.isValid():
            fields = [f.name() for f in layer.fields()]
            name_field = "name" if "name" in fields else ("Nome" if "Nome" in fields else None)
            for feat in layer.getFeatures():
                feat_id = feat.id()
                if name_field and feat.attribute(name_field):
                    label = f"{feat.attribute(name_field)} (id: {feat_id})"
                else:
                    label = self.tr("Feature {feat_id}").format(feat_id=feat_id)
                self.cmb_feature.addItem(label, feat_id)
        self.cmb_feature.blockSignals(False)
        self._update_preview()

    def _on_feature_changed(self, idx: int = 0) -> None:
        """Handle feature selection change."""
        self._update_preview()

    def _on_grid_param_changed(self, val: Any = None) -> None:
        """Handle grid parameter change (angle, turnaround, entry location, refly)."""
        self._update_preview()

    def _on_camera_changed(self) -> None:
        """Handle camera selection change."""
        spec = self.get_current_camera_spec()
        is_custom = spec is not None and spec.canonicalName == CUSTOM_CAMERA_NAME
        self.grp_custom_camera.setVisible(is_custom)
        self._recalculate_locked_pair()

    def _on_camera_param_changed(self) -> None:
        """Handle changes in custom camera numeric inputs."""
        self._recalculate_locked_pair()

    def _on_mode_changed(self, id_: int, checked: bool) -> None:
        """Handle altitude/GSD mode toggle."""
        if not checked:
            return

        is_altitude_mode = self.rad_mode_altitude.isChecked()
        self.spn_altitude.setEnabled(is_altitude_mode)
        self.spn_gsd.setEnabled(not is_altitude_mode)
        self._recalculate_locked_pair()

    def _on_altitude_changed(self, val: float) -> None:
        """Handle altitude value change when in altitude mode."""
        if self._updating_lock or not self.rad_mode_altitude.isChecked():
            return
        self._recalculate_locked_pair()

    def _on_gsd_changed(self, val: float) -> None:
        """Handle GSD value change when in GSD mode."""
        if self._updating_lock or not self.rad_mode_gsd.isChecked():
            return
        self._recalculate_locked_pair()

    def _on_overlap_changed(self, val: float = 0.0) -> None:
        """Handle overlap value changes."""
        self._recalculate_locked_pair()

    def _recalculate_locked_pair(self) -> None:
        """Recalculate live altitude ↔ GSD pair and adjusted footprints using CameraCalc core."""
        if self._updating_lock:
            return

        self._updating_lock = True
        try:
            spec = self.get_current_camera_spec()
            is_alt_mode = self.rad_mode_altitude.isChecked()
            alt_val = self.spn_altitude.value()
            gsd_val = self.spn_gsd.value()
            overlap_side = self.spn_overlap_side.value()
            overlap_frontal = self.spn_overlap_frontal.value()

            calc = CameraCalc(
                spec=spec,
                value_set_is_distance=is_alt_mode,
                distance_to_surface=alt_val,
                image_density=gsd_val,
                side_overlap=overlap_side,
                frontal_overlap=overlap_frontal,
            )

            if calc.recalculate():
                self.camera_calc = calc
                if is_alt_mode:
                    self.spn_gsd.setValue(calc.image_density)
                else:
                    self.spn_altitude.setValue(calc.distance_to_surface)

                self.lbl_footprint.setText(
                    f"{calc.adjusted_footprint_side:.2f} m x {calc.adjusted_footprint_frontal:.2f} m"
                )
            else:
                self.lbl_footprint.setText("N/A")
        finally:
            self._updating_lock = False
            self._update_preview()

    def save_settings(self, project: QgsProject | None = None) -> None:
        """Persist current dock parameters to QgsProject."""
        params = self.get_parameters()
        params["MODE_ALTITUDE"] = self.rad_mode_altitude.isChecked()
        save_project_settings(params, project)

    def load_settings(self, project: QgsProject | None = None) -> None:
        """Load and restore dock parameters from QgsProject entries."""
        settings = load_project_settings(project)
        self._updating_lock = True
        try:
            layer_id = settings.get("LAYER_ID")
            if layer_id:
                proj = project or QgsProject.instance()
                layer = proj.mapLayer(layer_id)
                if isinstance(layer, QgsVectorLayer) and layer.isValid():
                    self.cmb_layer.setLayer(layer)
                    self._on_layer_changed()

            feat_id = settings.get("FEATURE_ID")
            if feat_id is not None:
                idx = self.cmb_feature.findData(feat_id)
                if idx >= 0:
                    self.cmb_feature.setCurrentIndex(idx)

            cam_idx = settings.get("CAMERA", 0)
            if 0 <= cam_idx < self.cmb_camera.count():
                self.cmb_camera.setCurrentIndex(cam_idx)

            self.spn_sensor_w.setValue(settings.get("SENSOR_WIDTH", 35.9))
            self.spn_sensor_h.setValue(settings.get("SENSOR_HEIGHT", 24.0))
            self.spn_img_w.setValue(settings.get("IMAGE_WIDTH", 7952))
            self.spn_img_h.setValue(settings.get("IMAGE_HEIGHT", 5304))
            self.spn_focal.setValue(settings.get("FOCAL_LENGTH", 35.0))

            mode_alt = settings.get("MODE_ALTITUDE", True)
            if mode_alt:
                self.rad_mode_altitude.setChecked(True)
                self.spn_altitude.setEnabled(True)
                self.spn_gsd.setEnabled(False)
            else:
                self.rad_mode_gsd.setChecked(True)
                self.spn_altitude.setEnabled(False)
                self.spn_gsd.setEnabled(True)

            self.spn_altitude.setValue(settings.get("ALTITUDE", 100.0))
            self.spn_gsd.setValue(settings.get("GSD", 2.0))

            self.spn_overlap_side.setValue(settings.get("OVERLAP_SIDE", 70.0))
            self.spn_overlap_frontal.setValue(settings.get("OVERLAP_FRONTAL", 70.0))
            self.spn_angle.setValue(settings.get("ANGLE", 0.0))
            self.spn_turnaround.setValue(settings.get("TURNAROUND", 0.0))

            entry_loc = settings.get("ENTRY_LOCATION", 0)
            if 0 <= entry_loc < self.cmb_entry_location.count():
                self.cmb_entry_location.setCurrentIndex(entry_loc)

            self.chk_refly.setChecked(settings.get("REFLY", False))

            elev_layer_id = settings.get("ELEVATION_LAYER_ID")
            if elev_layer_id:
                proj = project or QgsProject.instance()
                elev_layer = proj.mapLayer(elev_layer_id)
                if isinstance(elev_layer, QgsRasterLayer) and elev_layer.isValid():
                    self.cmb_elevation_layer.setLayer(elev_layer)

            self.spn_tolerance.setValue(settings.get("TOLERANCE", 10.0))

            trig_mode = settings.get("TRIGGER_MODE", 0)
            if 0 <= trig_mode < self.cmb_trigger_mode.count():
                self.cmb_trigger_mode.setCurrentIndex(trig_mode)

            self.spn_speed.setValue(settings.get("SPEED", 5.0))
            self.spn_gimbal_pitch.setValue(settings.get("GIMBAL_PITCH", -90.0))
            self.spn_waypoint_wait.setValue(settings.get("WAYPOINT_WAIT", 0.0))
        finally:
            self._updating_lock = False

        self._recalculate_locked_pair()

    def _update_preview(self) -> None:
        """Update flight preview memory layers and flight statistics with current parameters."""
        if self._updating_lock:
            return
        params = self.get_parameters()
        self.preview_manager.update_preview(params)
        self._update_stats()
        self.save_settings()

    def _update_stats(self) -> None:
        """Update flight statistics panel labels based on current preview layers."""
        area_layer = self.preview_manager.area_layer
        line_layer = self.preview_manager.line_layer

        if (
            area_layer is None
            or not area_layer.isValid()
            or line_layer is None
            or not line_layer.isValid()
            or area_layer.featureCount() == 0
        ):
            self.lbl_stat_area.setText("-")
            self.lbl_stat_distance.setText("-")
            self.lbl_stat_photos.setText("-")
            self.lbl_stat_time.setText("-")
            self.lbl_stat_interval.setText("-")
            self.lbl_stat_wp_qgc.setText("-")
            self.lbl_stat_wp_litchi.setText("-")
            self.lbl_stat_wp_dji.setText("-")
            self.lbl_stat_warning.setText("")
            self.lbl_stat_warning.setVisible(False)
            return

        is_geodetic = area_layer.crs().isGeographic()

        polygon_points_list: list[list[tuple[float, float]]] = []
        for feat in area_layer.getFeatures():
            geom = feat.geometry()
            if geom.isEmpty():
                continue
            if geom.isMultipart():
                multi = geom.asMultiPolygon()
                for poly in multi:
                    if poly and poly[0]:
                        pts = [
                            (float(p.y()), float(p.x()))
                            if is_geodetic
                            else (float(p.x()), float(p.y()))
                            for p in poly[0]
                        ]
                        polygon_points_list.append(pts)
            else:
                poly = geom.asPolygon()
                if poly and poly[0]:
                    pts = [
                        (float(p.y()), float(p.x()))
                        if is_geodetic
                        else (float(p.x()), float(p.y()))
                        for p in poly[0]
                    ]
                    polygon_points_list.append(pts)

        transects: list[list[tuple[float, float]]] = []
        for feat in line_layer.getFeatures():
            geom = feat.geometry()
            if geom.isEmpty():
                continue
            if geom.isMultipart():
                multi = geom.asMultiPolyline()
                for line in multi:
                    if line:
                        pts = [
                            (float(p.y()), float(p.x()))
                            if is_geodetic
                            else (float(p.x()), float(p.y()))
                            for p in line
                        ]
                        transects.append(pts)
            else:
                line = geom.asPolyline()
                if line:
                    pts = [
                        (float(p.y()), float(p.x()))
                        if is_geodetic
                        else (float(p.x()), float(p.y()))
                        for p in line
                    ]
                    transects.append(pts)

        spec = self.get_current_camera_spec()
        min_interval = spec.minTriggerInterval if spec else 0.0
        trigger_dist = self.camera_calc.trigger_distance if self.camera_calc else 0.0

        stats = calculate_flight_stats(
            polygon_points=polygon_points_list[0] if polygon_points_list else None,
            transects=transects,
            trigger_distance=trigger_dist,
            flight_speed=15.0,
            camera_trigger_in_turnaround=True,
            turnaround_distance=self.spn_turnaround.value(),
            min_trigger_interval=min_interval,
            is_geodetic=is_geodetic,
        )

        if len(polygon_points_list) > 1:
            stats.polygon_area = sum(
                calculate_polygon_area(pts, is_geodetic=is_geodetic) for pts in polygon_points_list
            )

        if stats.polygon_area >= 10000.0:
            ha = stats.polygon_area / 10000.0
            self.lbl_stat_area.setText(f"{stats.polygon_area:.2f} m² ({ha:.2f} ha)")
        else:
            self.lbl_stat_area.setText(f"{stats.polygon_area:.2f} m²")

        if stats.total_distance >= 1000.0:
            km = stats.total_distance / 1000.0
            self.lbl_stat_distance.setText(f"{stats.total_distance:.2f} m ({km:.2f} km)")
        else:
            self.lbl_stat_distance.setText(f"{stats.total_distance:.2f} m")

        self.lbl_stat_photos.setText(str(stats.camera_shots))

        minutes = int(stats.flight_time) // 60
        seconds = int(stats.flight_time) % 60
        if minutes >= 60:
            hours = minutes // 60
            minutes = minutes % 60
            self.lbl_stat_time.setText(f"{hours}h {minutes:02d}m {seconds:02d}s")
        else:
            self.lbl_stat_time.setText(f"{minutes}m {seconds:02d}s")

        self.lbl_stat_interval.setText(f"{stats.time_between_shots:.2f} s")

        trig_mode_idx = self.cmb_trigger_mode.currentIndex()
        trig_mode_map = {0: "POR_DISTANCIA", 1: "POR_TEMPO", 2: "POR_FOTO"}
        trig_mode_str = trig_mode_map.get(trig_mode_idx, "POR_DISTANCIA")
        speed = self.spn_speed.value()

        if trig_mode_str == "POR_TEMPO":
            route_trig_dist = (
                self.camera_calc.trigger_distance / speed
                if (self.camera_calc and speed > 0.0)
                else 0.0
            )
        else:
            route_trig_dist = self.camera_calc.trigger_distance if self.camera_calc else 0.0

        if is_geodetic:
            route_transects = transects
        else:
            to_wgs84 = QgsCoordinateTransform(
                line_layer.crs(),
                QgsCoordinateReferenceSystem("EPSG:4326"),
                QgsProject.instance(),
            )
            route_transects = []
            for tr in transects:
                pts = []
                for x, y in tr:
                    projected = to_wgs84.transform(x, y)
                    pts.append((projected.y(), projected.x()))
                route_transects.append(pts)

        route = route_from_transects(
            route_transects,
            altitude=self.spn_altitude.value(),
            trigger_distance=route_trig_dist,
            trigger_mode=trig_mode_str,
            flight_speed=speed,
            gimbal_pitch=self.spn_gimbal_pitch.value(),
            max_waypoints=0,
        )
        wp_count = len(route.waypoints)

        route_qgc = route_from_transects(
            route_transects,
            altitude=self.spn_altitude.value(),
            trigger_distance=route_trig_dist,
            trigger_mode="POR_DISTANCIA",
            flight_speed=speed,
            gimbal_pitch=self.spn_gimbal_pitch.value(),
            max_waypoints=0,
        )
        self.lbl_stat_wp_qgc.setText(str(len(route_qgc.waypoints)))

        if wp_count > 99:
            self.lbl_stat_wp_litchi.setText(
                self.tr("{count} (exceeds limit: 99)").format(count=wp_count)
            )
        else:
            self.lbl_stat_wp_litchi.setText(str(wp_count))

        if wp_count > 200:
            self.lbl_stat_wp_dji.setText(
                self.tr("{count} (exceeds limit: 200)").format(count=wp_count)
            )
        else:
            self.lbl_stat_wp_dji.setText(str(wp_count))

        warnings: list[str] = []
        if stats.is_interval_too_short and stats.warning_message:
            warnings.append(stats.warning_message)
        if wp_count > 99:
            warnings.append(
                self.tr("Waypoint count ({count}) exceeds the Litchi limit (99).").format(
                    count=wp_count
                )
            )
        if wp_count > 200:
            warnings.append(
                self.tr("Waypoint count ({count}) exceeds the DJI limit (200).").format(
                    count=wp_count
                )
            )

        if warnings:
            self.lbl_stat_warning.setText("\n".join(warnings))
            self.lbl_stat_warning.setVisible(True)
        else:
            self.lbl_stat_warning.setText("")
            self.lbl_stat_warning.setVisible(False)

    def unload(self) -> None:
        """Remove flight preview layers and clean up dock widget."""
        self.save_settings()
        self.preview_manager.clear()

    def closeEvent(self, event: Any) -> None:
        """Handle dock widget close event."""
        self.unload()
        super().closeEvent(event)

    def get_parameters(self) -> dict[str, Any]:
        """Collect and return dictionary of parameters for flight grid algorithm."""
        layer = self.cmb_layer.currentLayer()
        feat_id = self.cmb_feature.currentData()

        is_altitude_mode = self.rad_mode_altitude.isChecked()
        altitude_val = self.spn_altitude.value()
        gsd_val = 0.0 if is_altitude_mode else self.spn_gsd.value()

        return {
            "INPUT": layer,
            "FEATURE_ID": feat_id,
            "ELEVATION_LAYER": self.cmb_elevation_layer.currentLayer(),
            "TOLERANCE": self.spn_tolerance.value(),
            "TERRAIN_TOLERANCE": self.spn_tolerance.value(),
            "CAMERA": self.cmb_camera.currentIndex(),
            "ALTITUDE": altitude_val,
            "GSD": gsd_val,
            "OVERLAP_SIDE": self.spn_overlap_side.value(),
            "OVERLAP_FRONTAL": self.spn_overlap_frontal.value(),
            "ANGLE": self.spn_angle.value(),
            "TURNAROUND": self.spn_turnaround.value(),
            "ENTRY_LOCATION": self.cmb_entry_location.currentIndex(),
            "REFLY": self.chk_refly.isChecked(),
            "SENSOR_WIDTH": self.spn_sensor_w.value(),
            "SENSOR_HEIGHT": self.spn_sensor_h.value(),
            "IMAGE_WIDTH": self.spn_img_w.value(),
            "IMAGE_HEIGHT": self.spn_img_h.value(),
            "FOCAL_LENGTH": self.spn_focal.value(),
            "TRIGGER_MODE": self.cmb_trigger_mode.currentIndex(),
            "SPEED": self.spn_speed.value(),
            "GIMBAL_PITCH": self.spn_gimbal_pitch.value(),
            "WAYPOINT_WAIT": self.spn_waypoint_wait.value(),
        }

    def generate_grid(self) -> None:
        """Trigger grid generation by building parameters, emitting signal, and executing algorithm if in QGIS environment."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return

        self.gridGenerated.emit(params)

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para gerar_grade_voo.")
            return

        try:
            proc_params = dict(params)
            feat_id = proc_params.pop("FEATURE_ID", None)

            if feat_id is not None:
                layer.selectByIds([feat_id])
                proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                    layer.id(), selectedFeaturesOnly=True
                )
            else:
                proc_params["INPUT"] = layer

            proc_params["OUTPUT"] = "memory:"
            processing.run("qgc4qgis:gerar_grade_voo", proc_params)
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:gerar_grade_voo: {e}")

    def export_plan(self, file_path: str | None = None) -> str | None:
        """Export flight plan to a QGroundControl (.plan) file using ExportPlanAlgorithm (Phase 3)."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return None

        if not file_path:
            file_path, _ = QFileDialog.getSaveFileName(
                self,
                self.tr("Export QGC Plan"),
                "",
                self.tr("QGroundControl Plan (*.plan);;All Files (*)"),
            )

        if not file_path:
            return None

        proc_params = dict(params)
        feat_id = proc_params.pop("FEATURE_ID", None)

        if feat_id is not None:
            layer.selectByIds([feat_id])
            proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                layer.id(), selectedFeaturesOnly=True
            )
        else:
            proc_params["INPUT"] = layer

        proc_params["OUTPUT"] = file_path
        self.planExported.emit(proc_params)

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para exportar_plano_qgc.")
            return file_path

        try:
            processing.run("qgc4qgis:exportar_plano_qgc", proc_params)
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:exportar_plano_qgc: {e}")

        return file_path

    def export_litchi(self, file_path: str | None = None) -> str | None:
        """Export flight plan to a Litchi (.csv) mission file via ExportLitchiAlgorithm."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return None

        if not file_path:
            file_path, _ = QFileDialog.getSaveFileName(
                self,
                self.tr("Export Litchi CSV"),
                "",
                self.tr("Litchi Mission (*.csv);;All Files (*)"),
            )

        if not file_path:
            return None

        proc_params = dict(params)
        feat_id = proc_params.pop("FEATURE_ID", None)

        if feat_id is not None:
            layer.selectByIds([feat_id])
            proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                layer.id(), selectedFeaturesOnly=True
            )
        else:
            proc_params["INPUT"] = layer

        proc_params["OUTPUT"] = file_path
        self.litchiExported.emit(proc_params)

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para exportar_litchi_csv.")
            return file_path

        try:
            processing.run("qgc4qgis:exportar_litchi_csv", proc_params)
        except QgsProcessingException as e:
            if self.isVisible():
                QMessageBox.critical(self, self.tr("Litchi export error"), str(e))
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:exportar_litchi_csv: {e}")

        return file_path

    def export_kml(self, file_path: str | None = None) -> str | None:
        """Export flight plan to a Litchi Mission Hub KML file via ExportLitchiKmlAlgorithm."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return None

        if not file_path:
            file_path, _ = QFileDialog.getSaveFileName(
                self,
                self.tr("Export Litchi Mission Hub KML"),
                "",
                self.tr("Litchi Mission Hub KML (*.kml);;All Files (*)"),
            )

        if not file_path:
            return None

        proc_params = dict(params)
        feat_id = proc_params.pop("FEATURE_ID", None)

        if feat_id is not None:
            layer.selectByIds([feat_id])
            proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                layer.id(), selectedFeaturesOnly=True
            )
        else:
            proc_params["INPUT"] = layer

        proc_params["OUTPUT"] = file_path
        self.kmlExported.emit(proc_params)

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para exportar_litchi_kml.")
            return file_path

        try:
            processing.run("qgc4qgis:exportar_litchi_kml", proc_params)
        except QgsProcessingException as e:
            if self.isVisible():
                QMessageBox.critical(self, self.tr("KML export error"), str(e))
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:exportar_litchi_kml: {e}")

        return file_path

    def export_dji(self, file_path: str | None = None) -> str | None:
        """Export flight plan to a DJI Fly (.kmz) mission file via ExportDjiAlgorithm."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return None

        if not file_path:
            file_path, _ = QFileDialog.getSaveFileName(
                self,
                self.tr("Export DJI Fly KMZ"),
                "",
                self.tr("DJI Fly Mission (*.kmz);;All Files (*)"),
            )

        if not file_path:
            return None

        proc_params = dict(params)
        feat_id = proc_params.pop("FEATURE_ID", None)

        if feat_id is not None:
            layer.selectByIds([feat_id])
            proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                layer.id(), selectedFeaturesOnly=True
            )
        else:
            proc_params["INPUT"] = layer

        proc_params["OUTPUT"] = file_path
        self.djiExported.emit(proc_params)

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para exportar_dji_kmz.")
            return file_path

        try:
            processing.run("qgc4qgis:exportar_dji_kmz", proc_params)
        except QgsProcessingException as e:
            if self.isVisible():
                QMessageBox.critical(self, self.tr("DJI export error"), str(e))
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:exportar_dji_kmz: {e}")

        return file_path

    def add_layers_to_project(self) -> None:
        """Generate flight survey grid and load output layer into the QGIS project (Phase 4)."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return

        self.gridGenerated.emit(params)

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para gerar_grade_voo.")
            return

        try:
            proc_params = dict(params)
            feat_id = proc_params.pop("FEATURE_ID", None)

            if feat_id is not None:
                layer.selectByIds([feat_id])
                proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                    layer.id(), selectedFeaturesOnly=True
                )
            else:
                proc_params["INPUT"] = layer

            layer_name = self.tr("Flight Grid")
            proc_params["OUTPUT"] = f"memory:{layer_name}"
            try:
                processing.runAndLoadResults("qgc4qgis:gerar_grade_voo", proc_params)
            except AttributeError:
                processing.run("qgc4qgis:gerar_grade_voo", proc_params)
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:gerar_grade_voo: {e}")

    def download_dem(self) -> None:
        """Download Copernicus DEM for selected polygon layer extent and add to project."""
        params = self.get_parameters()
        layer = params.get("INPUT")

        if layer is None or not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Select a valid polygon layer."))
            return

        try:
            import processing
        except ImportError:
            log_warning("Módulo processing indisponível para baixar_dem_copernicus.")
            return

        try:
            proc_params: dict[str, Any] = {}
            feat_id = params.get("FEATURE_ID")

            if feat_id is not None:
                layer.selectByIds([feat_id])
                proc_params["INPUT"] = QgsProcessingFeatureSourceDefinition(
                    layer.id(), selectedFeaturesOnly=True
                )
            else:
                proc_params["INPUT"] = layer

            proc_params["OUTPUT"] = "TEMPORARY_OUTPUT"

            res = processing.run("qgc4qgis:baixar_dem_copernicus", proc_params)
            out_path = res.get("OUTPUT") if isinstance(res, dict) else None

            if out_path:
                dem_layer = QgsRasterLayer(out_path, self.tr("Copernicus DEM"))
                if dem_layer.isValid():
                    meta = dem_layer.metadata()
                    meta.setAbstract(ATTRIBUTION)
                    dem_layer.setMetadata(meta)
                    QgsProject.instance().addMapLayer(dem_layer)
                    self.cmb_elevation_layer.setLayer(dem_layer)
        except Exception as e:
            log_error(f"Erro ao executar qgc4qgis:baixar_dem_copernicus: {e}")
