import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0

Rectangle {
    id:     settingsView
    color:  qgcPal.window
    visible: false

    Item {
        id:    settingsItem
        anchors.fill: parent
        width:  Math.max(_root.width, settingsColumn.width)
        height: settingsColumn.height

        ColumnLayout {
            id:                         settingsColumn
            anchors.horizontalCenter:   parent.horizontalCenter

            QGCLabel {
                id:         unitsSectionLabel
                text:       qsTr("Units")
                visible:    QGroundControl.settingsManager.unitsSettings.visible
            }
            Rectangle {
                Layout.preferredHeight: unitsGrid.height + (_margins * 2)
                Layout.preferredWidth:  unitsGrid.width + (_margins * 2)
                color:                  qgcPal.windowShade
                visible:                miscSectionLabel.visible
                Layout.fillWidth:       true

                GridLayout {
                    id:                         unitsGrid
                    anchors.topMargin:          _margins
                    anchors.top:                parent.top
                    Layout.fillWidth:           false
                    anchors.horizontalCenter:   parent.horizontalCenter
                    flow:                       GridLayout.TopToBottom
                    rows:                       4

                    Repeater {
                        model: [ qsTr("Distance"), qsTr("Area"), qsTr("Speed"), qsTr("Temperature") ]
                        QGCLabel { text: modelData }
                    }
                    Repeater {
                        model:  [ QGroundControl.settingsManager.unitsSettings.distanceUnits, QGroundControl.settingsManager.unitsSettings.areaUnits, QGroundControl.settingsManager.unitsSettings.speedUnits, QGroundControl.settingsManager.unitsSettings.temperatureUnits ]
                        FactComboBox {
                            Layout.preferredWidth:  _comboFieldWidth
                            fact:                   modelData
                            indexModel:             false
                        }
                    }
                }
            }
        }
    }
}
