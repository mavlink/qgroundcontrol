/****************************************************************************
 *
 * (c) 2009-2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Controls     1.2
import QtQuick.Dialogs      1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: _root

    property var  _autotune:   globals.activeVehicle.autotune
    property real _margins:    ScreenTools.defaultFontPixelHeight

    readonly property string dialogTitle: qsTr("Autotune")

    QGCPalette {
        id:                palette
        colorGroupEnabled: enabled
    }

    Component {
        id: autotuneConfirmationDialogComponent
        QGCViewMessage {
            message: qsTr("WARNING!\
\n\nThe auto-tuning procedure should be executed with caution and requires the vehicle to fly stable enough before \
attempting the procedure!\n\nBefore starting the auto-tuning process, make sure that: \
\n1. You have read the auto-tuning guide and have followed the preliminary steps \
\n2. The current control gains are good enough to stabilize the drone in presence of medium disturbances \
\n3. You are ready to abort the auto-tuning sequence by moving the RC sticks, if anything unexpected happens. \
\n\nClick Ok to start the auto-tuning process.\n")

            function accept() {
                hideDialog()
                _autotune.autotuneRequest()
            }
        }
    }

    Rectangle {
        width:   _root.width
        height:  statusColumn.height + (2 * _margins)
        color:   palette.windowShade
        enabled: _autotune.autotuneEnabled

        QGCButton {
            id:        autotuneButton
            primary:   true
            text:      dialogTitle
            enabled:   !_autotune.autotuneInProgress
            anchors {
                left:             parent.left
                leftMargin:       _margins
                verticalCenter:   parent.verticalCenter
            }

            onClicked: {
                mainWindow.showComponentDialog(autotuneConfirmationDialogComponent,
                                               dialogTitle,
                                               mainWindow.showDialogDefaultWidth,
                                               StandardButton.Ok | StandardButton.Cancel)
            }
        }

        Column {
            id:      statusColumn
            spacing: _margins
            anchors  {
                left:             autotuneButton.right
                right:            parent.right
                leftMargin:       _margins
                rightMargin:      _margins
                verticalCenter:   parent.verticalCenter
            }

            QGCLabel {
                text:   _autotune.autotuneStatus

                anchors {
                    left: parent.left
                }
            }

            ProgressBar {
                value:   _autotune.autotuneProgress

                anchors {
                    left:             parent.left
                    right:            parent.right
                }
            }
        }
    }
}
