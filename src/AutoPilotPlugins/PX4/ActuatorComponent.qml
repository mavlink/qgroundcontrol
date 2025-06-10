import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import QGroundControl.ScreenTools
import QGroundControl.AutoPilotPlugins.PX4

SetupPage {
    id:             actuatorPage
    pageComponent:  pageComponent
    showAdvanced:   true

    property var actuators:       globals.activeVehicle.actuators
    property var _escStatus:      globals.activeVehicle ? globals.activeVehicle.escStatus : null

    property var _showAdvanced:              advanced
    readonly property real _margins:         ScreenTools.defaultFontPixelHeight

    Component {
        id: pageComponent

        Row {
            spacing:                        ScreenTools.defaultFontPixelWidth * 4
            property var _leftColumnWidth:  Math.max(actuatorTesting.implicitWidth, mixerUi.implicitWidth) + (_margins * 2)

            // ESC data properties
            property bool   _escDataAvailable:  _escStatus ? _escStatus.telemetryAvailable : false
            property int    _motorCount:        _escStatus ? _escStatus.count.rawValue : 0
            property int    _infoBitmask:       _escStatus ? _escStatus.info.rawValue : 0

            function _isMotorOnline(motorIndex) {
                return (_infoBitmask & (1 << motorIndex)) !== 0
            }

            function _isMotorHealthy(motorIndex) {
                if (!_escDataAvailable || !_isMotorOnline(motorIndex)) return false
                var failureFlags = _getMotorFailureFlags(motorIndex)
                return failureFlags === 0
            }

            function _getMotorFailureFlags(motorIndex) {
                if (!_escStatus) return 0

                switch (motorIndex) {
                case 0: return _escStatus.failureFlagsFirst.rawValue
                case 1: return _escStatus.failureFlagsSecond.rawValue
                case 2: return _escStatus.failureFlagsThird.rawValue
                case 3: return _escStatus.failureFlagsFourth.rawValue
                case 4: return _escStatus.failureFlagsFifth.rawValue
                case 5: return _escStatus.failureFlagsSixth.rawValue
                case 6: return _escStatus.failureFlagsSeventh.rawValue
                case 7: return _escStatus.failureFlagsEighth.rawValue
                default: return 0
                }
            }

            function _getMotorRPM(motorIndex) {
                if (!_escStatus) return 0

                switch (motorIndex) {
                case 0: return _escStatus.rpmFirst.rawValue
                case 1: return _escStatus.rpmSecond.rawValue
                case 2: return _escStatus.rpmThird.rawValue
                case 3: return _escStatus.rpmFourth.rawValue
                case 4: return _escStatus.rpmFifth.rawValue
                case 5: return _escStatus.rpmSixth.rawValue
                case 6: return _escStatus.rpmSeventh.rawValue
                case 7: return _escStatus.rpmEighth.rawValue
                default: return 0
                }
            }

            function _getMotorVoltage(motorIndex) {
                if (!_escStatus) return 0

                switch (motorIndex) {
                case 0: return _escStatus.voltageFirst.rawValue
                case 1: return _escStatus.voltageSecond.rawValue
                case 2: return _escStatus.voltageThird.rawValue
                case 3: return _escStatus.voltageFourth.rawValue
                case 4: return _escStatus.voltageFifth.rawValue
                case 5: return _escStatus.voltageSixth.rawValue
                case 6: return _escStatus.voltageSeventh.rawValue
                case 7: return _escStatus.voltageEighth.rawValue
                default: return 0
                }
            }

            function _getMotorCurrent(motorIndex) {
                if (!_escStatus) return 0

                switch (motorIndex) {
                case 0: return _escStatus.currentFirst.rawValue
                case 1: return _escStatus.currentSecond.rawValue
                case 2: return _escStatus.currentThird.rawValue
                case 3: return _escStatus.currentFourth.rawValue
                case 4: return _escStatus.currentFifth.rawValue
                case 5: return _escStatus.currentSixth.rawValue
                case 6: return _escStatus.currentSeventh.rawValue
                case 7: return _escStatus.currentEighth.rawValue
                default: return 0
                }
            }

            ColumnLayout {
                spacing:                    ScreenTools.defaultFontPixelHeight
                implicitWidth:              _leftColumnWidth

                // mixer ui
                RowLayout {
                    width:                      _leftColumnWidth
                    visible:                    actuators.mixer.groups.count > 0
                    QGCLabel {
                        text:                   qsTr("Geometry") + (actuators.mixer.title ? ": " + actuators.mixer.title : "")
                        font.pointSize:         ScreenTools.mediumFontPointSize
                        Layout.fillWidth:       true
                    }
                    QGCLabel {
                        text:                   "<a href='"+actuators.mixer.helpUrl+"'>?</a>"
                        font.pointSize:         ScreenTools.mediumFontPointSize
                        visible:                actuators.mixer.helpUrl
                        textFormat:             Text.RichText
                        onLinkActivated: (link) => {
                            Qt.openUrlExternally(link);
                        }
                    }
                }

                Rectangle {
                    implicitWidth:          _leftColumnWidth
                    implicitHeight:         mixerUi.height + (_margins * 2)
                    color:                  qgcPal.windowShade
                    visible:                actuators.mixer.groups.count > 0

                    Column {
                        id:                 mixerUi
                        spacing:            _margins
                        anchors {
                            left:           parent.left
                            leftMargin:     _margins
                            verticalCenter: parent.verticalCenter
                        }
                        enabled:            !safetySwitch.checked && !actuators.motorAssignmentActive
                        Repeater {
                            model:          actuators.mixer.groups
                            ColumnLayout {
                                property var mixerGroup: object

                                RowLayout {
                                    QGCLabel {
                                        text:                    mixerGroup.label
                                        font.bold:               true
                                        rightPadding:            ScreenTools.defaultFontPixelWidth * 3
                                    }
                                    ActuatorFact {
                                        property var countParam: mixerGroup.countParam
                                        visible:                 countParam != null
                                        fact:                    countParam ? countParam.fact : null
                                    }
                                }

                                GridLayout {
                                    rows:       1 + mixerGroup.channels.count
                                    columns:    1 + mixerGroup.channelConfigs.count

                                    QGCLabel {
                                        text:   ""
                                    }

                                    // param config labels
                                    Repeater {
                                        model:              mixerGroup.channelConfigs
                                        QGCLabel {
                                            text:           object.label
                                            visible:        object.visible && (_showAdvanced || !object.advanced)
                                            Layout.row:     0
                                            Layout.column:  1 + index
                                        }
                                    }
                                    // param instances
                                    Repeater {
                                        model:              mixerGroup.channels
                                        QGCLabel {
                                            text:           object.label + ":"
                                            Layout.row:     1 + index
                                            Layout.column:  0
                                        }
                                    }
                                    Repeater {
                                        model:              mixerGroup.channels
                                        Repeater {
                                            property var channel: object
                                            property var channelIndex: index

                                            model: object.configInstances

                                            ActuatorFact {
                                                fact:           object.fact
                                                Layout.row:     1 + channelIndex
                                                Layout.column:  1 + index
                                                visible:        object.config.visible && (_showAdvanced || !object.config.advanced) && object.visible
                                                enabled:        object.enabled
                                            }
                                        }
                                    }
                                }

                                // extra group config params
                                Repeater {
                                    model: mixerGroup.configParams

                                    RowLayout {
                                        spacing:     ScreenTools.defaultFontPixelWidth
                                        QGCLabel {
                                            text:    object.label + ":"
                                            visible: _showAdvanced || !object.advanced
                                        }
                                        ActuatorFact {
                                            fact: object.fact
                                            visible: _showAdvanced || !object.advanced
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // actuator image
                Image {
                    id:                     actuatorImage
                    source:                 "image://actuators/geometry"+refreshFlag
                    sourceSize.width:       imageSize
                    sourceSize.height:      imageSize
                    Layout.preferredWidth:  imageSize
                    Layout.preferredHeight: imageSize
                    Layout.alignment:       Qt.AlignHCenter
                    visible:                actuators.isMultirotor
                    cache:                  false

                    property var refreshFlag:         actuators.imageRefreshFlag
                    readonly property real imageSize: 9 * ScreenTools.defaultFontPixelHeight

                    MouseArea {
                        anchors.fill:  parent
                        onClicked: (mouse) => {
                            if (mouse.button == Qt.LeftButton) {
                                actuators.imageClicked(Qt.size(width, height), mouse.x, mouse.y);
                            }
                        }
                    }
                }

                // actuator testing
                QGCLabel {
                    text:               qsTr("Actuator Testing")
                    font.pointSize:     ScreenTools.mediumFontPointSize
                }

                Rectangle {
                    implicitWidth:            _leftColumnWidth
                    implicitHeight:           actuatorTesting.height + (_margins * 2)
                    color:                    qgcPal.windowShade

                    Column {
                        id:                   actuatorTesting
                        spacing:              _margins
                        anchors {
                            left:             parent.left
                            leftMargin:       _margins
                            verticalCenter:   parent.verticalCenter
                        }

                        QGCLabel {
                            text: qsTr("Configure some outputs in order to test them.")
                            visible: actuators.actuatorTest.actuators.count == 0
                        }

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            visible: actuators.actuatorTest.actuators.count > 0

                            Switch {
                                id:      safetySwitch
                                enabled: !actuators.motorAssignmentActive &&  !actuators.actuatorTest.hadFailure
                                Connections {
                                    target: actuators.actuatorTest
                                    onHadFailureChanged: {
                                        if (actuators.actuatorTest.hadFailure) {
                                            safetySwitch.checked = false;
                                            safetySwitch.switchUpdated();
                                        }
                                    }
                                }
                                onClicked: {
                                    switchUpdated();
                                }
                                function switchUpdated() {
                                    if (!checked) {
                                        for (var channelIdx=0; channelIdx<sliderRepeater.count; channelIdx++) {
                                            sliderRepeater.itemAt(channelIdx).stop();
                                        }
                                        if (allMotorsLoader.item != null)
                                            allMotorsLoader.item.stop();
                                    }
                                    actuators.actuatorTest.setActive(checked);
                                }
                            }

                            QGCLabel {
                                color:  qgcPal.warningText
                                text: safetySwitch.checked ? qsTr("Careful: Actuator sliders are enabled") : qsTr("Propellers are removed - Enable sliders")
                            }
                        } // Row

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth * 2
                            enabled: safetySwitch.checked

                            // (optional) slider for all motors
                            Loader {
                                id:                allMotorsLoader
                                sourceComponent:   actuators.actuatorTest.allMotorsActuator ?  allMotorsComponent : null
                                Layout.alignment:  Qt.AlignTop
                            }
                            Component {
                                id:                allMotorsComponent
                                ActuatorSlider {
                                    channel:       actuators.actuatorTest.allMotorsActuator
                                    motorIndex:    -1  // Special value for "all" slider
                                    escStatus:     _escStatus
                                    onActuatorValueChanged: {
                                        stopTimer();
                                        for (var channelIdx=0; channelIdx<sliderRepeater.count; channelIdx++) {
                                            var channelSlider = sliderRepeater.itemAt(channelIdx);
                                            if (channelSlider.channel.isMotor) {
                                                channelSlider.value = sliderValue;
                                            }
                                        }
                                    }
                                }
                            }

                            // all channels
                            Repeater {
                                id:         sliderRepeater
                                model:      actuators.actuatorTest.actuators

                                ActuatorSlider {
                                    channel: object
                                    motorIndex: object.isMotor ? index : -1
                                    escStatus: _escStatus
                                    onActuatorValueChanged: (value) =>{
                                        if (isNaN(value)) {
                                            actuators.actuatorTest.stopControl(index);
                                            stop();
                                        } else {
                                            actuators.actuatorTest.setChannelTo(index, value);
                                        }
                                    }
                                }
                            }
                        } // Row

                        // actuator actions
                        Column {
                            visible: actuators.actuatorActions.count > 0
                            enabled: !safetySwitch.checked && !actuators.motorAssignmentActive
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth * 2
                                Repeater {
                                    model: actuators.actuatorActions

                                    QGCButton {
                                        property var actionGroup: object
                                        text:          actionGroup.label
                                        onClicked:     actionMenu.popup()
                                        QGCMenu {
                                            id:                 actionMenu

                                            Instantiator {
                                                model:              actionGroup.actions
                                                QGCMenuItem {
                                                    text:           object.label
                                                    onTriggered:    object.trigger()
                                                }
                                                onObjectAdded:      actionMenu.insertItem(index, object)
                                                onObjectRemoved:    actionMenu.removeItem(object)
                                            }
                                        }
                                    }
                                }
                            }

                        } // Column

                    } // Column
                } // Rectangle
            }

            // Right column
            Column {
                QGCLabel {
                    text:               qsTr("Actuator Outputs")
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    bottomPadding:      ScreenTools.defaultFontPixelHeight
                }
                QGCLabel {
                    text:          qsTr("One or more actuator still needs to be assigned to an output.")
                    visible:       actuators.hasUnsetRequiredFunctions
                    color:         qgcPal.warningText
                    bottomPadding: ScreenTools.defaultFontPixelHeight
                }

                // actuator output selection tabs
                QGCTabBar {
                    Repeater {
                        model: actuators.actuatorOutputs
                        QGCTabButton {
                            text:      '   ' + object.label + '   '
                            width:     implicitWidth
                        }
                    }
                    onCurrentIndexChanged: {
                        actuators.selectActuatorOutput(currentIndex)
                    }
                }

                // actuator outputs
                Rectangle {
                    id:                             selActuatorOutput
                    implicitWidth:                  actuatorGroupColumn.width + (_margins * 2)
                    implicitHeight:                 actuatorGroupColumn.height + (_margins * 2)
                    color:                          qgcPal.windowShade

                    property var actuatorOutput:    actuators.selectedActuatorOutput

                    Column {
                        id:               actuatorGroupColumn
                        spacing:          _margins
                        anchors.centerIn: parent

                        // Motor assignment
                        Row {
                            visible:           actuators.isMultirotor
                            enabled:           !safetySwitch.checked
                            anchors.right:     parent.right
                            spacing:           _margins
                            QGCButton {
                                text:          qsTr("Identify & Assign Motors")
                                visible:       !actuators.motorAssignmentActive && selActuatorOutput.actuatorOutput.groupsVisible
                                enabled:       actuators.motorAssignmentEnabled
                                onClicked: {
                                    var success = actuators.initMotorAssignment()
                                    if (success) {
                                        motorAssignmentConfirmDialog.open()
                                    } else {
                                        motorAssignmentFailureDialog.open()
                                    }
                                }
                                MessageDialog {
                                    id:         motorAssignmentConfirmDialog
                                    visible:    false
                                    //icon:       StandardIcon.Warning
                                    buttons:    MessageDialog.Yes | MessageDialog.No
                                    title:      qsTr("Motor Order Identification and Assignment")
                                    text:       actuators.motorAssignmentMessage
                                    onButtonClicked: function (button, role) {
                                        switch (button) {
                                        case MessageDialog.Yes:
                                            actuators.startMotorAssignment()
                                            break;
                                        }
                                    }
                                }
                                MessageDialog {
                                    id:         motorAssignmentFailureDialog
                                    visible:    false
                                    //icon:       StandardIcon.Critical
                                    buttons:    MessageDialog.Ok
                                    title:      qsTr("Error")
                                    text:       actuators.motorAssignmentMessage
                                }
                            }
                            QGCButton {
                                text:          qsTr("Spin Motor Again")
                                visible:       actuators.motorAssignmentActive
                                onClicked: {
                                    actuators.spinCurrentMotor()
                                }
                            }
                            QGCButton {
                                text:          qsTr("Abort")
                                visible:       actuators.motorAssignmentActive
                                onClicked: {
                                    actuators.abortMotorAssignment()
                                }
                            }
                        }

                        Column {
                            enabled:          !safetySwitch.checked && !actuators.motorAssignmentActive
                            spacing:          _margins

                            RowLayout {
                                property var enableParam:     selActuatorOutput.actuatorOutput.enableParam
                                QGCLabel {
                                    visible:                  parent.enableParam != null
                                    text:                     parent.enableParam ? parent.enableParam.label + ":" : ""
                                }
                                ActuatorFact {
                                    visible:                  parent.enableParam != null
                                    fact:                     parent.enableParam ?  parent.enableParam.fact : null
                                }
                            }


                            Repeater {
                                model: selActuatorOutput.actuatorOutput.subgroups

                                ColumnLayout {
                                    property var subgroup: object
                                    visible:               selActuatorOutput.actuatorOutput.groupsVisible

                                    RowLayout {
                                        visible: subgroup.label != ""
                                        QGCLabel {
                                            text:                    subgroup.label
                                            font.bold:               true
                                            rightPadding:            ScreenTools.defaultFontPixelWidth * 3
                                        }
                                        ActuatorFact {
                                            property var primaryParam: subgroup.primaryParam
                                            visible:                   primaryParam != null
                                            fact:                      primaryParam ? primaryParam.fact : null
                                        }
                                    }

                                    GridLayout {
                                        rows:      1 + subgroup.channels.count
                                        columns:   1 + subgroup.channelConfigs.count

                                        QGCLabel {
                                            text: ""
                                        }

                                        // param config labels
                                        Repeater {
                                            model: subgroup.channelConfigs
                                            QGCLabel {
                                                text:           object.label
                                                visible:        object.visible && (_showAdvanced || !object.advanced)
                                                Layout.row:     0
                                                Layout.column:  1 + index
                                            }
                                        }
                                        // param instances
                                        Repeater {
                                            model: subgroup.channels
                                            QGCLabel {
                                                text:            object.label + ":"
                                                Layout.row:      1 + index
                                                Layout.column:   0
                                            }
                                        }
                                        Repeater {
                                            model: subgroup.channels
                                            Repeater {
                                                property var channel:      object
                                                property var channelIndex: index
                                                model:                     object.configInstances
                                                ActuatorFact {
                                                    fact:           object.fact
                                                    Layout.row:     1 + channelIndex
                                                    Layout.column:  1 + index
                                                    visible:        object.config.visible && (_showAdvanced || !object.config.advanced)
                                                }
                                            }
                                        }
                                    }

                                    // extra subgroup config params
                                    Repeater {
                                        model: subgroup.configParams

                                        RowLayout {
                                            QGCLabel {
                                                text: object.label + ":"
                                            }
                                            ActuatorFact {
                                                fact: object.fact
                                            }
                                        }
                                    }

                                }
                            } // subgroup Repeater

                            // extra actuator config params
                            Repeater {
                                model: selActuatorOutput.actuatorOutput.configParams

                                RowLayout {
                                    QGCLabel {
                                        text: object.label + ":"
                                    }
                                    ActuatorFact {
                                        fact: object.fact
                                    }
                                }
                            }

                            // notes
                            Repeater {
                                model: selActuatorOutput.actuatorOutput.notes
                                ColumnLayout {
                                    spacing: ScreenTools.defaultFontPixelHeight
                                    QGCLabel {
                                        text:       modelData
                                        color:      qgcPal.warningText
                                    }
                                }
                            }
                        }
                    }
                } // Rectangle
            } // Column
        } // Row

    }
}
