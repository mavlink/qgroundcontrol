"""Check passive fly-view waypoint overlays without changing shared PlanView components."""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def overlay(custom_name, upstream_path):
    custom = ROOT / "custom/src" / custom_name
    return (custom if custom.exists() else ROOT / upstream_path).read_text()


class PassiveWaypointsTest(unittest.TestCase):
    def test_qt_location_waypoints_are_passive(self):
        source = overlay("PlanMapItems.qml", "src/FlightMap/MapItems/PlanMapItems.qml")
        self.assertNotIn("confirmAction", source)
        self.assertRegex(source, r"delegate:\s*MissionItemMapVisual\s*\{[^}]*interactive:\s*false")
        self.assertIn("MissionItemIndicatorGroup", source)
        self.assertIn("MissionLineView", source)
        self.assertIn("MapLineArrow", source)
        self.assertIn("simpleFlightPathSegments", source)
        self.assertIn("removeMapItemGroup", source)

    def test_geo_map_waypoints_keep_visuals_without_click_wiring(self):
        source = overlay("GeoMapMissionItems.qml", "src/GeoMap/GeoMapMissionItems.qml")
        self.assertNotIn("visual.clicked.connect", source)
        self.assertNotRegex(source, r"root\.(itemClicked|groupClicked)\(")
        self.assertRegex(source, r"function _markerClicked\(marker\)\s*\{\s*\}")
        self.assertIn("setSource(object.geoMapVisualQML", source)
        self.assertIn("homeTerrainBias", source)
        self.assertIn("function _regroup()", source)

    def test_overlays_are_registered(self):
        cmake = (ROOT / "custom/CMakeLists.txt").read_text()
        for file, module in (("PlanMapItems", "FlightMap/MapItems"), ("GeoMapMissionItems", "GeoMap")):
            with self.subTest(file=file):
                self.assertIn(f'"${{CMAKE_CURRENT_SOURCE_DIR}}/src/{file}.qml"', cmake)
                self.assertIn(f'QT_RESOURCE_ALIAS "QGroundControl/{module}/{file}.qml"', cmake)


if __name__ == "__main__":
    unittest.main()
