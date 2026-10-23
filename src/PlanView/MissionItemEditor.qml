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
    required property var    missionItem         ///< MissionItem associated with this editor
    required property var    map                 ///< Map control

    signal clicked
    signal remove
    signal selectNextNotReadyItem
    signal editorExpandedAndLoaded

    id:             _root
    height:         _currentItem ? (editorLoader.y + editorLoader.height + _innerMargin) : (topRowLayout.y + topRowLayout.height + _margin)
    color:          _currentItem ? qgcPal.accent : qgcPal.windowCard
    radius:         _radius
    opacity:        _currentItem ? 1.0 : 0.7
    border.width:   _readyForSave ? 0 : 2
    border.color:   qgcPal.errorText

    property var    _masterController:          missionItem.masterController
    property var    _missionController:         _masterController.missionController
    property bool   _currentItem:               missionItem.isCurrentItem
    property color  _outerTextColor:            _currentItem ? qgcPal.accentText : qgcPal.textPrimary
    property bool   _noMissionItemsAdded:       _missionController.visualItems ? _missionController.visualItems.count <= 1 : true
    property real   _sectionSpacer:             ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings
    property bool   _singleComplexItem:         _missionController.complexMissionItems.length === 1
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

    MissionItemMenu {
        id:          itemMenu
        missionItem: _root.missionItem
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
            color:                  qgcPal.errorIndicator
            radius:                 width / 2
            visible:                !_readyForSave

            QGCLabel {
                id:                 readyForSaveLabel
                anchors.centerIn:   parent
                //: Indicator in Plan view to show mission item is not ready for save/send
                text:               qsTr("?")
                color:              qgcPal.indicatorText
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
            color:                  qgcPal.accentText
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
                    color:              qgcPal.textPrimary
                    source:             "/qmlimages/arrow-down.png"
                }
            }

            QGCMouseArea {
                fillItem:   parent
                onClicked:  commandDialogFactory.open()
            }

            QGCPopupDialogFactory {
                id: commandDialogFactory

                dialogComponent: commandDialog
            }

            Component {
                id: commandDialog

                MissionCommandDialog {
                    vehicle:                    _masterController.controllerVehicle
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

    QGCColoredImage {
        id:                     hamburger
        anchors.margins:        _margin
        anchors.right:          parent.right
        anchors.verticalCenter: topRowLayout.verticalCenter
        width:                  _hamburgerSize
        height:                 _hamburgerSize
        sourceSize.height:      _hamburgerSize
        source:                 "qrc:/qmlimages/Hamburger.svg"
        visible:                missionItem.isCurrentItem && missionItem.sequenceNumber !== 0
        color:                  qgcPal.accentText

        QGCMouseArea {
            fillItem:   hamburger
            onClicked: (position) => {
                currentItemScope.focus = true
                itemMenu.openAt(hamburger, position)
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
        color:                  qgcPal.errorText
    }

*/

    MissionItemEditorLoader {
        id:                 editorLoader
        anchors.margins:    _innerMargin
        anchors.left:       parent.left
        anchors.top:        topRowLayout.bottom
        missionItem:        _root.missionItem
        availableWidth:     _root.width - (anchors.margins * 2)
        active:             _currentItem
    }

    onHeightChanged: {
        if (_currentItem && editorLoader.status === Loader.Ready) {
            _editorHeightSettleTimer.restart()
        }
    }

    Timer {
        id: _editorHeightSettleTimer
        interval: 100
        onTriggered: _root.editorExpandedAndLoaded()
    }
}
