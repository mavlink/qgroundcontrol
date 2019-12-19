from toolbar import ToolbarPO
import squish
import test


def start(clear_settings=True):
    test.startSection("[Test Setup] Starting the QGroundControl Ground Control Station")
    aut = "run_qgc.sh --clear-settings" if clear_settings else "run_qgc.sh"
    test.log(f"[Test Setup] Launch {aut}")
    squish.startApplication(aut)
    squish.waitForObject(ToolbarPO.o_icon_Image)
    test.log("[Test Setup] Application started")
    test.endSection()
