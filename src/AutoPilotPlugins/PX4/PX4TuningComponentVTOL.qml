/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2

import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             tuningPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width: availableWidth
            height: availableHeight

            FactPanelController {
                id:         controller
            }

            QGCTabBar {
                id:             bar
                width:          parent.width
                anchors.top:    parent.top

                QGCTabButton {
                    text:       qsTr("Multirotor")
                }
                //QGCTabButton {
                //    text:       qsTr("Fixedwing")
                //}
            }

            property var pages:  [
                "PX4TuningComponentCopterAll.qml",
                //"PX4TuningComponentPlaneAll.qml"
            ]

            Loader {
                source:            pages[bar.currentIndex]
                width:             parent.width
                anchors.top:       bar.bottom
                anchors.topMargin: ScreenTools.defaultFontPixelWidth
                anchors.bottom:    parent.bottom
            }
        }
    } // Component - pageComponent
}
