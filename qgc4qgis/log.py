"""Helpers de log do plugin sobre o QgsMessageLog (decisão D60 do plano 0.6.3)."""

from qgis.core import Qgis, QgsMessageLog

TAG = "QGC4QGIS"

# Qgis.MessageLevel.Warning (QGIS 3.24+/4.x) com fallback para Qgis.Warning
# (QGIS antigo) — mesmo padrão do dock.py para QFrame.Shape.NoFrame.
try:
    _WARNING = Qgis.MessageLevel.Warning
    _CRITICAL = Qgis.MessageLevel.Critical
except AttributeError:
    _WARNING = Qgis.Warning
    _CRITICAL = Qgis.Critical


def log_warning(msg: str) -> None:
    """Registrar aviso no log do QGIS com a tag do plugin.

    :param msg: Mensagem a registrar.
    """
    QgsMessageLog.logMessage(str(msg), TAG, _WARNING)


def log_error(msg: str) -> None:
    """Registrar erro no log do QGIS com a tag do plugin.

    :param msg: Mensagem a registrar.
    """
    QgsMessageLog.logMessage(str(msg), TAG, _CRITICAL)
