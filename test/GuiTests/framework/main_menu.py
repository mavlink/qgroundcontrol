import squish
import test

from qgroundcontrol import QGroundControlPO


class MainMenuPO(QGroundControlPO):
    Analyze_button = {
        "container": QGroundControlPO.o_Overlay,
        "text": "Analyze",
        "type": "Button",
        "unnamed": 1,
        "visible": True,
    }
    Fly_button = {
        "container": QGroundControlPO.o_Overlay,
        "text": "Fly",
        "type": "Button",
        "unnamed": 1,
        "visible": True,
    }
    Settings_button = {
        "container": QGroundControlPO.o_Overlay,
        "id": "settingsButton",
        "text": "Settings",
        "type": "Button",
        "unnamed": 1,
        "visible": True,
    }


def open_settings():
    test.log("[Main Menu] Open Settings")
    squish.mouseClick(squish.waitForObject(MainMenuPO.Settings_button))


def open_analyze():
    test.log("[Main Menu] Open Analyze")
    squish.mouseClick(squish.waitForObject(MainMenuPO.Analyze_button))


def open_fly():
    test.log("[Main Menu] Open Fly")
    squish.mouseClick(squish.waitForObject(MainMenuPO.Fly_button))
