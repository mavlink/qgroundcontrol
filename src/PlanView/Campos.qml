import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0

GridLayout {
    // The following properties must be available up the hierarchy chain
    //property var    missionItem       ///< Mission Item for editor

    Layout.fillWidth:   true
    columnSpacing:      _margin
    rowSpacing:         _margin
    columns:            3
    Layout.columnSpan:  2

    QGCLabel {
        text: qsTr("Survey Type")
        Layout.fillWidth:   true
    }

    QGCRadioButton {
        id:                     survey2DRadio
        leftPadding:            0
        text:                   qsTr("2D")
        checked:                !!missionItem.cameraCalc.camposSurvey2D.value
        onClicked:              missionItem.cameraCalc.camposSurvey2D.value = 1
    }

    QGCRadioButton {
        id:                     survey3DRadio
        leftPadding:            0
        text:                   qsTr("3D")
        checked:                !missionItem.cameraCalc.camposSurvey2D.value
        onClicked:              missionItem.cameraCalc.camposSurvey2D.value = 0
    }

    QGCLabel { text: qsTr("Positions") }
    FactTextField {
        fact:                   missionItem.cameraCalc.camposPositions
        Layout.fillWidth:       true
        Layout.columnSpan:      2
    }

    QGCLabel { text: qsTr("Min Interval") }
    FactTextField {
        fact:                   missionItem.cameraCalc.camposMinInterval
        Layout.fillWidth:       true
        Layout.columnSpan:      2
    }

    QGCLabel { text: qsTr("Roll Angle") }
    FactTextField {
        fact:                   missionItem.cameraCalc.camposRollAngle
        Layout.fillWidth:       true
        Layout.columnSpan:      2
    }

    QGCLabel { text: qsTr("Pitch Angle") }
    FactTextField {
        fact:                   missionItem.cameraCalc.camposPitchAngle
        Layout.fillWidth:       true
        Layout.columnSpan:      2
    }
}
