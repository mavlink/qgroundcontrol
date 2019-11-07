from complex_components import Slider
from qgroundcontrol import QGroundControlPO
from main_menu import MainMenuPO
import squish
import test


class DroneStatus:
    ARMED = ("Armed", "#27bf89")
    DISARMED = ("Disarmed", "#e1544c")


class ToolbarPO(QGroundControlPO):
    o_ToolBar = {
        "container": QGroundControlPO.application_window,
        "type": "ToolBar",
        "unnamed": 1,
        "visible": True,
    }
    o_icon_Image = {
        "container": o_ToolBar,
        "id": "_icon",
        "source": "qrc:/res/QGCLogoWhite",
        "type": "Image",
        "unnamed": 1,
        "visible": True,
    }
    vehicle_1_Text = {
        "container": o_ToolBar,
        "text": "Vehicle 1",
        "type": "Text",
        "unnamed": 1,
        "visible": True,
    }

    o_CustomArmedIndicator = {
        "container": o_ToolBar,
        "type": "CustomArmedIndicator",
        "unnamed": 1,
        "visible": True,
    }
    armedIndicatorRow = {
        "container": o_CustomArmedIndicator,
        "id": "labelRow",
        "type": "Row",
        "unnamed": 1,
        "visible": True,
    }
    armedIndicatorLabel = {
        "container": armedIndicatorRow,
        "type": "Text",
        "unnamed": 1,
        "visible": True,
    }
    armedIndicatorLed = {
        "container": armedIndicatorRow,
        "type": "Rectangle",
        "unnamed": 1,
        "visible": True,
    }


def open_main_menu():
    test.log("[Toolbar] Open Main Menu")
    squish.mouseClick(squish.waitForObject(ToolbarPO.o_icon_Image))
    squish.waitForObject(MainMenuPO.Fly_button)


def is_drone_armed():
    label = squish.waitForObject(ToolbarPO.armedIndicatorLabel)
    led = squish.waitForObject(ToolbarPO.armedIndicatorLed)
    if label.text == "Armed" and str(led.color.name) == "#27bf89":
        return True
    return False


def get_drone_status():
    label = squish.waitForObject(ToolbarPO.armedIndicatorLabel)
    led = squish.waitForObject(ToolbarPO.armedIndicatorLed)
    return (label.text, str(led.color.name))


def is_drone_disarmed():
    label = squish.waitForObject(ToolbarPO.armedIndicatorLabel)
    led = squish.waitForObject(ToolbarPO.armedIndicatorLed)
    if label.text == "Disarmed" and str(led.color.name) == "#e1544c":
        return True
    return False


def arm_drone():
    test.startSection("[Toolbarr] Arm Drone")
    label = ToolbarPO.armedIndicatorLabel.copy()
    label["text"] = "Disarmed"
    squish.mouseClick(squish.waitForObject(label))
    Slider.confirm()
    test.endSection()


def disarm_drone():
    test.startSection("[Toolbar] Disrm Drone")
    label = ToolbarPO.armedIndicatorLabel.copy()
    label["text"] = "Armed"
    squish.mouseClick(squish.waitForObject(label))
    Slider.confirm()
    test.endSection()
