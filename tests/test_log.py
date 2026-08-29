"""Testes do helper de log (passo 113, decisão D60)."""

from unittest.mock import patch

from qgis.core import Qgis, QgsMessageLog

from qgc4qgis.log import TAG, log_error, log_warning


def test_log_warning_nao_levanta_e_usa_tag_do_plugin(qgis_app):
    """log_warning delega ao QgsMessageLog com a tag QGC4QGIS sem levantar."""
    with patch.object(QgsMessageLog, "logMessage") as mock_log:
        log_warning("Mensagem de aviso")
        mock_log.assert_called_once_with("Mensagem de aviso", TAG, Qgis.Warning)


def test_log_error_nao_levanta_e_usa_tag_do_plugin(qgis_app):
    """log_error delega ao QgsMessageLog com nível Critical sem levantar."""
    with patch.object(QgsMessageLog, "logMessage") as mock_log:
        log_error("Mensagem de erro")
        mock_log.assert_called_once_with("Mensagem de erro", TAG, Qgis.Critical)


def test_log_aceita_nao_string(qgis_app):
    """Mensagens não-string são convertidas, como exceções capturadas."""
    with patch.object(QgsMessageLog, "logMessage") as mock_log:
        log_warning(12345)
        mock_log.assert_called_once_with("12345", TAG, Qgis.Warning)

    with patch.object(QgsMessageLog, "logMessage") as mock_log:
        log_error(Exception("Algo falhou"))
        mock_log.assert_called_once_with("Algo falhou", TAG, Qgis.Critical)
