# -*- coding: utf-8 -*-
from utils import remove_file, file_exists
import application
import toolbar
import main_menu
import settings


def main():
    tiles_file = "/tmp/tmp_default_tile_set"
    remove_file(tiles_file)
    test.verify(not file_exists(tiles_file), f"File {tiles_file} should NOT exist")
    application.start()
    toolbar.open_main_menu()
    main_menu.open_settings()
    settings.SettingsMenu.open_offline_maps()
    settings.OfflineMaps.export_default_tile_set(tiles_file)
    test.verify(file_exists(tiles_file), f"File {tiles_file} should exist")
