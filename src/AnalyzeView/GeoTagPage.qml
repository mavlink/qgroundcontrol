/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs  1.2

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

AnalyzePage {
    id:                 geoTagPage
    pageComponent:      pageComponent
    pageName:           qsTr("GeoTag Images (WIP)")
    pageDescription:    qsTr("GetTag Images is used to tag a set of images from a survey mission with gps coordinates. You must provide the binary log from the flight as well as the directory which contains the images to tag.")

    property real _margin: ScreenTools.defaultFontPixelWidth

    GeoTagController {
        id: controller
    }

    Component {
        id: pageComponent

        Column {
            id:         mainColumn
            width:      availableWidth
            spacing:    _margin

            Row {
                spacing: _margin

                QGCLabel {
                    text: "Log file:"
                }

                QGCLabel {
                    text: controller.logFile
                }

                QGCButton {
                    text:       qsTr("Select log file")
                    onClicked:  controller.pickLogFile()
                }
            }

            Row {
                spacing: _margin

                QGCLabel {
                    text: "Image directory:"
                }

                QGCLabel {
                    text: controller.imageDirectory
                }

                QGCButton {
                    text:       qsTr("Select image directory")
                    onClicked:  controller.pickImageDirectory()
                }
            }

            QGCLabel { text: "NYI - Simulated only" }

            QGCButton {
                text: controller.inProgress ? qsTr("Cancel Tagging") : qsTr("Start Tagging")

                onClicked: {
                    if (controller.inProgress) {
                        controller.cancelTagging()
                    } else {
                        if (controller.logFile == "" || controller.imageDirectory == "") {
                            geoTagPage.showMessage(qsTr("Error"), qsTr("You must select a log file and image directory before you can start tagging."), StandardButton.Ok)
                            return
                        }
                        controller.startTagging()
                    }
                }
            }

            QGCLabel {
                text: controller.errorMessage
            }

            ProgressBar {
                anchors.left:   parent.left
                anchors.right:  parent.right
                maximumValue:   100
                value:          controller.progress
            }
        } // Column
    } // Component
} // AnalyzePage
