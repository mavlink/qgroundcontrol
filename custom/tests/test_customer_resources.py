"""Source/resource regression checks complementing the Qt Quick input tests."""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class CustomerResourcesTest(unittest.TestCase):
    def test_fly_view_has_no_guided_actions(self):
        source = (ROOT / "custom/src/FlyViewToolStripActionList.qml").read_text()
        actions = re.findall(r"\b(Guided\w+|FlyViewAdditionalActionsButton)\s*\{", source)
        self.assertEqual(actions, [])
        self.assertRegex(source, r"model:\s*\[\s*\]")

    def test_toolbar_overrides_are_registered(self):
        cmake = (ROOT / "custom/CMakeLists.txt").read_text()
        for name in ("MainStatusIndicator", "GPSIndicator", "FlightModeIndicator", "BatteryIndicator", "EscIndicator"):
            with self.subTest(component=name):
                self.assertIn(f'"${{CMAKE_CURRENT_SOURCE_DIR}}/src/{name}.qml"', cmake)
                self.assertIn(f'QT_RESOURCE_ALIAS "QGroundControl/Toolbar/{name}.qml"', cmake)

    def test_custom_layer_has_no_compass_or_instruments(self):
        source = (ROOT / "custom/src/FlyViewCustomLayer.qml").read_text()
        self.assertNotRegex(source, r"\b(Rectangle|Image|CustomAttitudeWidget|Repeater)\s*\{")
        self.assertRegex(source, r"customOccluders:\s*\[\s*\]")

    def test_minimal_fly_view_overrides_are_registered(self):
        cmake = (ROOT / "custom/CMakeLists.txt").read_text()
        for name in ("FlyViewToolStrip", "FlyViewBottomRightRowLayout", "FlyViewCustomLayer"):
            with self.subTest(component=name):
                self.assertIn(f'"${{CMAKE_CURRENT_SOURCE_DIR}}/src/{name}.qml"', cmake)
                self.assertIn(f'QT_RESOURCE_ALIAS "QGroundControl/FlyView/{name}.qml"', cmake)

    def test_map_click_override_is_registered(self):
        cmake = (ROOT / "custom/CMakeLists.txt").read_text()
        self.assertTrue(
            '"${CMAKE_CURRENT_SOURCE_DIR}/src/FlyViewMapClickDropPanel.qml"' in cmake,
            "Customer map-click component must be included in resources",
        )
        self.assertTrue(
            'QT_RESOURCE_ALIAS "QGroundControl/FlyView/FlyViewMapClickDropPanel.qml"' in cmake,
            "Both map engines must resolve the inert customer component",
        )


if __name__ == "__main__":
    unittest.main()
