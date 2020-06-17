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
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

AnalyzePage {
    id:                 geoTagPage
    pageComponent:      pageComponent
    pageName:           qsTr("GeoTag Images")
    pageDescription:    qsTr("GeoTag Images is used to tag a set of images from a survey mission with gps coordinates. You must provide the binary log from the flight as well as the directory which contains the images to tag.")

    readonly property real _margin:     ScreenTools.defaultFontPixelWidth * 2
    readonly property real _minWidth:   ScreenTools.defaultFontPixelWidth * 20
    readonly property real _maxWidth:   ScreenTools.defaultFontPixelWidth * 30

    Component {
        id:  pageComponent
        GridLayout {
            columns:                2
            columnSpacing:          _margin
            rowSpacing:             ScreenTools.defaultFontPixelWidth * 2
            width:                  availableWidth
            BusyIndicator {
                running:            geoController.progress > 0 && geoController.progress < 100 && geoController.errorMessage === ""
                width:              progressBar.height
                height:             progressBar.height
                Layout.alignment:   Qt.AlignVCenter | Qt.AlignHCenter
            }
            //-----------------------------------------------------------------
            ProgressBar {
                id:                 progressBar
                to:                 100
                value:              geoController.progress
                opacity:            geoController.progress > 0 ? 1 : 0.25
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
            }
            //-----------------------------------------------------------------
            QGCLabel {
                text:               geoController.errorMessage
                color:              "red"
                font.family:        ScreenTools.demiboldFontFamily
                font.pointSize:     ScreenTools.mediumFontPointSize
                horizontalAlignment:Text.AlignHCenter
                Layout.alignment:   Qt.AlignHCenter
                Layout.columnSpan:  2
            }
            //-----------------------------------------------------------------
            //-- Log File
            QGCButton {
                text:               qsTr("Select log file")
                onClicked:          openLogFile.open()
                Layout.minimumWidth:_minWidth
                Layout.maximumWidth:_maxWidth
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
                FileDialog {
                    id:             openLogFile
                    title:          qsTr("Select log file")
                    folder:         shortcuts.home
                    nameFilters:    [qsTr("ULog file (*.ulg)"), qsTr("PX4 log file (*.px4log)"), qsTr("All Files (*.*)")]
                    defaultSuffix:  "ulg"
                    selectExisting: true
                    onAccepted: {
                        geoController.logFile = openLogFile.fileUrl
                        close()
                    }
                }
            }
            QGCLabel {
                text:               geoController.logFile
                elide:              Text.ElideLeft
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
            }
            //-----------------------------------------------------------------
            //-- Image Directory
            QGCButton {
                text:               qsTr("Select image directory")
                onClicked:          selectImageDir.open()
                Layout.minimumWidth:_minWidth
                Layout.maximumWidth:_maxWidth
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
                FileDialog {
                    id:             selectImageDir
                    title:          qsTr("Select image directory")
                    folder:         shortcuts.home
                    selectFolder:   true
                    selectExisting: true
                    onAccepted: {
                        geoController.imageDirectory = selectImageDir.folder
                        close()
                    }
                }
            }
            QGCLabel {
                text:               geoController.imageDirectory
                elide:              Text.ElideLeft
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
            }
            //-----------------------------------------------------------------
            //-- Save Directory
            QGCButton {
                text:               qsTr("(Optionally) Select save directory")
                onClicked:          selectDestDir.open()
                Layout.minimumWidth:_minWidth
                Layout.maximumWidth:_maxWidth
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
                FileDialog {
                    id:             selectDestDir
                    title:          qsTr("Select save directory")
                    folder:         shortcuts.home
                    selectFolder:   true
                    selectExisting: true
                    onAccepted: {
                        geoController.saveDirectory = selectDestDir.folder
                        close()
                    }
                }
            }
            QGCLabel {
                text:               geoController.saveDirectory === "" ? (geoController.imageDirectory === "" ? "/TAGGED folder in your image folder" : geoController.imageDirectory + "/TAGGED") : geoController.saveDirectory
                elide:              Text.ElideLeft
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
            }
            //-----------------------------------------------------------------
            //-- Execute
            QGCButton {
                text:               geoController.inProgress ? qsTr("Cancel Tagging") : qsTr("Start Tagging")
                width:              ScreenTools.defaultFontPixelWidth * 30
                enabled:            (geoController.imageDirectory !== "" && geoController.logFile !== "") || geoController.inProgress
                Layout.alignment:   Qt.AlignHCenter
                Layout.columnSpan:  2
                onClicked: {
                    if (geoController.inProgress) {
                        geoController.cancelTagging()
                    } else {
                        geoController.startTagging()
                    }
                }
            }
        }
    }
}
