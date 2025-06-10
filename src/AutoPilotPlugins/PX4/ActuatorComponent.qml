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

                        // Custom slider layout that adapts to motor count
                        Item {
                            id: sliderContainer
                            enabled: safetySwitch.checked
                            width: _leftColumnWidth
                            height: childrenRect.height

                            property int motorCount: {
                                var count = 0
                                for (var i = 0; i < actuators.actuatorTest.actuators.count; i++) {
                                    if (actuators.actuatorTest.actuators.get(i).isMotor) count++
                                }
                                return count
                            }
                            property bool useDoubleRow: motorCount > 4
                            property int totalSliders: motorCount + (actuators.actuatorTest.allMotorsActuator ? 1 : 0)
                            property int slidersPerRow: useDoubleRow ? 4 : Math.min(5, totalSliders)
                            property real sliderWidth: (width - (slidersPerRow - 1) * ScreenTools.defaultFontPixelWidth) / slidersPerRow
                            property real sliderHeight: useDoubleRow ? ScreenTools.defaultFontPixelHeight * 3.5 : ScreenTools.defaultFontPixelHeight * 5.5

                            Component {
                                id: customSliderComponent

                                Column {
                                    width: sliderContainer.sliderWidth
                                    spacing: ScreenTools.defaultFontPixelHeight * 0.15

                                    property var channel
                                    property alias value: slider.value
                                    property int sliderIndex: 0
                                    function stop() {
                                        slider.value = slider.snap ? channel.min - slider.snapRange : channel.defaultValue
                                        sendTimer.stop()
                                    }

                                    // Custom slider without rotated label
                                    QGCSlider {
                                        id: slider
                                        orientation: Qt.Vertical
                                        from: snap ? channel.min - snapRange : channel.min
                                        to: channel.max
                                        stepSize: (channel.max - channel.min) / 100
                                        value: snap ? channel.min - snapRange : channel.defaultValue
                                        live: true
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        height: sliderContainer.sliderHeight
                                        indicatorBarVisible: sendTimer.running

                                        property var snap: isNaN(channel.defaultValue)
                                        property var span: channel.max - channel.min
                                        property var snapRange: span * 0.15
                                        property var blockUpdates: true

                                        onValueChanged: {
                                            if (blockUpdates) return
                                            if (snap) {
                                                if (value < channel.min) {
                                                    if (value < channel.min - snapRange/2) {
                                                        value = channel.min - snapRange
                                                    } else {
                                                        value = channel.min
                                                    }
                                                }
                                            }
                                            sendTimer.start()
                                        }

                                        Timer {
                                            id: sendTimer
                                            interval: 50
                                            triggeredOnStart: true
                                            repeat: true
                                            running: false
                                            onTriggered: {
                                                var sendValue = slider.value
                                                if (sendValue < channel.min - slider.snapRange/2) {
                                                    sendValue = channel.defaultValue
                                                }
                                                parent.parent.actuatorValueChanged(sendValue, slider.value)
                                            }
                                        }

                                        Component.onCompleted: blockUpdates = false
                                    }

                                    // Horizontal label at bottom
                                    QGCLabel {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: channel.label
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        width: parent.width
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideMiddle
                                    }

                                    // ESC telemetry for motors
                                    Column {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        visible: channel.isMotor && sliderIndex >= 0
                                        spacing: ScreenTools.defaultFontPixelHeight * 0.05

                                        QGCLabel {
                                            visible: _isMotorHealthy(sliderIndex)
                                            text: _getMotorRPM(sliderIndex) + "rpm"
                                            font.pointSize: ScreenTools.smallFontPointSize * 0.85
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        QGCLabel {
                                            visible: _isMotorHealthy(sliderIndex)
                                            text: _getMotorVoltage(sliderIndex).toFixed(1) + "V"
                                            font.pointSize: ScreenTools.smallFontPointSize * 0.85
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        QGCLabel {
                                            visible: _isMotorHealthy(sliderIndex)
                                            text: _getMotorCurrent(sliderIndex).toFixed(1) + "A"
                                            font.pointSize: ScreenTools.smallFontPointSize * 0.85
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        QGCLabel {
                                            visible: _escDataAvailable && !_isMotorHealthy(sliderIndex)
                                            text: _isMotorOnline(sliderIndex) ? qsTr("UNHEALTHY") : qsTr("OFFLINE")
                                            font.pointSize: ScreenTools.smallFontPointSize * 0.85
                                            color: qgcPal.colorRed
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }
                                    }

                                    signal actuatorValueChanged(real value, real sliderValue)
                                }
                            }

                            GridLayout {
                                id: sliderGrid
                                anchors.fill: parent
                                columns: sliderContainer.slidersPerRow
                                rows: sliderContainer.useDoubleRow ? 2 : 1
                                columnSpacing: ScreenTools.defaultFontPixelWidth
                                rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5

                                // All motors slider (if exists)
                                Loader {
                                    id: allMotorsLoader
                                    sourceComponent: actuators.actuatorTest.allMotorsActuator ? customSliderComponent : null
                                    onLoaded: {
                                        if (item) {
                                            item.channel = actuators.actuatorTest.allMotorsActuator
                                            item.sliderIndex = -1
                                            item.actuatorValueChanged.connect(function(value, sliderValue) {
                                                for (var channelIdx = 0; channelIdx < sliderRepeater.count; channelIdx++) {
                                                    var sliderComponent = sliderRepeater.itemAt(channelIdx)
                                                    if (sliderComponent && sliderComponent.item && sliderComponent.item.channel.isMotor) {
                                                        sliderComponent.item.value = sliderValue
                                                    }
                                                }
                                            })
                                        }
                                    }
                                }

                                // Individual sliders
                                Repeater {
                                    id: sliderRepeater
                                    model: actuators.actuatorTest.actuators

                                    Loader {
                                        sourceComponent: customSliderComponent
                                        onLoaded: {
                                            if (item) {
                                                item.channel = object
                                                item.sliderIndex = index
                                                item.actuatorValueChanged.connect(function(value, sliderValue) {
                                                    if (isNaN(value)) {
                                                        actuators.actuatorTest.stopControl(index)
                                                        item.stop()
                                                    } else {
                                                        actuators.actuatorTest.setChannelTo(index, value)
                                                    }
                                                })
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // actuator actions
                        Column {
                            topPadding: ScreenTools.defaultFontPixelHeight
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
