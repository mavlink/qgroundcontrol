"""Tests for qgc4qgis plugin initialization and metadata verification."""

import configparser
import re
from pathlib import Path

import qgc4qgis


def test_import_qgc4qgis():
    """Verify qgc4qgis package can be imported without QGIS running."""
    assert qgc4qgis is not None


def test_plugin_metadata_file():
    """Verify metadata.txt exists, is valid INI, and contains required metadata fields."""
    plugin_dir = Path(qgc4qgis.__file__).parent
    metadata_path = plugin_dir / "metadata.txt"

    assert metadata_path.exists(), f"metadata.txt not found at {metadata_path}"

    config = configparser.ConfigParser()
    config.optionxform = str  # Preserve option case
    config.read(metadata_path, encoding="utf-8")

    assert "general" in config
    general = config["general"]

    required_keys = [
        "name",
        "qgisMinimumVersion",
        "qgisMaximumVersion",
        "description",
        "about",
        "version",
        "author",
        "email",
        "tracker",
        "homepage",
        "repository",
        "icon",
        "category",
        "supportsQt6",
        "hasProcessingProvider",
    ]

    for key in required_keys:
        assert key in general, f"Missing required metadata key: {key}"

    assert general["name"] == "QGC4QGIS"
    assert re.match(r"^\d+\.\d+\.\d+$", general["version"])
    assert general["qgisMinimumVersion"] == "3.34"
    assert general["qgisMaximumVersion"] == "4.99"
    assert general["hasProcessingProvider"] == "yes"

    icon_path = plugin_dir / general["icon"]
    assert icon_path.exists(), f"Icon file not found at {icon_path}"


def test_plugin_version_matches_metadata():
    """Verify Qgc4QgisPlugin.VERSION matches metadata.txt version and is not '0.0.0'."""
    from qgc4qgis.plugin import Qgc4QgisPlugin

    plugin_dir = Path(qgc4qgis.__file__).parent
    metadata_path = plugin_dir / "metadata.txt"

    config = configparser.ConfigParser()
    config.optionxform = str
    config.read(metadata_path, encoding="utf-8")

    expected_version = config["general"]["version"]

    assert expected_version == Qgc4QgisPlugin.VERSION
    assert Qgc4QgisPlugin.VERSION != "0.0.0"
