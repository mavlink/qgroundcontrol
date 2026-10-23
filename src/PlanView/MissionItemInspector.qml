import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PlanPanelLogic.js" as Logic

/// Inspector for the current plan view mission item: pinned header with the item actions, scrolling editor below
Item {
    id: root

    required property var missionController
    required property var map
    property bool drillIn: false    ///< true: show the back and previous/next buttons

    signal backRequested

    readonly property var   _item:              missionController.currentPlanViewItem
    readonly property int   _itemCount:         missionController.visualItems ? missionController.visualItems.count : 0
    readonly property int   _currentIndex:      missionController.currentPlanViewVIIndex
    readonly property bool  _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly
    readonly property bool  _commandEditable:   _item ? (_item.isSimpleItem && !_waypointsOnlyMode && !_item.isTakeoffItem) : false
    readonly property bool  _deletable:         _item ? _item.sequenceNumber !== 0 : false
    readonly property bool  _ready:             _item ? _item.readyForSaveState === VisualMissionItem.ReadyForSave : true

    function _step(delta) {
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        const index = Logic.steppedIndex(_currentIndex, delta, _itemCount)
        if (index >= 0) {
            missionController.setCurrentPlanViewSeqNum(missionController.visualItems.get(index).sequenceNumber, false)
        }
    }

    function _remove() {
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        if (_item && _deletable) {
            const viIndex = missionController.visualItemIndexForObject(_item)
            if (viIndex > 0) {
                missionController.removeVisualItem(viIndex)
            }
        }
    }

    QGCPalette { id: qgcPal }

    Loader {
        id:         menuLoader
        active:     root._item !== null

        sourceComponent: MissionItemMenu {
            missionItem: root._item
        }
    }

    QGCPopupDialogFactory {
        id: commandDialogFactory

        dialogComponent: commandDialog
    }

    Component {
        id: commandDialog

        MissionCommandDialog {
            vehicle:                    root._item ? root._item.masterController.controllerVehicle : null
            missionItem:                root._item
            map:                        root.map
            flyThroughCommandsAllowed:  true
        }
    }

    ColumnLayout {
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
        spacing:            ScreenTools.defaultFontPixelHeight * 0.4

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth / 2

            PlanPanelIconButton {
                objectName:     "planPanel_inspectorBack"
                accessibleName: qsTr("Back")
                iconSource:     "/InstrumentValueIcons/cheveron-right.svg"
                iconRotation:   180
                visible:        root.drillIn
                onClicked:      root.backRequested()
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               root._item ? qsTr("#%1").arg(root._item.sequenceNumber) : ""
                color:              qgcPal.textSecondary
            }

            PlanPanelIconButton {
                objectName:     "planPanel_inspectorPrevious"
                accessibleName: qsTr("Previous")
                iconSource:     "/InstrumentValueIcons/cheveron-right.svg"
                iconRotation:   -90
                visible:        root.drillIn
                enabled:        Logic.steppedIndex(root._currentIndex, -1, root._itemCount) >= 0
                onClicked:      root._step(-1)
            }

            PlanPanelIconButton {
                objectName:     "planPanel_inspectorNext"
                accessibleName: qsTr("Next")
                iconSource:     "/InstrumentValueIcons/cheveron-right.svg"
                iconRotation:   90
                visible:        root.drillIn
                enabled:        Logic.steppedIndex(root._currentIndex, 1, root._itemCount) >= 0
                onClicked:      root._step(1)
            }

            PlanPanelIconButton {
                id:             menuButton
                objectName:     "planPanel_inspectorMenu"
                accessibleName: qsTr("Menu")
                iconSource:     "qrc:/qmlimages/Hamburger.svg"
                visible:        root._deletable
                onClicked: {
                    if (menuLoader.item) {
                        menuLoader.item.openAt(menuButton, Qt.point(width / 2, height / 2))
                    }
                }
            }

            PlanPanelIconButton {
                objectName:     "planPanel_inspectorDelete"
                accessibleName: qsTr("Delete")
                iconSource:     "/res/TrashDelete.svg"
                danger:         true
                visible:        root._deletable
                onClicked:      root._remove()
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth / 2

            QGCLabel {
                objectName:             "planPanel_inspectorTitle"
                Layout.fillWidth:       true
                text:                   root._item ? root._item.commandName : ""
                font.bold:              true
                font.capitalization:    Font.AllUppercase
                font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
                elide:                  Text.ElideRight
            }

            QGCButton {
                objectName:     "planPanel_inspectorCommand"
                small:          true
                text:           qsTr("Change...")
                visible:        root._commandEditable
                onClicked:      commandDialogFactory.open()
            }
        }

        QGCLabel {
            objectName:         "planPanel_inspectorIncomplete"
            Layout.fillWidth:   true
            visible:            !root._ready
            wrapMode:           Text.WordWrap
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              qgcPal.warningText
            text:               root._item && root._item.readyForSaveState === VisualMissionItem.NotReadyForSaveTerrain ?
                                    qsTr("Incomplete: Waiting on terrain data.") :
                                    qsTr("Incomplete: Item not fully specified.")
        }

        QGCFlickable {
            id: flickable

            objectName:         "planPanel_inspectorFlickable"
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      editorLoader.height
            clip:               true

            MissionItemEditorLoader {
                id: editorLoader

                objectName:     "planPanel_inspectorEditor"
                missionItem:    root._item
                availableWidth: flickable.width
            }
        }
    }
}
