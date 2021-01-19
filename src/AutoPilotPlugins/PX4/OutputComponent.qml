/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.PX4           1.0

SetupPage {
    id:             powerPage
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        Item {
            width:  availableWidth
            height: bar.height + outputLoader.height

            QGCTabBar {
                id:             bar
                width:          parent.width
                Component.onCompleted: {
                    currentIndex = 0
                }
                anchors.top:    parent.top
                QGCTabButton {
                    text:       qsTr("Graphical")
                }
                QGCTabButton {
                    text:       qsTr("Table")
                }
                QGCTabButton {
                    text:       qsTr("Output Test")
                }
            }

            property var pages:  ["OutputGraphical.qml", "OutputTable.qml", "OutputTest.qml"]

            Loader {
                id:             outputLoader
                source:         pages[bar.currentIndex]
                width:          parent.width
                anchors.top:    bar.bottom
                anchors.topMargin: ScreenTools.defaultFontPixelHeight
            }
        } // Item
    } // Component
} // SetupPage
