from utils import confirm_with_slider
import global_object_map as names
import squish
import test
import object


def is_drone_armed():
    label = squish.waitForObject(names.armedIndicatorLabel)
    led = squish.waitForObject(names.armedIndicatorLed)
    green = "#27bf89"
    # red: "#e1544c"
    if squish.waitFor(lambda: label.text == "Armed", 10000):
        if squish.waitFor(lambda: str(led.color.name) == green, 10000):
            return True
    return False


def arm_drone():
    test.startSection("[Drone Bar] Arm Drone")
    label = names.armedIndicatorLabel.copy()
    label["text"] = "Disarmed"
    squish.mouseClick(squish.waitForObject(label))
    confirm_with_slider()
    test.endSection()


def disarm_drone():
    test.startSection("[Drone Bar] Disrm Drone")
    label = names.armedIndicatorLabel.copy()
    label["text"] = "Armed"
    squish.mouseClick(squish.waitForObject(label))
    confirm_with_slider()
    test.endSection()
