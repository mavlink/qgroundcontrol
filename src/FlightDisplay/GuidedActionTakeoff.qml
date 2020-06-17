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
    text:               guidedController.takeoffTitle
    iconSource:         "/res/takeoff.svg"
    visible:            guidedController.showTakeoff || !guidedController.showLand
    enabled:            guidedController.showTakeoff
    actionID:           guidedController.actionTakeoff
}
