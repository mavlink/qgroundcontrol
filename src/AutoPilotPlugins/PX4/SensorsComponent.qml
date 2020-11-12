/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl.Controls  1.0
import QGroundControl.PX4       1.0

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
