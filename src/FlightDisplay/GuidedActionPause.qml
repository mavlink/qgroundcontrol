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
    text:       _guidedController.pauseTitle
    iconSource: "/res/pause-mission.svg"
    visible:    _guidedController.showPause
    enabled:    _guidedController.showPause
    actionID:   _guidedController.actionPause
}
