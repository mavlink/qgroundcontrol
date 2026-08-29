"""Processing algorithm to export a QGroundControl (.plan) file."""

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsGeometry,
    QgsProcessing,
    QgsProcessingAlgorithm,
    QgsProcessingException,
    QgsProcessingParameterBoolean,
    QgsProcessingParameterEnum,
    QgsProcessingParameterFeatureSource,
    QgsProcessingParameterFileDestination,
    QgsProcessingParameterNumber,
    QgsProcessingParameterRasterLayer,
)
from qgis.PyQt.QtCore import QCoreApplication

from qgc4qgis.core.cameracalc import CameraCalc
from qgc4qgis.core.cameras import CUSTOM_CAMERA_NAME, CameraSpec, load_cameras
from qgc4qgis.core.geo import AEQDProjection, geodesic_distance
from qgc4qgis.core.missionitems import (
    DistanceMode,
    create_waypoint_item,
    expand_transects_to_mission_items,
)
from qgc4qgis.core.planfile import build_survey_item, save_plan_file
from qgc4qgis.core.stats import calculate_camera_shots
from qgc4qgis.core.survey import generate_survey_transects
from qgc4qgis.core.terrain import adjust_terrain_flight_path
from qgc4qgis.processing.alg_survey_grid import extract_polygons


class ExportPlanAlgorithm(QgsProcessingAlgorithm):
    """QGIS Processing Algorithm to export QGroundControl (.plan) files."""

    def tr(self, string: str) -> str:
        """Return the translated string using the class context."""
        return QCoreApplication.translate("ExportPlanAlgorithm", string)

    INPUT = "INPUT"
    CAMERA = "CAMERA"
    ALTITUDE = "ALTITUDE"
    GSD = "GSD"
    OVERLAP_SIDE = "OVERLAP_SIDE"
    OVERLAP_FRONTAL = "OVERLAP_FRONTAL"
    ANGLE = "ANGLE"
    TURNAROUND = "TURNAROUND"
    ENTRY_LOCATION = "ENTRY_LOCATION"
    REFLY = "REFLY"
    SENSOR_WIDTH = "SENSOR_WIDTH"
    SENSOR_HEIGHT = "SENSOR_HEIGHT"
    IMAGE_WIDTH = "IMAGE_WIDTH"
    IMAGE_HEIGHT = "IMAGE_HEIGHT"
    FOCAL_LENGTH = "FOCAL_LENGTH"
    CRUISE_SPEED = "CRUISE_SPEED"
    HOVER_SPEED = "HOVER_SPEED"
    FIRMWARE_TYPE = "FIRMWARE_TYPE"
    VEHICLE_TYPE = "VEHICLE_TYPE"
    ELEVATION_LAYER = "ELEVATION_LAYER"
    TOLERANCE = "TOLERANCE"
    OUTPUT = "OUTPUT"

    def name(self) -> str:
        """Return unique algorithm name."""
        return "exportar_plano_qgc"

    def displayName(self) -> str:
        """Return localized human-readable algorithm name."""
        return self.tr("Export QGC plan (.plan)")

    def group(self) -> str:
        """Return localized group name."""
        return self.tr("Flight Planning")

    def groupId(self) -> str:
        """Return unique group identifier."""
        return "planejamento_voo"

    def createInstance(self) -> "ExportPlanAlgorithm":
        """Create new instance of algorithm."""
        return ExportPlanAlgorithm()

    def shortHelpString(self) -> str:
        """Return short help text for algorithm GUI."""
        return self.tr(
            "Exports a mission plan in QGroundControl format (.plan) "
            "from coverage polygons or flight grid lines."
        )

    def initAlgorithm(self, config=None) -> None:
        """Define algorithm parameters and outputs."""
        cameras = load_cameras()
        camera_names = [c.canonicalName for c in cameras]

        self.addParameter(
            QgsProcessingParameterFeatureSource(
                self.INPUT,
                self.tr("Input layer (Polygons or Lines)"),
                [QgsProcessing.TypeVectorPolygon, QgsProcessing.TypeVectorLine],
            )
        )

        self.addParameter(
            QgsProcessingParameterEnum(
                self.CAMERA,
                self.tr("Camera"),
                options=camera_names,
                defaultValue=0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.ALTITUDE,
                self.tr("Flight altitude (m)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=100.0,
                minValue=0.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.GSD,
                self.tr("GSD (cm/px) - if > 0, overrides/calculates altitude"),
                QgsProcessingParameterNumber.Double,
                defaultValue=0.0,
                minValue=0.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.OVERLAP_SIDE,
                self.tr("Side overlap (%)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=70.0,
                minValue=0.0,
                maxValue=99.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.OVERLAP_FRONTAL,
                self.tr("Frontal overlap (%)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=70.0,
                minValue=0.0,
                maxValue=99.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.ANGLE,
                self.tr("Grid angle (degrees)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=0.0,
                minValue=-180.0,
                maxValue=180.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.TURNAROUND,
                self.tr("Turnaround distance (m)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=0.0,
                minValue=0.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterEnum(
                self.ENTRY_LOCATION,
                self.tr("Entry point"),
                options=["Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right"],
                defaultValue=0,
            )
        )

        self.addParameter(
            QgsProcessingParameterBoolean(
                self.REFLY,
                self.tr("Cross grid (Refly 90°)"),
                defaultValue=False,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.SENSOR_WIDTH,
                self.tr("Manual camera: Sensor width (mm)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=35.9,
                minValue=0.0,
                optional=True,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.SENSOR_HEIGHT,
                self.tr("Manual camera: Sensor height (mm)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=24.0,
                minValue=0.0,
                optional=True,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.IMAGE_WIDTH,
                self.tr("Manual camera: Image width (px)"),
                QgsProcessingParameterNumber.Integer,
                defaultValue=7952,
                minValue=0,
                optional=True,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.IMAGE_HEIGHT,
                self.tr("Manual camera: Image height (px)"),
                QgsProcessingParameterNumber.Integer,
                defaultValue=5304,
                minValue=0,
                optional=True,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.FOCAL_LENGTH,
                self.tr("Manual camera: Focal length (mm)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=35.0,
                minValue=0.0,
                optional=True,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.CRUISE_SPEED,
                self.tr("Cruise speed (m/s)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=15.0,
                minValue=0.1,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.HOVER_SPEED,
                self.tr("Hover speed (m/s)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=5.0,
                minValue=0.1,
            )
        )

        self.addParameter(
            QgsProcessingParameterEnum(
                self.FIRMWARE_TYPE,
                self.tr("Firmware Type"),
                options=["PX4 (12)", "ArduPilot (3)"],
                defaultValue=0,
            )
        )

        self.addParameter(
            QgsProcessingParameterEnum(
                self.VEHICLE_TYPE,
                self.tr("Vehicle Type"),
                options=["Multirotor (2)", self.tr("Fixed Wing (1)"), "VTOL (19)"],
                defaultValue=0,
            )
        )

        self.addParameter(
            QgsProcessingParameterRasterLayer(
                self.ELEVATION_LAYER,
                self.tr("Elevation layer (DEM) — if set, exports in above-terrain mode"),
                optional=True,
                defaultValue=None,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.TOLERANCE,
                self.tr("Terrain tolerance (m)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=10.0,
                minValue=0.1,
            )
        )

        self.addParameter(
            QgsProcessingParameterFileDestination(
                self.OUTPUT,
                self.tr("Output file (.plan)"),
                fileFilter="QGroundControl Plan (*.plan);;JSON (*.json)",
            )
        )

    def processAlgorithm(self, parameters, context, feedback):
        """Execute QGC .plan export algorithm logic."""
        source = self.parameterAsSource(parameters, self.INPUT, context)
        if source is None:
            raise QgsProcessingException(self.invalidSourceError(parameters, self.INPUT))

        output_file = self.parameterAsFileOutput(parameters, self.OUTPUT, context)
        if not output_file:
            raise QgsProcessingException(self.tr("Output file path not specified."))

        camera_idx = self.parameterAsEnum(parameters, self.CAMERA, context)
        cameras = load_cameras()
        if 0 <= camera_idx < len(cameras):
            spec = cameras[camera_idx]
        else:
            spec = cameras[0]

        if spec.canonicalName == CUSTOM_CAMERA_NAME:
            sensor_w = self.parameterAsDouble(parameters, self.SENSOR_WIDTH, context)
            sensor_h = self.parameterAsDouble(parameters, self.SENSOR_HEIGHT, context)
            img_w = self.parameterAsInt(parameters, self.IMAGE_WIDTH, context)
            img_h = self.parameterAsInt(parameters, self.IMAGE_HEIGHT, context)
            focal_l = self.parameterAsDouble(parameters, self.FOCAL_LENGTH, context)
            spec = CameraSpec(
                canonicalName=CUSTOM_CAMERA_NAME,
                sensorWidth=sensor_w,
                sensorHeight=sensor_h,
                imageWidth=img_w,
                imageHeight=img_h,
                focalLength=focal_l,
            )

        altitude = self.parameterAsDouble(parameters, self.ALTITUDE, context)
        gsd = self.parameterAsDouble(parameters, self.GSD, context)
        side_overlap = self.parameterAsDouble(parameters, self.OVERLAP_SIDE, context)
        frontal_overlap = self.parameterAsDouble(parameters, self.OVERLAP_FRONTAL, context)
        grid_angle = self.parameterAsDouble(parameters, self.ANGLE, context)
        turnaround = self.parameterAsDouble(parameters, self.TURNAROUND, context)
        entry_loc = self.parameterAsEnum(parameters, self.ENTRY_LOCATION, context)
        refly = self.parameterAsBool(parameters, self.REFLY, context)

        cruise_speed = self.parameterAsDouble(parameters, self.CRUISE_SPEED, context)
        hover_speed = self.parameterAsDouble(parameters, self.HOVER_SPEED, context)
        firmware_idx = self.parameterAsEnum(parameters, self.FIRMWARE_TYPE, context)
        firmware_type = 12 if firmware_idx == 0 else 3

        vehicle_idx = self.parameterAsEnum(parameters, self.VEHICLE_TYPE, context)
        vehicle_type_map = {0: 2, 1: 1, 2: 19}
        vehicle_type = vehicle_type_map.get(vehicle_idx, 2)

        elevation_layer = self.parameterAsRasterLayer(parameters, self.ELEVATION_LAYER, context)
        terrain_mode = elevation_layer is not None and elevation_layer.isValid()
        tolerance = self.parameterAsDouble(parameters, self.TOLERANCE, context)
        distance_mode = DistanceMode.CALC_ABOVE_TERRAIN if terrain_mode else DistanceMode.RELATIVE

        value_set_is_distance = gsd <= 0.0
        calc = CameraCalc(
            spec=spec,
            value_set_is_distance=value_set_is_distance,
            distance_to_surface=altitude,
            image_density=gsd,
            frontal_overlap=frontal_overlap,
            side_overlap=side_overlap,
        )

        if not calc.recalculate():
            raise QgsProcessingException(self.tr("Failed to calculate camera/flight parameters."))

        crs_wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")
        source_crs = source.sourceCrs()
        need_transform = source_crs != crs_wgs84
        if need_transform:
            to_wgs84 = QgsCoordinateTransform(source_crs, crs_wgs84, context.transformContext())

        items = []
        home_pos = None
        seq = 1

        feature_count = source.featureCount()
        total_steps = feature_count if feature_count > 0 else 100
        step = 0

        for feature in source.getFeatures():
            if feedback.isCanceled():
                break

            geom = feature.geometry()
            if geom.isEmpty() or geom.isNull():
                continue

            geom_wgs84 = QgsGeometry(geom)
            if need_transform:
                geom_wgs84.transform(to_wgs84)

            is_polygon = QgsGeometry.type(geom_wgs84) == 2  # Polygon geometry type

            if is_polygon:
                polygons = extract_polygons(geom_wgs84)
                for polygon_pts in polygons:
                    if len(polygon_pts) < 3:
                        continue

                    if home_pos is None:
                        home_pos = [polygon_pts[0][0], polygon_pts[0][1], calc.distance_to_surface]

                    ref_lat, ref_lon = polygon_pts[0]
                    proj = AEQDProjection(ref_lat, ref_lon)
                    planar_pts = [proj.forward(lat, lon) for lat, lon in polygon_pts]

                    transects = generate_survey_transects(
                        polygon_points=planar_pts,
                        grid_angle=grid_angle,
                        grid_spacing=calc.adjusted_footprint_side,
                        entry_location=entry_loc,
                        turnaround_distance=turnaround,
                        refly_90_degrees=refly,
                    )

                    geo_transects = [[proj.inverse(x, y) for x, y in tr] for tr in transects]

                    if terrain_mode:
                        adjusted_transects = [
                            adjust_terrain_flight_path(
                                tr,
                                raster_layer=elevation_layer,
                                target_altitude=calc.distance_to_surface,
                                step_distance=tolerance,
                                tolerance=tolerance,
                                transform_context=context.transformContext(),
                            )
                            for tr in geo_transects
                        ]
                        geo_transects = [
                            [(p.lat, p.lon, p.altitude) for p in tr] for tr in adjusted_transects
                        ]

                    visual_transect_pts = []
                    total_length = 0.0

                    for tr in geo_transects:
                        for pt in tr:
                            visual_transect_pts.append([pt[0], pt[1]])
                        for k in range(len(tr) - 1):
                            total_length += geodesic_distance(
                                tr[k][0], tr[k][1], tr[k + 1][0], tr[k + 1][1]
                            )

                    trig_dist = calc.trigger_distance
                    camera_shots = calculate_camera_shots(
                        total_distance=total_length,
                        trigger_distance=trig_dist,
                        camera_trigger_in_turnaround=True,
                    )

                    simple_items = expand_transects_to_mission_items(
                        geo_transects,
                        trigger_distance=trig_dist,
                        camera_trigger_in_turnaround=True,
                        has_turnaround=turnaround > 0.0,
                        distance_mode=distance_mode,
                        default_altitude=calc.distance_to_surface,
                        start_sequence=seq,
                    )
                    seq += len(simple_items)

                    survey_item = build_survey_item(
                        polygon_points=polygon_pts,
                        visual_transect_points=visual_transect_pts,
                        items=simple_items,
                        camera_calc=calc,
                        grid_angle=grid_angle,
                        entry_location=entry_loc,
                        turnaround_distance=turnaround,
                        camera_trigger_in_turnaround=True,
                        refly_90_degrees=refly,
                        distance_mode=distance_mode,
                        distance_to_surface=calc.distance_to_surface,
                        camera_name=spec.canonicalName,
                        camera_shots=camera_shots,
                        terrain_adjust_tolerance=tolerance,
                        terrain_flight_speed=cruise_speed,
                    )
                    items.append(survey_item)
            else:
                # LineString geometry fallback
                if geom_wgs84.isMultipart():
                    lines = geom_wgs84.asMultiPolyline()
                else:
                    lines = [geom_wgs84.asPolyline()]

                for line in lines:
                    if not line:
                        continue
                    if home_pos is None:
                        home_pos = [line[0].y(), line[0].x(), calc.distance_to_surface]
                    for pt in line:
                        items.append(
                            create_waypoint_item(
                                seq=seq,
                                lat=pt.y(),
                                lon=pt.x(),
                                alt=calc.distance_to_surface,
                            )
                        )
                        seq += 1

            step += 1
            feedback.setProgress(int(100.0 * step / total_steps))

        if home_pos is None:
            home_pos = [0.0, 0.0, 0.0]

        save_plan_file(
            filepath=output_file,
            planned_home_position=home_pos,
            items=items,
            cruise_speed=cruise_speed,
            hover_speed=hover_speed,
            firmware_type=firmware_type,
            vehicle_type=vehicle_type,
        )

        return {self.OUTPUT: output_file}
