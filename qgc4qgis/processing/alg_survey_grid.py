"""Processing algorithm to generate flight survey grid lines from polygon geometries."""

import math

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
from qgc4qgis.core.geo import AEQDProjection, geodesic_distance
from qgc4qgis.core.survey import generate_survey_transects


def extract_polygons(geom: QgsGeometry) -> list[list[tuple[float, float]]]:
    """Extract exterior ring vertices (lat, lon) for each polygon in a polygon or multipolygon geometry."""
    polygons: list[list[tuple[float, float]]] = []
    if geom.isMultipart():
        multi = geom.asMultiPolygon()
        for poly in multi:
            if poly and poly[0]:
                polygons.append([(float(p.y()), float(p.x())) for p in poly[0]])
    else:
        poly = geom.asPolygon()
        if poly and poly[0]:
            polygons.append([(float(p.y()), float(p.x())) for p in poly[0]])
    return polygons


class SurveyGridAlgorithm(QgsProcessingAlgorithm):
    """QGIS Processing Algorithm to generate flight grid lines for polygon layers."""

    def tr(self, string: str) -> str:
        """Return the translated string using the class context."""
        return QCoreApplication.translate("SurveyGridAlgorithm", string)

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
    OUTPUT = "OUTPUT"

    def name(self) -> str:
        """Return unique algorithm name."""
        return "gerar_grade_voo"

    def displayName(self) -> str:
        """Return localized human-readable algorithm name."""
        return self.tr("Generate flight grid")

    def group(self) -> str:
        """Return localized group name."""
        return self.tr("Flight Planning")

    def groupId(self) -> str:
        """Return unique group identifier."""
        return "planejamento_voo"

    def createInstance(self) -> "SurveyGridAlgorithm":
        """Create new instance of algorithm."""
        return SurveyGridAlgorithm()

    def shortHelpString(self) -> str:
        """Return short help text for algorithm GUI."""
        return self.tr(
            "Generates photogrammetric flight grid lines from a polygon layer, "
            "camera settings, altitude/GSD, overlaps, angle and turnaround distance."
        )

    def initAlgorithm(self, config=None) -> None:
        """Define algorithm parameters and outputs."""
        cameras = load_cameras()
        camera_names = [c.canonicalName for c in cameras]

        self.addParameter(
            QgsProcessingParameterFeatureSource(
                self.INPUT,
                self.tr("Polygon layer"),
                [QgsProcessing.TypeVectorPolygon],
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
                self.OUTPUT,
                self.tr("Flight grid (Lines)"),
                QgsProcessing.TypeVectorLine,
            )
        )

    def processAlgorithm(self, parameters, context, feedback):
        """Execute flight survey grid algorithm logic."""
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

        fields = QgsFields()
        fields.append(QgsField("id", QVariant.Int))
        fields.append(QgsField("length_m", QVariant.Double))
        fields.append(QgsField("altitude_m", QVariant.Double))
        fields.append(QgsField("gsd_cm", QVariant.Double))
        fields.append(QgsField("side_overlap", QVariant.Double))
        fields.append(QgsField("frontal_overlap", QVariant.Double))
        fields.append(QgsField("trigger_dist_m", QVariant.Double))
        fields.append(QgsField("spacing_m", QVariant.Double))
        fields.append(QgsField("camera", QVariant.String))
        fields.append(QgsField("photo_count", QVariant.Int))

        (sink, dest_id) = self.parameterAsSink(
            parameters,
            self.OUTPUT,
            context,
            fields,
            Qgis.WkbType.LineString,
            source.sourceCrs(),
        )
        if sink is None:
            raise QgsProcessingException(self.invalidSinkError(parameters, self.OUTPUT))

        crs_wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")
        source_crs = source.sourceCrs()
        need_transform = source_crs != crs_wgs84
        if need_transform:
            to_wgs84 = QgsCoordinateTransform(source_crs, crs_wgs84, context.transformContext())
            to_source = QgsCoordinateTransform(crs_wgs84, source_crs, context.transformContext())

        feature_count = source.featureCount()
        total_steps = feature_count if feature_count > 0 else 100
        step = 0
        transect_id = 1

        for feature in source.getFeatures():
            if feedback.isCanceled():
                break

            geom = feature.geometry()
            if geom.isEmpty() or geom.isNull():
                continue

            geom_wgs84 = QgsGeometry(geom)
            if need_transform:
                geom_wgs84.transform(to_wgs84)

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

                for transect in transects:
                    if feedback.isCanceled():
                        break

                    geo_pts = [proj.inverse(x, y) for x, y in transect]
                    qgs_pts = [QgsPointXY(lon, lat) for lat, lon in geo_pts]
                    line_geom = QgsGeometry.fromPolylineXY(qgs_pts)

                    if need_transform:
                        line_geom.transform(to_source)

                    length_m = 0.0
                    for k in range(len(geo_pts) - 1):
                        length_m += geodesic_distance(
                            geo_pts[k][0], geo_pts[k][1], geo_pts[k + 1][0], geo_pts[k + 1][1]
                        )

                    trig_dist = calc.trigger_distance
                    photo_count = math.ceil(length_m / trig_dist) if trig_dist > 0 else 0

                    out_feat = QgsFeature()
                    out_feat.setGeometry(line_geom)
                    out_feat.setAttributes(
                        [
                            transect_id,
                            round(length_m, 2),
                            round(calc.distance_to_surface, 2),
                            round(calc.image_density, 2),
                            round(calc.side_overlap, 1),
                            round(calc.frontal_overlap, 1),
                            round(trig_dist, 2),
                            round(calc.adjusted_footprint_side, 2),
                            spec.canonicalName,
                            photo_count,
                        ]
                    )
                    sink.addFeature(out_feat, QgsFeatureSink.FastInsert)
                    transect_id += 1

            step += 1
            feedback.setProgress(int(100.0 * step / total_steps))

        return {self.OUTPUT: dest_id}
