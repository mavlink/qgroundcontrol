/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
            columns:                3
            columnSpacing:          _margin
            rowSpacing:             ScreenTools.defaultFontPixelWidth * 2
            width:                  availableWidth
            //-----------------------------------------------------------------
            ProgressBar {
                id:                 progressBar
                to:                 100
                value:              geoController.progress
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
                Layout.columnSpan:  2
            }
            BusyIndicator {
                running:            geoController.progress > 0 && geoController.progress < 100 && geoController.errorMessage === ""
                width:              progressBar.height
                height:             progressBar.height
                Layout.alignment:   Qt.AlignVCenter
            }
            //-----------------------------------------------------------------
            QGCLabel {
                text:               geoController.errorMessage
                font.bold:          true
                font.pointSize:     ScreenTools.largeFontPointSize
                color:              "red"
                Layout.columnSpan:  3
            }
            //-----------------------------------------------------------------
            //-- Horizontal spacer line
            Rectangle {
                height:             1
                color:              qgcPal.windowShadeDark
                Layout.fillWidth:   true
                Layout.columnSpan:  3
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
                Layout.columnSpan:  2
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
                        geoController.selectImageDir = openLogFile.folder
                        close()
                    }
                }
            }
            QGCLabel {
                text:               geoController.imageDirectory
                elide:              Text.ElideLeft
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
                Layout.columnSpan:  2
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
                text:               geoController.saveDirectory !== "" ? geoController.saveDirectory : "/TAGGED folder in your image folder"
                elide:              Text.ElideLeft
                Layout.fillWidth:   true
                Layout.alignment:   Qt.AlignVCenter
                Layout.columnSpan:  2
            }
            //-----------------------------------------------------------------
            //-- Horizontal spacer line
            Rectangle {
                height:             1
                color:              qgcPal.windowShadeDark
                Layout.fillWidth:   true
                Layout.columnSpan:  3
            }
            //-----------------------------------------------------------------
            //-- Execute
            QGCButton {
                text:               geoController.inProgress ? qsTr("Cancel Tagging") : qsTr("Start Tagging")
                width:              ScreenTools.defaultFontPixelWidth * 30
                Layout.alignment:   Qt.AlignHCenter
                Layout.columnSpan:  3
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
