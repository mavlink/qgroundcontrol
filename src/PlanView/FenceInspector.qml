import QtQuick
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Inspector of the selected fence or the breach return point
Item {
    id: root

    required property var page
    property bool drillIn: false

    signal backRequested

    readonly property string _kind:     page.selectedKind
    readonly property var   _fence:     page.selectedObject
    readonly property var   _controller: page.controller

    QGCPalette { id: qgcPal }

    Fact { id: emptyFact }

    ColumnLayout {
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5

        PlanInspectorHeader {
            Layout.fillWidth:   true
            namePrefix:         "planPanel_fenceInspector"
            drillIn:            root.drillIn
            deletable:          root._kind !== "breach"
            canStepPrevious:    root.page.selectedEntryIndex > 0
            canStepNext:        root.page.selectedEntryIndex >= 0 && root.page.selectedEntryIndex < root.page.entries.length - 1
            title: {
                switch (root._kind) {
                case "polygon":
                    return qsTr("Polygon %1").arg(root.page.selectedIndex + 1)
                case "circle":
                    return qsTr("Circle %1").arg(root.page.selectedIndex + 1)
                case "breach":
                    return qsTr("Breach return point")
                }
                return ""
            }

            onBackRequested:        root.backRequested()
            onPreviousRequested:    root.page.step(-1)
            onNextRequested:        root.page.step(1)
            onDeleteRequested: {
                if (mainWindow.allowViewSwitch()) {
                    root.page.deleteSelected()
                }
            }
        }

        QGCFlickable {
            id: flickable

            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      bodyColumn.height
            clip:               true

            ColumnLayout {
                id: bodyColumn

                width:      flickable.width
                spacing:    ScreenTools.defaultFontPixelHeight * 0.5

                QGCRadioButtonBar {
                    objectName:     "planPanel_fenceInclusion"
                    model:          [ qsTr("Include"), qsTr("Exclude") ]
                    currentIndex:   root._fence && root._fence.inclusion ? 0 : 1
                    visible:        root._kind === "polygon" || root._kind === "circle"

                    onActivated: (index) => {
                        if (root._fence) {
                            root._fence.inclusion = index === 0
                        }
                    }
                }

                QGCLabel {
                    objectName:         "planPanel_fenceHint"
                    Layout.fillWidth:   true
                    visible:            root._kind === "polygon" || root._kind === "circle"
                    wrapMode:           Text.WordWrap
                    color:              qgcPal.textSecondary
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               root._kind === "polygon" ?
                                            qsTr("Drag the vertices on the map to reshape the fence.") :
                                            qsTr("Drag the center on the map to move the fence.")
                }

                Loader {
                    Layout.fillWidth:   true
                    active:             root._kind === "circle" && root._fence !== null

                    sourceComponent: LabelledFactTextField {
                        objectName: "planPanel_fenceRadius"
                        label:      qsTr("Radius")
                        // The selection can change to a fence without a radius before this field is torn down
                        fact:       root._fence && root._fence.radius ? root._fence.radius : emptyFact
                    }
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    visible:            root._kind === "breach"
                    wrapMode:           Text.WordWrap
                    color:              qgcPal.textSecondary
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               qsTr("Optional point the vehicle flies to after a fence breach.")
                }

                LabelledFactTextField {
                    objectName:         "planPanel_fenceBreachAltitude"
                    Layout.fillWidth:   true
                    label:              qsTr("Altitude")
                    fact:               root._controller.breachReturnAltitude
                    visible:            root._kind === "breach" && root._controller.breachReturnPoint.isValid
                }

                QGCButton {
                    objectName:         "planPanel_fenceBreachToggle"
                    Layout.fillWidth:   true
                    visible:            root._kind === "breach"
                    text:               root._controller.breachReturnPoint.isValid ? qsTr("Remove breach return point") : qsTr("Set at map center")

                    onClicked: {
                        if (!mainWindow.allowViewSwitch()) {
                            return
                        }
                        if (root._controller.breachReturnPoint.isValid) {
                            root._controller.breachReturnPoint = QtPositioning.coordinate()
                        } else {
                            root._controller.breachReturnPoint = root.page.editorMap.center
                        }
                    }
                }
            }
        }
    }
}
