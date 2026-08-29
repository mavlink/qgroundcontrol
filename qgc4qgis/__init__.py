"""QGC4QGIS Plugin package initialization."""

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from qgis.gui import QgisInterface

    from .plugin import Qgc4QgisPlugin


def classFactory(iface: "QgisInterface") -> "Qgc4QgisPlugin":
    """QGIS plugin class factory entrypoint.

    :param iface: QGIS interface instance.
    :return: Qgc4QgisPlugin instance.
    """
    from .plugin import Qgc4QgisPlugin

    return Qgc4QgisPlugin(iface)
