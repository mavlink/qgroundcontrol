/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl.FlightDisplay 1.0

GuidedToolStripAction {
    text:       _guidedController.rtlTitle
    iconSource: "/res/rtl.svg"
    visible:    true
    enabled:    _guidedController.showRTL
    actionID:   _guidedController.actionRTL
}
