import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.ScreenTools           1.0

Rectangle {
    id: settingsView
    color: qgcPal.window
    visible: false

    QGCLabel {
        id: unitsSectionLabel
        x: ScreenTools.defaultFontPixelWidth * 3 + ScreenTools.defaultFontPixelWidth / 2
        y: ScreenTools.defaultFontPixelWidth
        text: qsTr("UNITS")
        color: qgcPal.button
        font.pixelSize: 14
        font.bold: true
        visible: QGroundControl.settingsManager.unitsSettings.visible
    }

    Item{
        id: settingsItem
        anchors.fill: parent
        width: Math.max(_root.width, settingsColumn.width)
        height: settingsColumn.height

        ColumnLayout {
            id: settingsColumn
            spacing: ScreenTools.defaultFontPixelWidth
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                Layout.topMargin: ScreenTools.defaultFontPixelWidth * 6
                Layout.preferredHeight: unitsGrid.height + (_margins * 2)
                Layout.preferredWidth:  unitsGrid.width + (_margins * 2)
                color: qgcPal.windowShade
                visible: miscSectionLabel.visible
                Layout.fillWidth: true

                GridLayout {
                    id: unitsGrid
                    rowSpacing: ScreenTools.defaultFontPixelWidth
                    anchors.topMargin: _margins
                    anchors.top: parent.top
                    Layout.fillWidth: false
                    anchors.horizontalCenter: parent.horizontalCenter
                    flow: GridLayout.TopToBottom
                    rows: 4

                    //                    ToolSeparator
                    //                    {
                    //                        orientation: Qt.Horizontal
                    //                    }

                    Repeater {
                        model:  [ QGroundControl.settingsManager.unitsSettings.distanceUnits,
                            QGroundControl.settingsManager.unitsSettings.areaUnits,
                            QGroundControl.settingsManager.unitsSettings.speedUnits,
                            QGroundControl.settingsManager.unitsSettings.temperatureUnits
                        ]
                        FactComboBox {
                            Layout.preferredWidth: settingsView.width - ScreenTools.defaultFontPixelWidth * 6
                            Layout.topMargin: ScreenTools.defaultFontPixelWidth * 2
                            Layout.bottomMargin: ScreenTools.defaultFontPixelWidth * 3
                            fact: modelData
                            indexModel: false

                            //ToolSeparator { orientation: Qt.Horizontal }

                            Component.onCompleted:
                            {
                                hideIndicator()
                                hideBorder()
                                setItemFont(qgcPal.colorBlue, 14)
                                showTitle("Distance")
                            }
                        }
                    }
                }
            }
        }
    }
}
