/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtCharts                 2.0
import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

SetupPage {
    id:         keyConfigurationView
    pageComponent:      pageComponent
    pageName:           qsTr("Buttons")
    pageDescription:    qsTr("Button Setup is used to assign Button Functions.")

    property real _gap: 15
    property int _sbusID: 1
    property int _channelID: 0
    property int _keyCount: 0
    property int _inUseSbusID: 0
    property int _inUseChannelID: 0
    property int _inUseKeyId: 0
    property int _dupKeyId: 0
    property bool _saveEnabled: false
    property bool _settingEnabled: true
    property int _minSbusValue: 0
    property int _maxSbusValue: 2047
    property int _minPPMValue: 800
    property int _maxPPMValue: 2200
    property int _switchType: 1
    property variant _sliderValues: []
    property variant _keyIndexes: []

    property var _keyConfiguration: joystickManager.keyConfigurationList[0]
    property var _joystickMessageSender: joystickManager.joystickMessageSender

    Component {
        id: pageComponent
        Rectangle {
                id:              centreWindow
                anchors.fill:    parent
                anchors.margins: ScreenTools.defaultFontPixelWidth
                color:           qgcPal.window

                function loadKeySetting() {
                    _settingEnabled = false;
                    var keyValue;
                    var keyDefaultValue;
                    if (_keyConfiguration.getControlModeByKeyCount(_keyCount) !== _keyConfiguration.getControlMode(_channelID) &&
                        _keyConfiguration.getControlMode(_channelID) !== 0) {
                        controlModeChangeDialog.open();
                        return;
                    }

                    if(_keyCount == 1) {
                        if(_keyConfiguration.getSwitchType(_channelID) > 0)
                            _switchType = _keyConfiguration.getSwitchType(_channelID);
                        else
                            _switchType = 1;//default value
                        singleKeyCombo1.currentIndex = _keyConfiguration.getKeyIndex(_channelID, 1, 1);
                        keyValue = _keyConfiguration.getValue(_channelID, 1, 1);
                        singleKeySlider1.value = _keyConfiguration.sbusEnable ? keyValue : _keyConfiguration.sbusToPPM(keyValue);
                        singleKeyLabel1.text = singleKeySlider1.value;

                        keyDefaultValue = _keyConfiguration.getDefaultValue(_channelID)
                        singleKeySlider2.value = _keyConfiguration.sbusEnable ? keyDefaultValue : _keyConfiguration.sbusToPPM(keyDefaultValue);
                        singleKeyLabel2.text = singleKeySlider2.value;
                        switchTypeGroup.current = _switchType > 1 ? momentaryButton : troggleButton;

                        mainlayout.visible = false;
                        singleKeyLayout.visible = true;
                    } else {
                        var i;
                        for(i = 0; i < _keyCount; i++) {
                            _keyIndexes[i] = _keyConfiguration.getKeyIndex(_channelID, i + 1, _keyCount);
                            keyValue = _keyConfiguration.getValue(_channelID, i + 1, _keyCount);
                            _sliderValues[i] = _keyConfiguration.sbusEnable ? keyValue : _keyConfiguration.sbusToPPM(keyValue);
                        }
                         mainlayout.visible = false;
                         multiKeyLayoutLoader.sourceComponent = multiKeySetting;
                         multiKeyLayoutLoader.visible = true;
                    }
                }

                function checkBeforeSave() {
                    var sbusInUse;
                    var channelInUse;
                    if(_keyCount == 1) {
                        // check duplicated value
                        if(singleKeySlider1.value == singleKeySlider2.value) {
                            dupValueDialog.open();
                            return;
                        }
                        // check key used by other channel
                        _inUseSbusID = _keyConfiguration.sbusOnKey(singleKeyCombo1.currentIndex, _switchType);
                        _inUseChannelID = _keyConfiguration.channelOnKey(singleKeyCombo1.currentIndex, _switchType);
                        _inUseKeyId = singleKeyCombo1.currentIndex;
                        if(_inUseSbusID !== 0 && _inUseSbusID !== _sbusID) {
                            messageDialogTimer.start();
                            return;
                        }
                        if(_inUseChannelID !== 0 && _inUseChannelID !== _channelID) {
                            messageDialogTimer.start();
                            return;
                        }
                    } else {
                        // check duplicated key
                        var i, j;
                        for(i = 0; i < _keyCount - 1; i++) {
                            for(j = i + 1; j < _keyCount; j++) {
                                if(_keyIndexes[i] == _keyIndexes[j]) {
                                    _dupKeyId = _keyIndexes[i];
                                    dupKeyDialog.open();
                                    return;
                                }
                            }
                        }
                        // check duplicated value
                        for(i = 0; i < _keyCount - 1; i++) {
                            for(j = i + 1; j < _keyCount; j++) {
                                if(_sliderValues[i] == _sliderValues[j]) {
                                    dupValueDialog.open();
                                    return;
                                }
                            }
                        }
                        // check key used by other channel
                        for(i = 0; i < _keyCount; i++) {
                            _inUseKeyId = _keyIndexes[i];
                            _inUseSbusID = _keyConfiguration.sbusOnKey(_keyIndexes[i], 0);
                            _inUseChannelID = _keyConfiguration.channelOnKey(_keyIndexes[i], 0);

                            if(_inUseSbusID !== 0 && _inUseSbusID !== _sbusID) {
                                multiKeyLayoutLoader.item.enabled = false;
                                messageDialogTimer.start();
                                return;
                            }
                            if(_inUseChannelID !== 0 && _inUseChannelID !== _channelID) {
                                multiKeyLayoutLoader.item.enabled = false;
                                messageDialogTimer.start();
                                return;
                            }
                        }
                    }
                    _saveEnabled = true;
                }

                function saveChannelKeySetting() {
                    var i;
                    var keyValue;
                    var initialKeyValue;
                    _saveEnabled = false;
                    checkBeforeSave();
                    if(!_saveEnabled) {
                        return;
                    }
                    _keyConfiguration.resetKeySetting(_sbusID, _channelID);

                    if(_keyCount == 1) {
                        keyValue = _keyConfiguration.sbusEnable ? singleKeySlider1.value : _keyConfiguration.ppmToSbus(singleKeySlider1.value);
                        initialKeyValue = _keyConfiguration.sbusEnable ? singleKeySlider2.value : _keyConfiguration.ppmToSbus(singleKeySlider2.value);
                        _keyConfiguration.saveSingleKeySetting(singleKeyCombo1.currentIndex,
                                                               _switchType,
                                                               _channelID, keyValue,
                                                               initialKeyValue);
                    }
                    if(_keyCount > 1) {
                        for(i = 0; i < _keyCount; i++) {
                            keyValue = _keyConfiguration.sbusEnable ? _sliderValues[i] : _keyConfiguration.ppmToSbus(_sliderValues[i]);
                            _keyConfiguration.saveKeySetting(_keyIndexes[i], _channelID, keyValue);
                        }
                    }
                    _keyConfiguration.setChannelDefaultValue(_sbusID, _channelID);

                    singleKeyLayout.visible = false;
                    mainlayout.visible = true;
                    multiKeyLayoutLoader.visible = false;
                    multiKeyLayoutLoader.sourceComponent = null;
                }

                ExclusiveGroup { id: sbusButtonGroup }

                Column {
                    id:             mainlayout
                    anchors.fill:   parent
                    anchors.leftMargin: 20
                    spacing: 10

                    Row {
                        spacing: 22
                        QGCButton {
                            id: sbus1Button
                            text:      qsTr("RC 1")
                            checkable:              true
                            checked:                true

                            onClicked: {
                                sbus1Button.checked = true;
                                sbus2Button.checked = false;
                                _sbusID = 1;
                                _keyConfiguration = joystickManager.keyConfigurationList[0];
                                channelSettingLoader1.visible = true;
                                channelSettingLoader2.visible = false;
                            }
                        }
                        QGCButton {
                            id: sbus2Button
                            text:      qsTr("RC 2")
                            checkable:              true
                            checked:                false

                            onClicked: {
                                sbus1Button.checked = false;
                                sbus2Button.checked = true;
                                _sbusID = 2;
                                _keyConfiguration = joystickManager.keyConfigurationList[1];
                                channelSettingLoader1.visible = false;
                                channelSettingLoader2.visible = true;
                            }
                        }
                        Column {
                            spacing: 100
                            QGCCheckBox {
                                x: 360
                                id: sbusEnable
                                checked: _keyConfiguration.sbusEnable
                                text: qsTr("Display channel values as sbus mode")
                                onClicked: _keyConfiguration.sbusEnable = checked
                             }

                        }
                    }
                    Row {
                        Rectangle {
                            color: "black"
                            height: 1
                            width: 1500
                        }
                    }

                    Loader {
                        id: channelSettingLoader1
                        sourceComponent: channelSetting
                        visible:        true
                    }

                    Loader {
                        id: channelSettingLoader2
                        sourceComponent: channelSetting
                        visible:        false
                    }
                }

                Component {
                    id: channelSetting

                    Column{
                        spacing: 22
                        Row{
                            spacing: 70
                            QGCLabel {
                                text: qsTr("Channel")
                            }
                            QGCLabel {
                                text: qsTr("Control Mode")
                            }
                        }

                        Row {
                            width: availableWidth - 20 - ScreenTools.defaultFontPixelWidth*2
                            spacing: 20
                            QGCFlickable {
                                height: 500
                                width: parent.width*4/5
                                flickableDirection: Flickable.VerticalFlick
                                visible: true
                                clip:               true
                                contentHeight: channelSetColumn.height
                                contentWidth: channelSetColumn.width
                                Column {
                                    id: channelSetColumn
                                    spacing: 20
                                    Repeater {
                                        id:          chrepeater
                                        model:       _keyConfiguration.channelCount

                                        Row{
                                            id:      chrow
                                            spacing: 120

                                            QGCLabel {
                                                anchors.top:       parent.top
                                                anchors.topMargin: _gap
                                                text:              qsTr("CH" + (modelData + _keyConfiguration.getChannelMinNum()))
                                                width:               70
                                            }

                                            QGCComboBox {
                                                id:         chCombo
                                                model:      _keyConfiguration.availableControlModes
                                                width:      250
                                                currentIndex: Math.max(_keyConfiguration.channelKeyCounts[modelData], 0)
                                            }

                                            QGCLabel {
                                                width:     chButton.width
                                                visible:   !chResetButton.visible && !chButton.visible && !chOKButton.visible ? true : false
                                            }
                                            QGCButton {
                                                id:        chButton
                                                text:      qsTr("Settings")
                                                width:     150
                                                visible:   chCombo.currentIndex > 0 && chCombo.currentText != "Scroll Wheel" ? true : false
                                                onClicked:{
                                                    _channelID = modelData + _keyConfiguration.getChannelMinNum();
                                                    _keyCount = chCombo.currentIndex > 1 ? chCombo.currentIndex - 1 : chCombo.currentIndex;
                                                    centreWindow.loadKeySetting();
                                                }
                                            }
                                            QGCButton {
                                                id:        chOKButton
                                                text:      qsTr("OK")
                                                width:     150
                                                visible:   chCombo.currentText == "Scroll Wheel" && chLabel.text != "Scroll Wheel" ? true : false
                                                onClicked:{
                                                    _channelID = modelData + _keyConfiguration.getChannelMinNum();
                                                    scrollWheelDialog.open();
                                                }
                                            }
                                            QGCButton {
                                                id:        chResetButton
                                                text:      qsTr("Reset")
                                                width:     150
                                                visible:   chCombo.currentIndex == 0 && _keyConfiguration.channelKeyCounts[modelData] ? true : false
                                                onClicked:{
                                                    _channelID = modelData + _keyConfiguration.getChannelMinNum();
                                                    channelResetDialog.open();
                                                }
                                            }
                                            QGCLabel {
                                                id:                chLabel
                                                width:             250
                                                anchors.top:       parent.top
                                                anchors.topMargin: _gap
                                                wrapMode:          Label.WordWrap
                                                text:              _keyConfiguration.keySettingStrings[modelData]
                                            }
                                        }
                                    }//Repeater
                                }
                            }//QGCFlickable
                            // Channel monitor
                            Loader {
                                id: channelMonitorLoader
                                sourceComponent: channelMonitor
                            }
                            Component {
                                id: channelMonitor
                                Column {
                                    spacing:        20

                                    QGCLabel { text: qsTr("Channel Monitor") }

                                    Row {
                                        QGCLabel { text: qsTr("RC1:") }
                                    }
                                    Row {
                                        x: 20
                                        visible: _joystickMessageSender.sbusChannelStatus[0] !== ""
                                        QGCLabel {
                                            width: 150
                                            visible: true
                                            wrapMode: Text.WordWrap
                                            text: _joystickMessageSender.sbusChannelStatus[0]
                                        }
                                    }
                                    Row {
                                        QGCLabel { text: qsTr("RC2:") }
                                    }
                                    Row {
                                        x: 20
                                        visible: _joystickMessageSender.sbusChannelStatus[1] !== ""
                                        QGCLabel {
                                            width: 150
                                            visible: true
                                            wrapMode: Text.WordWrap
                                            text: _joystickMessageSender.sbusChannelStatus[1]
                                        }
                                    }
                                }
                            }// Channel monitor
                        }
                    }
                }

                Column{
                    id:                 singleKeyLayout
                    anchors.fill:       parent
                    anchors.leftMargin: 20
                    spacing:            80
                    Row{
                        id:      singleKeyRow1
                        spacing: 100
                        QGCLabel {
                            id:                  singleKeyLabel
                            text:                qsTr("CH" + _channelID)
                            anchors.top:         parent.top
                            anchors.topMargin:   _gap
                             width:              40
                        }
                        QGCComboBox {
                            id:         singleKeyCombo1
                            model:      _keyConfiguration.availableKeys
                            width:      300
                        }
                        QGCLabel {
                            id:                  singleKeyLabel1
                            anchors.top:         parent.top
                            anchors.topMargin:   _gap
                            width:               40
                        }
                        QGCSlider {
                            id:                  singleKeySlider1
                            orientation:         Qt.Horizontal
                            minimumValue:        _keyConfiguration.sbusEnable ? _minSbusValue : _minPPMValue;
                            maximumValue:        _keyConfiguration.sbusEnable ? _maxSbusValue : _maxPPMValue;
                            stepSize:            1
                            width:               800

                            onValueChanged: {
                                singleKeyLabel1.text = singleKeySlider1.value;
                            }
                        }
                    }
                    Row{
                        id:      singleKeyRow2
                        spacing: 100
                        QGCLabel {
                            text:                qsTr(" ")
                            width:               40
                        }
                        QGCLabel {
                            text:                qsTr("Initial value")
                            anchors.top:         parent.top
                            anchors.topMargin:   _gap
                            width:               300
                        }
                        QGCLabel {
                            id:                  singleKeyLabel2
                            anchors.top:         parent.top
                            anchors.topMargin:   _gap
                            width:               40
                        }
                        QGCSlider {
                            id:                   singleKeySlider2
                            orientation:          Qt.Horizontal
                            minimumValue:         _keyConfiguration.sbusEnable ? _minSbusValue : _minPPMValue;
                            maximumValue:         _keyConfiguration.sbusEnable ? _maxSbusValue : _maxPPMValue;
                            stepSize:             1
                            width:                800

                            onValueChanged: {
                                singleKeyLabel2.text = singleKeySlider2.value;
                            }
                        }
                    }
                    ExclusiveGroup {
                        id: switchTypeGroup
                        onCurrentChanged: {
                            switch(current) {
                            case troggleButton:
                                _switchType = 1;
                                break;
                            case momentaryButton:
                                _switchType = 2;
                                break;
                            }
                        }
                    }
                    Row{
                        id:      singleKeyRow4
                        spacing: 100
                        QGCLabel {
                            text:                qsTr(" ")
                            width:               40
                        }
                        QGCLabel {
                            text:                qsTr("Switch Type")
                            width:               300
                        }
                        QGCRadioButton {
                            id: troggleButton
                            checked:            true
                            exclusiveGroup:     switchTypeGroup
                            text:      qsTr("Toggle switch")
                        }
                        QGCRadioButton {
                            id: momentaryButton
                            checked:            false
                            exclusiveGroup:     switchTypeGroup
                            text:      qsTr("Momentary switch")
                        }
                    }
                    Row{
                        spacing: 200
                        QGCLabel {
                            text:      qsTr(" ")
                            width:     40
                        }
                        QGCButton {
                            id:        singleKeySaveButton
                            text:      qsTr("Save")
                            onClicked: {
                                if (_switchType == 2) {
                                    useBothPressDialog.open();
                                } else {
                                    centreWindow.saveChannelKeySetting();
                                }

                            }
                        }
                        QGCButton {
                            id:        singleKeyResetButton
                            text:      qsTr("Reset")
                            onClicked: {
                                channelResetDialog.open();
                            }
                        }
                        QGCButton {
                            id:        singleKeyCancelButton
                            text:      qsTr("Cancel")
                            onClicked: {
                                singleKeyLayout.visible = false;
                                mainlayout.visible = true;
                            }
                        }
                    }
                }

                Loader {
                    id: multiKeyLayoutLoader
                    sourceComponent: null
                    visible:        false
                }
                Component {
                    id: multiKeySetting

                    Item {
                        id: multiKeySettingItem
                        Column {
                            anchors.fill:       parent
                            Rectangle {
                                width:  availableWidth
                                height: 700
                                QGCFlickable {
                                    anchors.fill: parent
                                    visible:      true
                                    clip:         true
                                    contentHeight: multiKeyLayout.height
                                    contentWidth: parent.width
                                    Column {
                                        id:                 multiKeyLayout
                                        anchors.leftMargin: 20
                                        anchors.topMargin:  10
                                        spacing:            30
                                        Repeater {
                                            id:    multiKeyRepeater
                                            model: _keyCount

                                            Row{
                                                id:      multiKeyRow
                                                spacing: 100
                                                QGCLabel {
                                                    id:                  multiKeyLabe
                                                    anchors.top:         parent.top
                                                    anchors.topMargin:   _gap
                                                    width:               40
                                                    text:                qsTr("CH" + _channelID)
                                                }
                                                QGCComboBox {
                                                    id:         multiKeyRowCombox
                                                    model:      _keyConfiguration.availableKeys
                                                    width:      300
                                                    currentIndex: _keyIndexes[modelData]

                                                    onActivated: _keyIndexes[modelData] = index
                                                }
                                                QGCLabel {
                                                    id:                  multiKeyRowLabel
                                                    anchors.top:         parent.top
                                                    anchors.topMargin:   _gap
                                                    width:               40
                                                    text:                _sliderValues[modelData]
                                                }
                                                QGCSlider {
                                                    id:                  multiKeyRowSlider
                                                    orientation:         Qt.Horizontal
                                                    minimumValue:        _keyConfiguration.sbusEnable ? _minSbusValue : _minPPMValue;
                                                    maximumValue:        _keyConfiguration.sbusEnable ? _maxSbusValue : _maxPPMValue;
                                                    stepSize:            1
                                                    width:               800
                                                    value:               _sliderValues[modelData]

                                                    onValueChanged: {
                                                        multiKeyRowLabel.text = multiKeyRowSlider.value;
                                                        _sliderValues[modelData] = multiKeyRowSlider.value;
                                                    }
                                                }
                                            }
                                        }
                                        Row{
                                            spacing: 200
                                            QGCLabel {
                                                text:       qsTr(" ")
                                                width:      40
                                            }

                                            QGCButton {
                                                id:        mulSaveButton
                                                text:      qsTr("Save")
                                                onClicked: {
                                                    centreWindow.saveChannelKeySetting();
                                                }
                                            }
                                            QGCButton {
                                                id:        mulResetButton
                                                text:      qsTr("Reset")
                                                onClicked: {
                                                    channelResetDialog.open();
                                                }
                                            }
                                            QGCButton {
                                                id:        mulCancelButton
                                                text:      qsTr("Cancel")
                                                onClicked: {
                                                    multiKeyLayoutLoader.visible = false;
                                                    multiKeyLayoutLoader.sourceComponent = null;
                                                    mainlayout.visible = true;
                                                }
                                            }
                                        }
                                    }
                                }//QGCFlickable
                            }
                        }//Column
                    }
                }//Component

                Component.onCompleted: {
                    mainlayout.visible = true;
                    singleKeyLayout.visible = false;
                    multiKeyLayoutLoader.visible = false;
                }

                MessageDialog {
                    id:         keyIsOccupiedDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      qsTr("NOTICE")
                    text:       qsTr("\"%1\" has been used by Sbus %2/CH%3. Confirm to clear configuration of Sbus %2/CH%3 and continue?")
                                    .arg(_keyConfiguration.getKeyStringFromIndex(_inUseKeyId)).arg(_inUseSbusID).arg(_inUseChannelID)
                    onYes: {
                        _keyConfiguration.resetKeySetting(_inUseSbusID, _inUseChannelID);
                        centreWindow.saveChannelKeySetting();
                    }
                    onNo: {
                        multiKeyLayoutLoader.item.enabled = true;
                    }
                }
                Timer {
                    id: messageDialogTimer
                    interval: 200
                    repeat: false
                    triggeredOnStart: false
                    running: false
                    onTriggered: {
                        keyIsOccupiedDialog.open();
                    }
                }

                MessageDialog {
                    id:         dupKeyDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes
                    title:      qsTr("NOTICE")
                    text:       qsTr("\"%1\" is defined for multiple values, please check.")
                                    .arg(_keyConfiguration.getKeyStringFromIndex(_dupKeyId))
                }

                MessageDialog {
                    id:         useBothPressDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      qsTr("NOTICE")
                    text:       qsTr("In this switch type, both short press and long press of the key are occupied. Continue?")
                    onYes: {
                        centreWindow.saveChannelKeySetting();
                    }
                    onNo: {
                    }
                }

                MessageDialog {
                    id:         controlModeChangeDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      qsTr("NOTICE")
                    text:       qsTr("The control mode of CH%1 is changed. Confirm to clear configuration of CH%1 and continue?").arg(_channelID)
                    onYes: {
                        _keyConfiguration.resetKeySetting(_sbusID, _channelID);
                        centreWindow.loadKeySetting();
                    }
                    onNo: {
                    }
                }

                MessageDialog {
                    id:         dupValueDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes
                    title:      qsTr("NOTICE")
                    text:       qsTr("There are duplicated values, please check.")
                }

                MessageDialog {
                    id:         scrollWheelDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      qsTr("NOTICE")
                    text:       qsTr("Confirm to configure the scroll wheel to control SBUS%1 CH%2? This will clear configuration of SBUS%1 CH%2 and current scroll wheel setting.").arg(_sbusID).arg(_channelID)
                    onYes: {
                        _keyConfiguration.resetKeySetting(_sbusID, _channelID);
                        _keyConfiguration.resetScrollWheelSetting();
                        _keyConfiguration.saveScollWheelSetting(_channelID);
                    }
                }

                MessageDialog {
                    id:         channelResetDialog
                    visible:    false
                    icon:       StandardIcon.Warning
                    standardButtons: StandardButton.Yes | StandardButton.No
                    title:      qsTr("NOTICE")
                    text:       qsTr("Confirm to clear configuration of CH%1?").arg(_channelID)
                    onYes: {
                        _keyConfiguration.resetKeySetting(_sbusID, _channelID);
                        mainlayout.visible = true;
                        singleKeyLayout.visible = false;
                        multiKeyLayoutLoader.visible = false;
                        multiKeyLayoutLoader.sourceComponent = null;
                    }
                }
        }
    }
} // QGCView

