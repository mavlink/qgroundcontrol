/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    width:                  mainCol.width  + (ScreenTools.defaultFontPixelWidth  * 2)
    height:                 mainCol.height + (ScreenTools.defaultFontPixelHeight * 2)

    Column {

        QGCLabel {
            id:         joystickPeripheralsPage
            text:       qsTr("Select wanted peripheral to be active")
        }

        Row {
            Rectangle {
                Rectangle {
                    QGCLabel {
                        id:                 tbHeader
                        text:               qsTr("Peripheral")
                    }
                }
                Rectangle {
                    QGCLabel {
                        id:                 item1
                        text:               qsTr("Item 1")
                    }
                    QGCLabel {
                        id:                 item2
                        text:               qsTr("Item 2")
                    }
                    QGCLabel {
                        id:                 item3
                        text:               qsTr("Item 3")
                    }
                }
            }
            Rectangle {
                Rectangle {
                    QGCLabel {
                        id:                 tbHeader
                        text:               qsTr("Active")
                    }
                }
                Rectangle {
                    CheckBox {
                        id:                 item1
                        checkState: childGroup.checkState
                    }
                    CheckBox {
                        id:                 item2
                        checkState: childGroup.checkState
                    }
                    CheckBox {
                        id:                 item3
                        checkState: childGroup.checkState
                    }
                }
            }
        }
    }
}
