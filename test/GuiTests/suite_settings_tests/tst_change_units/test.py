# -*- coding: utf-8 -*-
from utils import start_qgc
import navbar
import main_menu
from settings import general
import popups


def main():
    start_qgc()
    navbar.open_main_menu()
    main_menu.open_settings()
    general.change_distance_units("Feet")
    test.compare(
        general.get_distance_units(), "Feet", 'The Distance unit should be "Feet"'
    )
    test.compare(
        popups.get_popup_text(),
        '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">\n<html><head><meta name="qrichtext" content="1" /><style type="text/css">\np, li { white-space: pre-wrap; }\n</style></head><body style=" font-family:\'opensans-demibold\'; font-size:11pt; font-weight:400; font-style:normal;">\n<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">Change of \'Distance units\' value requires restart of CustomQGC to take effect.</p></body></html>',
        "Dialog should alert the user that restart is required",
    )
