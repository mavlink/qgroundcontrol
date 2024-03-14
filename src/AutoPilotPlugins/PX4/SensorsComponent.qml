/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl.Controls
import QGroundControl.PX4

SetupPage {
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        SensorsSetup {
            width:      availableWidth
            height:     availableHeight
        }
    }
}
