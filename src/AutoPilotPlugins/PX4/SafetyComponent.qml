import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {
    QGCPalette { id: palette; colorGroupEnabled: true }

    width: 600
    height: 600
    color: palette.window

    property int flightLineWidth: 2             // width of lines for flight graphic
    property int loiterAltitudeColumnWidth: 180 // width of loiter altitude column
    property int shadedMargin: 20               // margin inset for shaded areas
    property int controlVerticalSpacing: 10     // vertical spacing between controls
    property int homeWidth: 50                  // width of home graphic
    property int planeWidth: 40                 // width of plane graphic
    property int arrowToHomeSpacing: 20         // space between down arrow and home graphic
    property int arrowWidth: 18                 // width for arrow graphic
    property int firstColumnWidth: 220          // Width of first column in return home triggers area

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "SAFETY CONFIG"
            font.pointSize: 20
        }

        Item { height: 20; width: 10 } // spacer

        //-----------------------------------------------------------------
        //-- Return Home Triggers

        QGCLabel { text: "Triggers For Return Home"; color: palette.text; font.pointSize: 20 }

        Item { height: 10; width: 10 } // spacer

        Rectangle {
            width: parent.width
            height: triggerColumn.height
            color: palette.windowShade

            Column {
                id: triggerColumn
                spacing: controlVerticalSpacing
                anchors.margins: shadedMargin
                anchors.left: parent.left

                // Top margin
                Item { height: 1; width: 10 }

                Row {
                    spacing: 10
                    QGCLabel { text: "RC Transmitter Signal Loss"; width: firstColumnWidth; anchors.baseline: rcLossField.baseline }
                    QGCLabel { text: "Return Home after"; anchors.baseline: rcLossField.baseline }
                    FactTextField {
                        id: rcLossField
                        fact: Fact { name: "COM_RC_LOSS_T" }
                        showUnits: true
                    }
                }

                Row {
                    spacing: 10
                    FactCheckBox {
                        id: telemetryTimeoutCheckbox
                        fact: Fact { name: "COM_DL_LOSS_EN" }
                        checkedValue: 1
                        uncheckedValue: 0
                        text: "Telemetry Signal Timeout"
                        anchors.baseline: telemetryLossField.baseline
                        width: firstColumnWidth
                    }
                    QGCLabel { text: "Return Home after"; anchors.baseline: telemetryLossField.baseline }
                    FactTextField {
                        id: telemetryLossField
                        fact: Fact { name: "COM_DL_LOSS_T" }
                        showUnits: true
                        enabled: telemetryTimeoutCheckbox.checked
                    }
                }

                // Bottom margin
                Item { height: 1; width: 10 }
            }
        }

        Item { height: 20; width: 10 }    // spacer

        //-----------------------------------------------------------------
        //-- Return Home Settings

        QGCLabel { text: "Return Home Settings"; font.pointSize: 20 }

        Item { height: 10; width: 10 } // spacer

        Rectangle {
            width: parent.width
            height: settingsColumn.height
            color: palette.windowShade

            Column {
                id: settingsColumn
                width: parent.width
                anchors.margins: shadedMargin
                anchors.left: parent.left

                Item { height: shadedMargin; width: 10 } // top margin

                // This item is the holder for the climb alt and loiter seconds fields
                Item {
                    width: parent.width
                    height: climbAltitudeColumn.height

                    Column {
                        id: climbAltitudeColumn
                        spacing: controlVerticalSpacing

                        QGCLabel { text: "Climb to altitude of" }
                        FactTextField {
                            id: climbField
                            fact: Fact { name: "RTL_RETURN_ALT" }
                            showUnits: true
                        }
                    }


                    Column {
                        x: flightGraphic.width - 200
                        spacing: controlVerticalSpacing

                        QGCCheckBox {
                            id: homeLoiterCheckbox
                            property Fact fact: Fact { name: "RTL_LAND_DELAY" }

                            checked: fact.value > 0
                            text: "Loiter at Home altitude for"
                            onClicked: {
                                fact.value = checked ? 60 : -1
                            }
                        }
                        FactTextField {
                            fact: Fact { name: "RTL_LAND_DELAY" }
                            showUnits: true
                            enabled: homeLoiterCheckbox.checked == true
                        }
                    }
                }

                Item { height: 20; width: 10 }    // spacer

                // This row holds the flight graphic and the home loiter alt column
                Row {
                    width: parent.width
                    spacing: 20

                    // Flight graphic
                    Item {
                        id: flightGraphic
                        width: parent.width - loiterAltitudeColumnWidth
                        height: 200 // controls the height of the flight graphic

                        Rectangle {
                            x: planeWidth / 2
                            height: planeImage.y - 5
                            width: flightLineWidth
                            color: palette.button
                        }
                        Rectangle {
                            x: planeWidth / 2
                            height: flightLineWidth
                            width: parent.width - x
                            color: palette.button
                        }
                        Rectangle {
                            x: parent.width - flightLineWidth
                            height: parent.height - homeWidth - arrowToHomeSpacing
                            width: flightLineWidth
                            color: palette.button
                        }

                        QGCColoredImage {
                            id: planeImage
                            y: parent.height - planeWidth - 40
                            source: "/qml/SafetyComponentPlane.png"
                            fillMode: Image.PreserveAspectFit
                            width: planeWidth
                            height: planeWidth
                            smooth: true
                            color: palette.button
                        }

                        QGCColoredImage {
                            x: planeWidth + 70
                            y: parent.height - height - 20
                            width: 80
                            height: parent.height / 2
                            source: "/qml/SafetyComponentTree.png"
                            fillMode: Image.Stretch
                            smooth: true
                            color: palette.windowShadeDark
                        }

                        QGCColoredImage {
                            x: planeWidth + 15
                            y: parent.height - height
                            width: 100
                            height: parent.height * .75
                            source: "/qml/SafetyComponentTree.png"
                            fillMode: Image.Stretch
                            smooth: true
                            color: palette.button
                        }

                        QGCColoredImage {
                            x: parent.width - (arrowWidth/2) - 1
                            y: parent.height - homeWidth - arrowToHomeSpacing - 2
                            source: "/qml/SafetyComponentArrowDown.png"
                            fillMode: Image.PreserveAspectFit
                            width: arrowWidth
                            height: arrowWidth
                            smooth: true
                            color: palette.button
                        }

                        QGCColoredImage {
                            id: homeImage
                            x: parent.width - (homeWidth / 2)
                            y: parent.height - homeWidth
                            source: "/qml/SafetyComponentHome.png"
                            fillMode: Image.PreserveAspectFit
                            width: homeWidth
                            height: homeWidth
                            smooth: true
                            color: palette.button
                        }
                    }

                    Column {
                        spacing: controlVerticalSpacing

                        QGCLabel {
                            text: "Home loiter altitude";
                            color: palette.text;
                            enabled: homeLoiterCheckbox.checked == true
                        }
                        FactTextField {
                            id: descendField;
                            fact: Fact { name: "RTL_DESCEND_ALT" }
                            enabled: homeLoiterCheckbox.checked == true
                            showUnits: true
                        }
                    }
                }

                Item { height: shadedMargin; width: 10 } // bottom margin
            }
        }

        QGCLabel {
            property Fact fact: Fact { name: "NAV_RCL_OBC" }
            width: parent.width
            font.pointSize: 14
            text: "Warning: You have an advanced safety configuration set using the NAV_RCL_OBC parameter. The above settings may not apply.";
            visible: fact.value != 0
            wrapMode: Text.Wrap
        }
        QGCLabel {
            property Fact fact: Fact { name: "NAV_DLL_OBC" }
            width: parent.width
            font.pointSize: 14
            text: "Warning: You have an advanced safety configuration set using the NAV_DLL_OBC parameter. The above settings may not apply.";
            visible: fact.value != 0
            wrapMode: Text.Wrap
        }
    }
}
