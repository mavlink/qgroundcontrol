from utils import confirm_with_slider
import global_object_map as names
import squish
import test


def open_main_menu():
    test.log("[Toolbar] Open Main Menu")
    squish.mouseClick(squish.waitForObject(names.o_icon_Image))
    squish.waitForObject(names.fly_Button)


def is_drone_armed():
    label = squish.waitForObject(names.armedIndicatorLabel)
    led = squish.waitForObject(names.armedIndicatorLed)
    if label.text == "Armed" and str(led.color.name) == "#27bf89":
        return True
    return False


def is_drone_disarmed():
    label = squish.waitForObject(names.armedIndicatorLabel)
    led = squish.waitForObject(names.armedIndicatorLed)
    if label.text == "Disarmed" and str(led.color.name) == "#e1544c":
        return True
    return False


def arm_drone():
    test.startSection("[Toolbarr] Arm Drone")
    label = names.armedIndicatorLabel.copy()
    label["text"] = "Disarmed"
    squish.mouseClick(squish.waitForObject(label))
    confirm_with_slider()
    test.endSection()


def disarm_drone():
    test.startSection("[Toolbar] Disrm Drone")
    label = names.armedIndicatorLabel.copy()
    label["text"] = "Armed"
    squish.mouseClick(squish.waitForObject(label))
    confirm_with_slider()
    test.endSection()
