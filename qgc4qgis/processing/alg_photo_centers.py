"""Processing algorithm to generate photo centers and oriented footprint polygons from flight transects or polygon layers."""

from qgis.core import (
    Qgis,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsFeature,
    QgsFeatureSink,
    QgsField,
    QgsFields,
    QgsGeometry,
    QgsPointXY,
    QgsProcessing,
    QgsProcessingAlgorithm,
    QgsProcessingException,
    QgsProcessingParameterBoolean,
    QgsProcessingParameterEnum,
    QgsProcessingParameterFeatureSink,
    QgsProcessingParameterFeatureSource,
    QgsProcessingParameterNumber,
)
from qgis.PyQt.QtCore import QCoreApplication, QVariant

from qgc4qgis.core.cameracalc import CameraCalc
from qgc4qgis.core.cameras import CUSTOM_CAMERA_NAME, CameraSpec, load_cameras
from qgc4qgis.core.geo import AEQDProjection
from qgc4qgis.core.route import sample_photo_centers
from qgc4qgis.core.survey import generate_survey_transects
from qgc4qgis.processing.alg_survey_grid import extract_polygons


def extract_lines(geom: QgsGeometry) -> list[list[tuple[float, float]]]:
    """Extract line vertices (lat, lon) for each line in a linestring or multilinestring geometry."""
    lines: list[list[tuple[float, float]]] = []
    if geom.isMultipart():
        multi = geom.asMultiPolyline()
        for line in multi:
            if line:
                lines.append([(float(p.y()), float(p.x())) for p in line])
    else:
        line = geom.asPolyline()
        if line:
            lines.append([(float(p.y()), float(p.x())) for p in line])
    return lines


class PhotoCentersAlgorithm(QgsProcessingAlgorithm):
    """QGIS Processing Algorithm to generate photo centers and footprint polygons."""

    def tr(self, string: str) -> str:
        """Return the translated string using the class context."""
        return QCoreApplication.translate("PhotoCentersAlgorithm", string)

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
    OUTPUT_CENTERS = "OUTPUT_CENTERS"
    OUTPUT_FOOTPRINTS = "OUTPUT_FOOTPRINTS"

    def name(self) -> str:
        """Return unique algorithm name."""
        return "gerar_centros_foto"

    def displayName(self) -> str:
        """Return localized human-readable algorithm name."""
        return self.tr("Generate photo centers and footprints")

    def group(self) -> str:
        """Return localized group name."""
        return self.tr("Flight Planning")

    def groupId(self) -> str:
        """Return unique group identifier."""
        return "planejamento_voo"

    def createInstance(self) -> "PhotoCentersAlgorithm":
        """Create new instance of algorithm."""
        return PhotoCentersAlgorithm()

    def shortHelpString(self) -> str:
        """Return short help text for algorithm GUI."""
        return self.tr(
            "Generates point layers of photo center positions and footprint polygons, "
            "oriented by the flight transects' azimuth."
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
            QgsProcessingParameterFeatureSink(
                self.OUTPUT_CENTERS,
                self.tr("Photo centers (Points)"),
                QgsProcessing.TypeVectorPoint,
            )
        )

        self.addParameter(
            QgsProcessingParameterFeatureSink(
                self.OUTPUT_FOOTPRINTS,
                self.tr("Photo footprints (Polygons)"),
                QgsProcessing.TypeVectorPolygon,
            )
        )

    def processAlgorithm(self, parameters, context, feedback):
        """Execute photo centers and footprint polygon generation algorithm logic."""
        source = self.parameterAsSource(parameters, self.INPUT, context)
        if source is None:
            raise QgsProcessingException(self.invalidSourceError(parameters, self.INPUT))

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

        center_fields = QgsFields()
        center_fields.append(QgsField("id", QVariant.Int))
        center_fields.append(QgsField("transect_id", QVariant.Int))
        center_fields.append(QgsField("photo_id", QVariant.Int))
        center_fields.append(QgsField("lat", QVariant.Double))
        center_fields.append(QgsField("lon", QVariant.Double))
        center_fields.append(QgsField("altitude_m", QVariant.Double))
        center_fields.append(QgsField("azimuth_deg", QVariant.Double))
        center_fields.append(QgsField("gsd_cm", QVariant.Double))
        center_fields.append(QgsField("camera", QVariant.String))

        footprint_fields = QgsFields()
        footprint_fields.append(QgsField("id", QVariant.Int))
        footprint_fields.append(QgsField("transect_id", QVariant.Int))
        footprint_fields.append(QgsField("photo_id", QVariant.Int))
        footprint_fields.append(QgsField("altitude_m", QVariant.Double))
        footprint_fields.append(QgsField("azimuth_deg", QVariant.Double))
        footprint_fields.append(QgsField("area_m2", QVariant.Double))
        footprint_fields.append(QgsField("gsd_cm", QVariant.Double))
        footprint_fields.append(QgsField("camera", QVariant.String))

        centers_param_key = (
            self.OUTPUT_CENTERS
            if self.OUTPUT_CENTERS in parameters
            else ("OUTPUT" if "OUTPUT" in parameters else self.OUTPUT_CENTERS)
        )

        (centers_sink, dest_centers_id) = self.parameterAsSink(
            parameters,
            centers_param_key,
            context,
            center_fields,
            Qgis.WkbType.Point,
            source.sourceCrs(),
        )
        if centers_sink is None:
            raise QgsProcessingException(self.invalidSinkError(parameters, centers_param_key))

        (footprints_sink, dest_footprints_id) = self.parameterAsSink(
            parameters,
            self.OUTPUT_FOOTPRINTS,
            context,
            footprint_fields,
            Qgis.WkbType.Polygon,
            source.sourceCrs(),
        )
        if footprints_sink is None:
            raise QgsProcessingException(self.invalidSinkError(parameters, self.OUTPUT_FOOTPRINTS))

        crs_wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")
        source_crs = source.sourceCrs()
        need_transform = source_crs != crs_wgs84
        if need_transform:
            to_wgs84 = QgsCoordinateTransform(source_crs, crs_wgs84, context.transformContext())
            to_source = QgsCoordinateTransform(crs_wgs84, source_crs, context.transformContext())

        feature_count = source.featureCount()
        total_steps = feature_count if feature_count > 0 else 100
        step = 0
        global_photo_id = 1
        transect_id = 1

        trig_dist = calc.trigger_distance
        fp_side = calc.image_footprint_side
        fp_frontal = calc.image_footprint_frontal
        fp_area = fp_side * fp_frontal

        for feature in source.getFeatures():
            if feedback.isCanceled():
                break

            geom = feature.geometry()
            if geom.isEmpty() or geom.isNull():
                continue

            geom_wgs84 = QgsGeometry(geom)
            if need_transform:
                geom_wgs84.transform(to_wgs84)

            geom_type = geom_wgs84.type()

            transects_list: list[list[tuple[float, float]]] = []

            if geom_type == Qgis.GeometryType.Polygon:
                polygons = extract_polygons(geom_wgs84)
                for polygon_pts in polygons:
                    if len(polygon_pts) < 3:
                        continue
                    ref_lat, ref_lon = polygon_pts[0]
                    proj = AEQDProjection(ref_lat, ref_lon)
                    planar_pts = [proj.forward(lat, lon) for lat, lon in polygon_pts]
                    generated_transects = generate_survey_transects(
                        polygon_points=planar_pts,
                        grid_angle=grid_angle,
                        grid_spacing=calc.adjusted_footprint_side,
                        entry_location=entry_loc,
                        turnaround_distance=turnaround,
                        refly_90_degrees=refly,
                    )
                    for tr in generated_transects:
                        geo_pts = [proj.inverse(x, y) for x, y in tr]
                        transects_list.append(geo_pts)
            elif geom_type == Qgis.GeometryType.Line:
                lines = extract_lines(geom_wgs84)
                transects_list.extend(lines)

            for geo_pts in transects_list:
                if feedback.isCanceled():
                    break

                centers = sample_photo_centers(geo_pts, trig_dist)
                for center in centers:
                    pt_geom = QgsGeometry.fromPointXY(QgsPointXY(center.lon, center.lat))
                    if need_transform:
                        pt_geom.transform(to_source)

                    c_feat = QgsFeature()
                    c_feat.setGeometry(pt_geom)
                    c_feat.setAttributes(
                        [
                            global_photo_id,
                            transect_id,
                            center.photo_in_transect_id,
                            round(center.lat, 7),
                            round(center.lon, 7),
                            round(calc.distance_to_surface, 2),
                            round(center.azimuth_deg, 1),
                            round(calc.image_density, 2),
                            spec.canonicalName,
                        ]
                    )
                    centers_sink.addFeature(c_feat, QgsFeatureSink.FastInsert)

                    poly_pts = [
                        QgsPointXY(c_lon, c_lat)
                        for c_lat, c_lon in center.footprint_corners(fp_frontal, fp_side)
                    ]
                    if poly_pts:
                        poly_pts.append(poly_pts[0])

                    fp_geom = QgsGeometry.fromPolygonXY([poly_pts])
                    if need_transform:
                        fp_geom.transform(to_source)

                    fp_feat = QgsFeature()
                    fp_feat.setGeometry(fp_geom)
                    fp_feat.setAttributes(
                        [
                            global_photo_id,
                            transect_id,
                            center.photo_in_transect_id,
                            round(calc.distance_to_surface, 2),
                            round(center.azimuth_deg, 1),
                            round(fp_area, 2),
                            round(calc.image_density, 2),
                            spec.canonicalName,
                        ]
                    )
                    footprints_sink.addFeature(fp_feat, QgsFeatureSink.FastInsert)

                    global_photo_id += 1

                transect_id += 1

            step += 1
            feedback.setProgress(int(100.0 * step / total_steps))

        return {
            self.OUTPUT_CENTERS: dest_centers_id,
            self.OUTPUT_FOOTPRINTS: dest_footprints_id,
        }
