"""Processing algorithm to download Copernicus DEM (GLO-30) for a given area."""

import math

from qgis.core import (
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsProcessing,
    QgsProcessingAlgorithm,
    QgsProcessingException,
    QgsProcessingParameterFeatureSource,
    QgsProcessingParameterNumber,
    QgsProcessingParameterRasterDestination,
)
from qgis.PyQt.QtCore import QCoreApplication

from qgc4qgis.core.elevation import ATTRIBUTION, download_dem


class DownloadDemAlgorithm(QgsProcessingAlgorithm):
    """QGIS Processing Algorithm to download Copernicus DEM (GLO-30) raster data."""

    def tr(self, string: str) -> str:
        """Return the translated string using the class context."""
        return QCoreApplication.translate("DownloadDemAlgorithm", string)

    INPUT = "INPUT"
    MARGEM = "MARGEM"
    OUTPUT = "OUTPUT"

    def name(self) -> str:
        """Return unique algorithm name."""
        return "baixar_dem_copernicus"

    def displayName(self) -> str:
        """Return localized human-readable algorithm name."""
        return self.tr("Download Copernicus DEM for area")

    def group(self) -> str:
        """Return localized group name."""
        return self.tr("Flight Planning")

    def groupId(self) -> str:
        """Return unique group identifier."""
        return "planejamento_voo"

    def createInstance(self) -> "DownloadDemAlgorithm":
        """Create new instance of algorithm."""
        return DownloadDemAlgorithm()

    def shortHelpString(self) -> str:
        """Return short help text for algorithm GUI."""
        return self.tr(
            "Downloads the Copernicus (GLO-30) digital elevation model (DEM) for the "
            "extent of the input layer, with a configurable safety margin.\n\n"
            "Source: {attribution}"
        ).format(attribution=ATTRIBUTION)

    def initAlgorithm(self, config=None) -> None:
        """Define algorithm parameters and outputs."""
        self.addParameter(
            QgsProcessingParameterFeatureSource(
                self.INPUT,
                self.tr("Polygon layer"),
                [QgsProcessing.TypeVectorPolygon],
            )
        )

        self.addParameter(
            QgsProcessingParameterNumber(
                self.MARGEM,
                self.tr("Safety margin (m)"),
                QgsProcessingParameterNumber.Double,
                defaultValue=250.0,
                minValue=0.0,
            )
        )

        self.addParameter(
            QgsProcessingParameterRasterDestination(
                self.OUTPUT,
                self.tr("Output DEM"),
            )
        )

    def processAlgorithm(self, parameters, context, feedback):
        """Execute DEM download algorithm logic."""
        source = self.parameterAsSource(parameters, self.INPUT, context)
        if source is None:
            raise QgsProcessingException(self.invalidSourceError(parameters, self.INPUT))

        margin_m = self.parameterAsDouble(parameters, self.MARGEM, context)
        out_path = self.parameterAsOutputLayer(parameters, self.OUTPUT, context)

        extent = source.sourceExtent()
        source_crs = source.sourceCrs()
        crs_wgs84 = QgsCoordinateReferenceSystem("EPSG:4326")

        if source_crs != crs_wgs84:
            transform = QgsCoordinateTransform(source_crs, crs_wgs84, context.transformContext())
            extent = transform.transformBoundingBox(extent)

        sw_lat = extent.yMinimum()
        ne_lat = extent.yMaximum()
        sw_lon = extent.xMinimum()
        ne_lon = extent.xMaximum()

        lat_mean = (sw_lat + ne_lat) / 2.0
        lat_margin_deg = margin_m / 111320.0
        cos_lat = math.cos(math.radians(lat_mean))
        if abs(cos_lat) < 1e-6:
            lon_margin_deg = lat_margin_deg
        else:
            lon_margin_deg = margin_m / (111320.0 * cos_lat)

        sw_lat -= lat_margin_deg
        ne_lat += lat_margin_deg
        sw_lon -= lon_margin_deg
        ne_lon += lon_margin_deg

        try:
            download_dem(
                sw_lat=sw_lat,
                sw_lon=sw_lon,
                ne_lat=ne_lat,
                ne_lon=ne_lon,
                out_path=out_path,
                feedback=feedback,
            )
        except (ValueError, RuntimeError) as exc:
            raise QgsProcessingException(
                self.tr("Error downloading Copernicus DEM: {error}").format(error=exc)
            ) from exc

        return {self.OUTPUT: out_path}
