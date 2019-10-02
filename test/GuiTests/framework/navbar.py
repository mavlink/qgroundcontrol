import global_object_map as names
import squish
import test


def open_main_menu():
    test.log("[Nav Bar] Open Main Menu")
    squish.mouseClick(squish.waitForObject(names.o_icon_Image))
    squish.waitForObject(names.fly_Button)
