"""Translation helper for core modules.

The core keeps working without Qt (it is tested outside QGIS), so the helper
falls back to the untranslated string when the Qt bindings are unavailable.
"""

try:
    from qgis.PyQt.QtCore import QCoreApplication

    def tr(string: str) -> str:
        """Return the translated string, or the source string if untranslated."""
        return QCoreApplication.translate("qgc4qgis", string)

except ImportError:  # pragma: no cover - exercised only outside QGIS

    def tr(string: str) -> str:
        """Return the string unchanged when Qt is unavailable."""
        return string
