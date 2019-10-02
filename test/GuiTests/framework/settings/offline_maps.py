import global_object_map as names
import squish
import test
import os


def open_offline_maps():
    test.log("[Settings][Offline Maps] Open Offline Maps")
    squish.mouseClick(squish.waitForObject(names.offline_Maps_QGCButton))


def export_default_tile_set(file_name="tmp"):
    test.startSection("[Settings][Offline Maps] Export Map Tiles")
    test.log("[Settings][Offline Maps] Open Export - Select Tile Sets to Export")
    squish.mouseClick(squish.waitForObject(names.export_QGCButton))
    default_tile_set_checkbox = squish.waitForObject(
        names.o_exporTiles_Default_Tile_Set_QGCCheckBox
    )
    if not default_tile_set_checkbox.checked:
        test.log('[Settings][Offline Maps] Activate "Default Tile Set"')
        squish.mouseClick(default_tile_set_checkbox)
    test.log("[Settings][Offline Maps] Trigger Export")
    squish.mouseClick(squish.waitForObject(names.export_QGCButton))
    test.log(f'[Settings][Offline Maps] Provide "{file_name}" file name')
    squish.waitForImage(
        squish.findFile("scripts", os.path.join("settings", "TileSetsButton.png"))
    )
    squish.nativeType(file_name)
    squish.snooze(1)
    squish.nativeType("<Return>")
    test.log("[Settings][Offline Maps] Close Export progress bar")
    squish.mouseClick(squish.waitForObject(names.close_QGCButton))
    test.endSection()
