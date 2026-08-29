"""Processing algorithm to export a Litchi mission (.csv) file."""

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
    QgsProcessingParameterPoint,
    QgsProcessingParameterRasterLayer,
)
from qgis.PyQt.QtCore import QCoreApplication

from qgc4qgis.core.cameracalc import CameraCalc
from qgc4qgis.core.cameras import CUSTOM_CAMERA_NAME, CameraSpec, load_cameras
from qgc4qgis.core.geo import AEQDProjection
from qgc4qgis.core.litchi import save_litchi_csv
from qgc4qgis.core.missionitems import DistanceMode
from qgc4qgis.core.route import rebase_route_to_takeoff, route_from_transects
from qgc4qgis.core.survey import generate_survey_transects
from qgc4qgis.core.terrain import adjust_terrain_flight_path, sample_terrain_point
from qgc4qgis.processing.alg_survey_grid import extract_polygons


class ExportLitchiAlgorithm(QgsProcessingAlgorithm):
    """QGIS Processing Algorithm to export Litchi mission (.csv) files."""

    def tr(self, string: str) -> str:
        """Return the translated string using the class context."""
        return QCoreApplication.translate("ExportLitchiAlgorithm", string)

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
    TRIGGER_MODE = "TRIGGER_MODE"
    SPEED = "SPEED"
    GIMBAL_PITCH = "GIMBAL_PITCH"
    WAYPOINT_WAIT = "WAYPOINT_WAIT"
    ELEVATION_LAYER = "ELEVATION_LAYER"
    TOLERANCE = "TOLERANCE"
    TAKEOFF_POINT = "PONTO_DECOLAGEM"
    OUTPUT = "OUTPUT"

    def name(self) -> str:
        """Return unique algorithm name."""
        return "exportar_litchi_csv"

    def displayName(self) -> str:
        """Return localized human-readable algorithm name."""
        return self.tr("Export Litchi mission (.csv)")

    def group(self) -> str:
        """Return localized group name."""
        return self.tr("Flight Planning")

    def groupId(self) -> str:
        """Return unique group identifier."""
        return "planejamento_voo"

    def createInstance(self) -> "ExportLitchiAlgorithm":
        """Create new instance of algorithm."""
        return ExportLitchiAlgorithm()

    def shortHelpString(self) -> str:
        """Return short help text for algorithm GUI."""
        return self.tr(
            "Exports a flight mission in Litchi format (.csv) "
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
            QgsProcessingParameterEnum(
                self.TRIGGER_MODE,
                self.tr("Trigger mode"),
                options=[self.tr("By distance"), self.tr("By time"), self.tr("By photo")],
                defaultValue=0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.SPEED,
                self.tr("Speed (m/s)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=5.0,
                minValue=0.1,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.GIMBAL_PITCH,
                self.tr("Gimbal angle (degrees)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=-90.0,
                minValue=-90.0,
                maxValue=20.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.WAYPOINT_WAIT,
                self.tr("Wait at waypoint (s)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=0.0,
                minValue=0.0,
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
            QgsProcessingParameterPoint(
                self.TAKEOFF_POINT,
                self.tr("Takeoff point (optional — default: first waypoint)"),
                optional=True,
                defaultValue=None,
            )
        )

        self.addParameter(
            QgsProcessingParameterFileDestination(
                self.OUTPUT,
                self.tr("Output file (.csv)"),
                fileFilter="Litchi CSV (*.csv)",
            )
        )

    def processAlgorithm(self, parameters, context, feedback):
        """Execute Litchi CSV export algorithm logic."""
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

        trigger_mode_idx = self.parameterAsEnum(parameters, self.TRIGGER_MODE, context)
        trigger_mode_map = {0: "POR_DISTANCIA", 1: "POR_TEMPO", 2: "POR_FOTO"}
        trigger_mode_str = trigger_mode_map.get(trigger_mode_idx, "POR_DISTANCIA")

        speed = self.parameterAsDouble(parameters, self.SPEED, context)
        gimbal_pitch = self.parameterAsDouble(parameters, self.GIMBAL_PITCH, context)
        waypoint_wait = self.parameterAsDouble(parameters, self.WAYPOINT_WAIT, context)

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

        all_transects = []

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

                    all_transects.extend(geo_transects)
            else:
                # LineString geometry fallback
                if geom_wgs84.isMultipart():
                    lines = geom_wgs84.asMultiPolyline()
                else:
                    lines = [geom_wgs84.asPolyline()]

                for line in lines:
                    if not line:
                        continue
                    line_pts = [(pt.y(), pt.x()) for pt in line]
                    if terrain_mode:
                        adjusted = adjust_terrain_flight_path(
                            line_pts,
                            raster_layer=elevation_layer,
                            target_altitude=calc.distance_to_surface,
                            step_distance=tolerance,
                            tolerance=tolerance,
                            transform_context=context.transformContext(),
                        )
                        line_pts = [(p.lat, p.lon, p.altitude) for p in adjusted]
                    all_transects.append(line_pts)

            step += 1
            feedback.setProgress(int(100.0 * step / total_steps))

        if trigger_mode_str == "POR_TEMPO":
            trig_dist = calc.trigger_distance / speed if speed > 0 else calc.trigger_distance
        else:
            trig_dist = calc.trigger_distance

        route = route_from_transects(
            all_transects,
            altitude=calc.distance_to_surface,
            trigger_distance=trig_dist,
            trigger_mode=trigger_mode_str,
            distance_mode=distance_mode,
            flight_speed=speed,
            gimbal_pitch=gimbal_pitch,
        )

        if waypoint_wait > 0.0:
            wait_ms = int(waypoint_wait * 1000)
            for wp in route.waypoints:
                wp.acoes.insert(0, {"actiontype": 0, "actionparam": wait_ms})

        if terrain_mode:
            takeoff_pt = self.parameterAsPoint(parameters, self.TAKEOFF_POINT, context, crs_wgs84)
            if takeoff_pt is not None and not takeoff_pt.isEmpty():
                lat, lon = takeoff_pt.y(), takeoff_pt.x()
            elif route.waypoints:
                lat, lon = route.waypoints[0].lat, route.waypoints[0].lon
            else:
                lat, lon = 0.0, 0.0

            elev = sample_terrain_point(
                elevation_layer, (lat, lon), transform_context=context.transformContext()
            )
            if elev is None:
                raise QgsProcessingException(
                    self.tr(
                        "Could not sample elevation at the takeoff point (outside the DEM "
                        "or NoData). Choose another point or expand the elevation raster."
                    )
                )

            route = rebase_route_to_takeoff(route, elev)

        warnings = save_litchi_csv(
            output_file,
            route,
            default_speed=speed,
            default_gimbal_pitch=gimbal_pitch,
        )

        for warning in warnings:
            feedback.pushWarning(warning)

        return {self.OUTPUT: output_file}
