# -*- coding: utf-8 -*-
from utils import start_qgc, remove_file, file_exists
import main_menu
from settings import offline_maps


def main():
    tiles_file = "/tmp/tmp_default_tile_set"
    remove_file(tiles_file)
    test.verify(not file_exists(tiles_file), f"File {tiles_file} should NOT exist")
    start_qgc()
    main_menu.open_main_menu()
    main_menu.open_settings()
    offline_maps.open_offline_maps()
    offline_maps.export_default_tile_set(tiles_file)
    test.verify(file_exists(tiles_file), f"File {tiles_file} should exist")
