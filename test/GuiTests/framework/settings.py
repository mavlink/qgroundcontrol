import squish
import objectMap
import test
import os
from complex_components import ComboBox
from qgroundcontrol import QGroundControlPO


class SettingsMenuPO(QGroundControlPO):
    Offline_Maps_QGCButton = {
        "container": QGroundControlPO.application_window,
        "text": "Offline Maps",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }


class SettingsMenu:
    @staticmethod
    def open_offline_maps():
        test.log("[Settings][Offline Maps] Open Offline Maps")
        squish.mouseClick(squish.waitForObject(SettingsMenuPO.Offline_Maps_QGCButton))


class General:
    @staticmethod
    def change_distance_units(new_unit):
        test.log(f'[Settings][General] Change Distance units to "{new_unit}"')
        ComboBox.change_combo_value("distanceUnits", new_unit)

    @staticmethod
    def get_distance_units():
        combo_real_name = objectMap.realName(ComboBox.get_combo("distanceUnits"))
        return str(squish.waitForObjectExists(combo_real_name).displayText)


class OfflineMapsPO(QGroundControlPO):
    Export_QGCButton = {
        "container": QGroundControlPO.application_window,
        "text": "Export",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    o_exporTiles_QGCFlickable = {
        "container": QGroundControlPO.application_window,
        "id": "_exporTiles",
        "type": "QGCFlickable",
        "unnamed": 1,
        "visible": True,
    }
    o_exporTiles_Default_Tile_Set_QGCCheckBox = {
        "container": o_exporTiles_QGCFlickable,
        "text": "Default Tile Set",
        "type": "QGCCheckBox",
        "unnamed": 1,
        "visible": True,
    }
    Close_QGCButton = {
        "container": QGroundControlPO.o_Overlay,
        "id": "exportCloseButton",
        "text": "Close",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }


class OfflineMaps:
    @staticmethod
    def export_default_tile_set(file_name="tmp"):
        test.startSection("[Settings][Offline Maps] Export Map Tiles")
        test.log("[Settings][Offline Maps] Open Export - Select Tile Sets to Export")
        squish.mouseClick(squish.waitForObject(OfflineMapsPO.Export_QGCButton))
        default_tile_set_checkbox = squish.waitForObject(
            OfflineMapsPO.o_exporTiles_Default_Tile_Set_QGCCheckBox
        )
        if not default_tile_set_checkbox.checked:
            test.log('[Settings][Offline Maps] Activate "Default Tile Set"')
            squish.mouseClick(default_tile_set_checkbox)
        test.log("[Settings][Offline Maps] Trigger Export")
        squish.mouseClick(squish.waitForObject(OfflineMapsPO.Export_QGCButton))
        test.log(f'[Settings][Offline Maps] Provide "{file_name}" file name')
        squish.waitForImage(
            squish.findFile("scripts", os.path.join("images", "TileSetsButton.png"))
        )
        squish.nativeType(file_name)
        squish.snooze(1)
        squish.nativeType("<Return>")
        test.log("[Settings][Offline Maps] Close Export progress bar")
        squish.mouseClick(squish.waitForObject(OfflineMapsPO.Close_QGCButton))
        test.endSection()
