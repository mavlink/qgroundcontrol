import squish
import test
import global_object_map as names


def open_settings():
    test.log("[Main Menu] Open Settings")
    squish.mouseClick(squish.waitForObject(names.settings_Button))
    squish.waitForObject(names.application_Settings_Text)
