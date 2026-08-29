"""Processing provider implementation for QGC4QGIS plugin."""

from qgis.core import QgsProcessingProvider
from qgis.PyQt.QtCore import QCoreApplication
from qgis.PyQt.QtGui import QIcon

from qgc4qgis.processing.alg_download_dem import DownloadDemAlgorithm
from qgc4qgis.processing.alg_export_dji import ExportDjiAlgorithm
from qgc4qgis.processing.alg_export_kml import ExportLitchiKmlAlgorithm
from qgc4qgis.processing.alg_export_litchi import ExportLitchiAlgorithm
from qgc4qgis.processing.alg_export_plan import ExportPlanAlgorithm
from qgc4qgis.processing.alg_photo_centers import PhotoCentersAlgorithm
from qgc4qgis.processing.alg_survey_grid import SurveyGridAlgorithm


class Qgc4QgisProvider(QgsProcessingProvider):
    """QGIS Processing provider for QGC4QGIS algorithms."""

    def tr(self, string: str) -> str:
        """Return the translated string using the class context."""
        return QCoreApplication.translate("Qgc4QgisProvider", string)

    def id(self) -> str:
        """Return unique provider identifier."""
        return "qgc4qgis"

    def name(self) -> str:
        """Return user-visible provider name."""
        return "QGC4QGIS"

    def icon(self) -> QIcon:
        """Return provider icon."""
        return QIcon()

    def loadAlgorithms(self) -> None:
        """Load all algorithms belonging to this provider."""
        self.addAlgorithm(SurveyGridAlgorithm())
        self.addAlgorithm(ExportPlanAlgorithm())
        self.addAlgorithm(ExportLitchiAlgorithm())
        self.addAlgorithm(ExportLitchiKmlAlgorithm())
        self.addAlgorithm(ExportDjiAlgorithm())
        self.addAlgorithm(PhotoCentersAlgorithm())
        self.addAlgorithm(DownloadDemAlgorithm())
