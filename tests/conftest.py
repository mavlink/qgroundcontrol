"""Pytest setup and fixtures for qgc4qgis plugin tests."""

import os
import sys

import pytest

# Ensure headless Qt environment
os.environ["QT_QPA_PLATFORM"] = "offscreen"

# Ensure QGIS plugins path is in sys.path for processing module
QGIS_PLUGIN_PATHS = ["/usr/share/qgis/python", "/usr/share/qgis/python/plugins"]
for path in QGIS_PLUGIN_PATHS:
    if os.path.exists(path) and path not in sys.path:
        sys.path.insert(0, path)

# Ensure repository root directory is in sys.path
PLUGIN_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if PLUGIN_DIR not in sys.path:
    sys.path.insert(0, PLUGIN_DIR)


@pytest.fixture(scope="session", autouse=True)
def qgis_app():
    """Initialize QgsApplication in headless mode for test suite execution."""
    from qgis.core import QgsApplication

    app = QgsApplication([], False)
    app.initQgis()

    yield app

    app.exitQgis()
