"""QGC4QGIS Plugin main class implementation."""

import configparser
import os
from typing import TYPE_CHECKING

from qgis.core import QgsApplication
from qgis.PyQt.QtCore import QCoreApplication, QLocale, Qt, QTranslator
from qgis.PyQt.QtGui import QIcon
from qgis.PyQt.QtWidgets import QAction

from qgc4qgis.processing.provider import Qgc4QgisProvider

if TYPE_CHECKING:
    from qgc4qgis.gui.dock import QgcPlanningDockWidget

try:
    RIGHT_DOCK_WIDGET_AREA = Qt.DockWidgetArea.RightDockWidgetArea  # Qt6 (PyQt6)
except AttributeError:
    RIGHT_DOCK_WIDGET_AREA = Qt.RightDockWidgetArea  # Qt5 (PyQt5)


_TRANSLATORS: list[QTranslator] = []


def _load_translations() -> None:
    """Install the Qt translator for the current locale when one is shipped."""
    locale = QLocale.system().name()
    i18n_dir = os.path.join(os.path.dirname(__file__), "i18n")
    for candidate in (locale, locale[:2]):
        qm_path = os.path.join(i18n_dir, f"qgc4qgis_{candidate}.qm")
        if os.path.exists(qm_path):
            translator = QTranslator()
            if translator.load(qm_path):
                QCoreApplication.installTranslator(translator)
                _TRANSLATORS.append(translator)
            break


def _metadata_version() -> str:
    try:
        config = configparser.ConfigParser()
        config.optionxform = str
        config.read(os.path.join(os.path.dirname(__file__), "metadata.txt"), encoding="utf-8")
        return config["general"]["version"]
    except Exception:
        return "0.0.0"


class Qgc4QgisPlugin:
    """QGIS Plugin to integrate QGroundControl functionalities."""

    NAME = "QGC4QGIS"
    VERSION = _metadata_version()

    def __init__(self, iface):
        """Constructor.

        :param iface: An interface instance that will be passed to this class
            which gives the plugin access to the QGIS GUI.
        """
        _load_translations()
        self.iface = iface
        self.action: QAction | None = None
        self.provider: Qgc4QgisProvider | None = None
        self.dock_widget: QgcPlanningDockWidget | None = None

    def initProcessing(self) -> None:
        """Initialize and register the Processing provider."""
        if self.provider is None:
            self.provider = Qgc4QgisProvider()
            QgsApplication.processingRegistry().addProvider(self.provider)

    def initGui(self) -> None:
        """Create the menu entries and toolbar icons inside the QGIS GUI."""
        self.initProcessing()

        icon_path = os.path.join(os.path.dirname(__file__), "resources", "icon.svg")
        icon = QIcon(icon_path)

        self.action = QAction(icon, self.NAME, self.iface.mainWindow())
        self.action.setStatusTip(
            QCoreApplication.translate("Qgc4QgisPlugin", "{name} - Version {version}").format(
                name=self.NAME, version=self.VERSION
            )
        )
        self.action.triggered.connect(self.run)

        # Add the action to the Vector ▸ QGC4QGIS menu and the vector toolbar
        self.iface.addPluginToVectorMenu(self.NAME, self.action)
        self.iface.addVectorToolBarIcon(self.action)

    def unload(self) -> None:
        """Remove the plugin menu item, icon, dock widget, and processing provider from QGIS GUI."""
        if self.dock_widget is not None:
            self.dock_widget.unload()
            self.iface.removeDockWidget(self.dock_widget)
            self.dock_widget.deleteLater()
            self.dock_widget = None

        if self.provider:
            QgsApplication.processingRegistry().removeProvider(self.provider)
            self.provider = None

        if self.action:
            self.iface.removeVectorToolBarIcon(self.action)
            self.iface.removePluginVectorMenu(self.NAME, self.action)
            del self.action

    def run(self) -> None:
        """Run method that shows or toggles the QGC Flight Planning dock panel."""
        if self.dock_widget is None:
            from qgc4qgis.gui.dock import QgcPlanningDockWidget

            self.dock_widget = QgcPlanningDockWidget(self.iface.mainWindow())
            self.iface.addDockWidget(RIGHT_DOCK_WIDGET_AREA, self.dock_widget)

        self.dock_widget.show()
        self.dock_widget.raise_()
