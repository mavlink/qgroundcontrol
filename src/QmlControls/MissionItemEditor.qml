import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQml
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Mission item edit control
Rectangle {
    id:             _root
    height:         _currentItem ? (editorLoader.y + editorLoader.height + _innerMargin) : (topRowLayout.y + topRowLayout.height + _margin)
    color:          _currentItem ? qgcPal.buttonHighlight : qgcPal.windowShade
    radius:         _radius
    opacity:        _currentItem ? 1.0 : 0.7
    border.width:   _readyForSave ? 0 : 2
    border.color:   qgcPal.warningText

    property var    map                 ///< Map control
    property var    masterController
    property var    missionItem         ///< MissionItem associated with this editor
    property bool   readOnly            ///< true: read only view, false: full editing view

    signal clicked
    signal remove
    signal selectNextNotReadyItem

    property var    _masterController:          masterController
    property var    _missionController:         _masterController.missionController
    property bool   _currentItem:               missionItem.isCurrentItem
    property color  _outerTextColor:            _currentItem ? qgcPal.buttonHighlightText : qgcPal.text
    property bool   _noMissionItemsAdded:       ListView.view.model.count === 1
    property real   _sectionSpacer:             ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings
    property bool   _singleComplexItem:         _missionController.complexMissionItemNames.length === 1
    property bool   _readyForSave:              missionItem.readyForSaveState === VisualMissionItem.ReadyForSave

    readonly property real  _editFieldWidth:    Math.min(width - _innerMargin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _innerMargin:       2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _hamburgerSize:     commandPicker.height * 0.75
    readonly property real  _trashSize:         commandPicker.height * 0.75
    readonly property bool  _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    FocusScope {
        id:             currentItemScope
        anchors.fill:   parent

        MouseArea {
            anchors.fill:   parent
            onClicked: {
                if (mainWindow.allowViewSwitch()) {
                    currentItemScope.focus = true
                    _root.clicked()
                }
            }
        }
    }

    Component {
        id: editPositionDialog

        EditPositionDialog {
            coordinate:             missionItem.isSurveyItem ?  missionItem.centerCoordinate : missionItem.coordinate
            onCoordinateChanged:    missionItem.isSurveyItem ?  missionItem.centerCoordinate = coordinate : missionItem.coordinate = coordinate
        }
    }

    Row {
        id:                 topRowLayout
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.top:        parent.top
        spacing:            _margin

        Rectangle {
            id:                     notReadyForSaveIndicator
            anchors.verticalCenter: parent.verticalCenter
            width:                  _hamburgerSize
            height:                 width
            border.width:           1
            border.color:           qgcPal.warningText
            color:                  "white"
            radius:                 width / 2
            visible:                !_readyForSave

            QGCLabel {
                id:                 readyForSaveLabel
                anchors.centerIn:   parent
                //: Indicator in Plan view to show mission item is not ready for save/send
                text:               qsTr("?")
                color:              qgcPal.warningText
                font.pointSize:     ScreenTools.smallFontPointSize
            }
        }

        QGCColoredImage {
            id:                     deleteButton
            anchors.verticalCenter: parent.verticalCenter
            height:                 _hamburgerSize
            width:                  height
            sourceSize.height:      height
            fillMode:               Image.PreserveAspectFit
            mipmap:                 true
            smooth:                 true
            color:                  qgcPal.buttonHighlightText
            visible:                _currentItem && missionItem.sequenceNumber !== 0
            source:                 "/res/TrashDelete.svg"

            QGCMouseArea {
                fillItem:   parent
                onClicked:  remove()
            }
        }

        Item {
            id:                     commandPicker
            anchors.verticalCenter: parent.verticalCenter
            height:                 ScreenTools.implicitComboBoxHeight
            width:                  innerLayout.width
            visible:                !commandLabel.visible

            RowLayout {
                id:                     innerLayout
                anchors.verticalCenter: parent.verticalCenter
                spacing:                _padding

                property real _padding: ScreenTools.comboBoxPadding

                QGCLabel { text: missionItem.commandName }

                QGCColoredImage {
                    height:             ScreenTools.defaultFontPixelWidth
                    width:              height
                    fillMode:           Image.PreserveAspectFit
                    smooth:             true
                    antialiasing:       true
                    color:              qgcPal.text
                    source:             "/qmlimages/arrow-down.png"
                }
            }

            QGCMouseArea {
                fillItem:   parent
                onClicked:  commandDialog.createObject(mainWindow).open()
            }

            Component {
                id: commandDialog

                MissionCommandDialog {
                    vehicle:                    masterController.controllerVehicle
                    missionItem:                _root.missionItem
                    map:                        _root.map
                    // FIXME: Disabling fly through commands doesn't work since you may need to change from an RTL to something else
                    flyThroughCommandsAllowed:  true //_missionController.flyThroughCommandsAllowed
                }
            }
        }

        QGCLabel {
            id:                     commandLabel
            anchors.verticalCenter: parent.verticalCenter
            width:                  commandPicker.width
            height:                 commandPicker.height
            visible:                !missionItem.isCurrentItem || !missionItem.isSimpleItem || _waypointsOnlyMode || missionItem.isTakeoffItem
            verticalAlignment:      Text.AlignVCenter
            text:                   missionItem.commandName
            color:                  _outerTextColor
        }
    }

    Component {
        id: hamburgerMenuDropPanelComponent

        DropPanel {
            id: hamburgerMenuDropPanel
            onClosed: destroy()

            sourceComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Move to vehicle position")
                        enabled:            _activeVehicle && missionItem.specifiesCoordinate

                        onClicked: {
                            missionItem.coordinate = _activeVehicle.coordinate
                            hamburgerMenuDropPanel.close()
                        }

                        property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Move to previous item position")
                        enabled:            _missionController.previousCoordinate.isValid
                        onClicked: {
                            missionItem.coordinate = _missionController.previousCoordinate
                            hamburgerMenuDropPanel.close()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Edit position...")
                        enabled:            missionItem.specifiesCoordinate
                        onClicked: {
                            editPositionDialog.createObject(mainWindow).open()
                            hamburgerMenuDropPanel.close()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth:       true
                        Layout.preferredHeight: 1
                        color:                  qgcPal.groupBorder
                    }

                    QGCCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               qsTr("Show all values")
                        visible:            QGroundControl.corePlugin.showAdvancedUI
                        checked:            missionItem.isSimpleItem ? missionItem.rawEdit : false
                        enabled:            missionItem.isSimpleItem && !_waypointsOnlyMode

                        onClicked: {
                            missionItem.rawEdit = checked
                            if (missionItem.rawEdit && !missionItem.friendlyEditAllowed) {
                                missionItem.rawEdit = false
                                checked = false
                                mainWindow.showMessageDialog(qsTr("Mission Edit"), qsTr("You have made changes to the mission item which cannot be shown in Simple Mode"))
                            }
                            hamburgerMenuDropPanel.close()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth:       true
                        Layout.preferredHeight: 1
                        color:                  qgcPal.groupBorder
                    }

                    QGCLabel {
                        text:       qsTr("Item #%1").arg(missionItem.sequenceNumber)
                        enabled:    false
                    }
                }
            }
        }
    }

    Component {
        id: missionStartHamburgerMenuDropPanelComponent

        DropPanel {
            id: missionStartHamburgerMenuDropPanel
            onClosed: destroy()

            sourceComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Offset mission...")
                        onClicked: {
                            offsetMissionDialogComponent.createObject(mainWindow).open()
                            missionStartHamburgerMenuDropPanel.close()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Rotate mission...")
                        onClicked: {
                            rotateMissionDialogComponent.createObject(mainWindow).open()
                            missionStartHamburgerMenuDropPanel.close()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Edit mission position...")
                        onClicked: {
                            editMissionPositionDialogComponent.createObject(
                                        mainWindow,
                                        { coordinate: _missionController.plannedHomePosition }).open()
                            missionStartHamburgerMenuDropPanel.close()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: offsetMissionDialogComponent

        QGCPopupDialog {
            id: root
            title: qsTr("Offset Mission")
            buttons: Dialog.Cancel | Dialog.Ok
            modal: true

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
            property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 20
            property real _labelWidth: ScreenTools.defaultFontPixelWidth * 14

            ColumnLayout {
                spacing: _margin

                RowLayout {
                    spacing: _margin
                    Layout.fillWidth: true

                    QGCLabel {
                        text: qsTr("East (m)")
                        Layout.preferredWidth: _labelWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    }

                    QGCTextField {
                        id: eastField
                        text: "0"
                        Layout.preferredWidth: _textFieldWidth
                        Layout.fillWidth: true
                        inputMethodHints: Qt.ImhPreferNumbers
                        validator: DoubleValidator {
                            notation: DoubleValidator.StandardNotation
                            locale: Qt.locale()
                        }
                    }
                }

                RowLayout {
                    spacing: _margin
                    Layout.fillWidth: true

                    QGCLabel {
                        text: qsTr("North (m)")
                        Layout.preferredWidth: _labelWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    }

                    QGCTextField {
                        id: northField
                        text: "0"
                        Layout.preferredWidth: _textFieldWidth
                        Layout.fillWidth: true
                        inputMethodHints: Qt.ImhPreferNumbers
                        validator: DoubleValidator {
                            notation: DoubleValidator.StandardNotation
                            locale: Qt.locale()
                        }
                    }
                }

                RowLayout {
                    spacing: _margin
                    Layout.fillWidth: true

                    QGCLabel {
                        text: qsTr("Up (m)")
                        Layout.preferredWidth: _labelWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    }

                    QGCTextField {
                        id: upField
                        text: "0"
                        Layout.preferredWidth: _textFieldWidth
                        Layout.fillWidth: true
                        inputMethodHints: Qt.ImhPreferNumbers
                        validator: DoubleValidator {
                            notation: DoubleValidator.StandardNotation
                            locale: Qt.locale()
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    QGCCheckBox {
                        id: offsetTakeoffCheck
                        text: qsTr("Also move takeoff items")
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    }

                    QGCCheckBox {
                        id: offsetLandingCheck
                        text: qsTr("Also move landing items")
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    }
                }
            }

            onAccepted: {
                var east = Number.fromLocaleString(Qt.locale(), eastField.text)
                var north = Number.fromLocaleString(Qt.locale(), northField.text)
                var up = Number.fromLocaleString(Qt.locale(), upField.text)

                if (isNaN(east)) east = 0
                if (isNaN(north)) north = 0
                if (isNaN(up)) up = 0

                _missionController.offsetMission(
                    east,
                    north,
                    up,
                    offsetTakeoffCheck.checked,
                    offsetLandingCheck.checked
                )
            }
        }
    }

    Component {
        id: rotateMissionDialogComponent

        QGCPopupDialog {
            id: root
            title: qsTr("Rotate Mission")
            buttons: Dialog.Cancel | Dialog.Ok
            modal: true

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
            property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 20
            property real _labelWidth: ScreenTools.defaultFontPixelWidth * 14

            ColumnLayout {
                spacing: _margin

                RowLayout {
                    spacing: _margin
                    Layout.fillWidth: true

                    QGCLabel {
                        text: qsTr("Clockwise (°)")
                        Layout.preferredWidth: _labelWidth
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    }

                    QGCTextField {
                        id: degreesCWField
                        text: "0"
                        Layout.preferredWidth: _textFieldWidth
                        Layout.fillWidth: true
                        inputMethodHints: Qt.ImhPreferNumbers
                        validator: DoubleValidator {
                            notation: DoubleValidator.StandardNotation
                            locale: Qt.locale()
                        }
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    QGCCheckBox {
                        id: rotateTakeoffCheck
                        text: qsTr("Also move takeoff items")
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    }

                    QGCCheckBox {
                        id: rotateLandingCheck
                        text: qsTr("Also move landing items")
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    Layout.maximumWidth: _labelWidth + _textFieldWidth

                    wrapMode: Text.WordWrap
                    font.pointSize: ScreenTools.smallFontPointSize

                    text: qsTr("Note: Complex items are rotated by moving their reference coordinate: their geometry and orientation are not changed.")
                }
            }

            onAccepted: {
                var degreesCW = Number.fromLocaleString(Qt.locale(), degreesCWField.text)

                if (isNaN(degreesCW)) degreesCW = 0

                _missionController.rotateMission(
                    degreesCW,
                    rotateTakeoffCheck.checked,
                    rotateLandingCheck.checked
                )
            }
        }
    }

    Component {
        id: editMissionPositionDialogComponent

        EditPositionDialog {
            id: missionEditPositionDialog

            onCoordinateChanged: _missionController.repositionMission(coordinate)
        }
    }


    QGCColoredImage {
        id:                     hamburger
        anchors.margins:        _margin
        anchors.right:          parent.right
        anchors.verticalCenter: topRowLayout.verticalCenter
        width:                  _hamburgerSize
        height:                 _hamburgerSize
        sourceSize.height:      _hamburgerSize
        source:                 "qrc:/qmlimages/Hamburger.svg"
        visible:                missionItem.isCurrentItem
        color:                  qgcPal.buttonHighlightText

        QGCMouseArea {
            fillItem:   hamburger
            onClicked: (position) => {
                currentItemScope.focus = true
                position = Qt.point(position.x, position.y)
                // For some strange reason using mainWindow in mapToItem doesn't work, so we use globals.parent instead which also gets us mainWindow
                position = mapToItem(globals.parent, position)

                var panelComponent = (missionItem.sequenceNumber === 0)
                           ? missionStartHamburgerMenuDropPanelComponent
                           : hamburgerMenuDropPanelComponent
                var dropPanel = panelComponent.createObject(mainWindow, { clickRect: Qt.rect(position.x, position.y, 0, 0) })
                dropPanel.open()
            }
        }
    }

    /*
    QGCLabel {
        id:                     notReadyForSaveLabel
        anchors.margins:        _margin
        anchors.left:           notReadyForSaveIndicator.right
        anchors.right:          parent.right
        anchors.top:            commandPicker.bottom
        visible:                _currentItem && !_readyForSave
        text:                   missionItem.readyForSaveState === VisualMissionItem.NotReadyForSaveTerrain ?
                                    qsTr("Incomplete: Waiting on terrain data.") :
                                    qsTr("Incomplete: Item not fully specified.")
        wrapMode:               Text.WordWrap
        horizontalAlignment:    Text.AlignHCenter
        color:                  qgcPal.warningText
    }

*/

    Loader {
        id:                 editorLoader
        anchors.margins:    _innerMargin
        anchors.left:       parent.left
        anchors.top:        topRowLayout.bottom
        source:             _currentItem ? missionItem.editorQml : ""
        asynchronous:       true

        property var    masterController:   _masterController
        property real   availableWidth:     _root.width - (anchors.margins * 2) ///< How wide the editor should be
        property var    editorRoot:         _root
    }
}
