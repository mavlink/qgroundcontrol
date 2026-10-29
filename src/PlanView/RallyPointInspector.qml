import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Inspector of the current rally point
Item {
    id: root

    required property var page
    property bool drillIn: false

    signal backRequested

    ColumnLayout {
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth / 2
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5

        PlanInspectorHeader {
            Layout.fillWidth:   true
            namePrefix:         "planPanel_rallyInspector"
            title:              qsTr("Rally Point %1").arg(root.page.currentIndex + 1)
            drillIn:            root.drillIn
            canStepPrevious:    root.page.currentIndex > 0
            canStepNext:        root.page.currentIndex >= 0 && root.page.currentIndex < root.page.points.count - 1

            onBackRequested:        root.backRequested()
            onPreviousRequested:    root.page.step(-1)
            onNextRequested:        root.page.step(1)
            onDeleteRequested: {
                if (mainWindow.allowViewSwitch()) {
                    root.page.deleteCurrent()
                }
            }
        }

        QGCFlickable {
            id: flickable

            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      fieldsColumn.height
            clip:               true

            ColumnLayout {
                id: fieldsColumn

                width:      flickable.width
                spacing:    ScreenTools.defaultFontPixelHeight * 0.5

                LabelledFactTextField {
                    objectName:         "planPanel_rallyField_0"
                    Layout.fillWidth:   true
                    visible:            root.page.current !== null
                    fact:               root.page.current ? root.page.current.latitude : null
                }

                LabelledFactTextField {
                    objectName:         "planPanel_rallyField_1"
                    Layout.fillWidth:   true
                    visible:            root.page.current !== null
                    fact:               root.page.current ? root.page.current.longitude : null
                }

                Loader {
                    Layout.fillWidth:   true
                    active:             root.page.current !== null

                    sourceComponent: RallyPointAltitudeEditor {
                        rallyPoint: root.page.current
                        vehicle:    root.page.planMasterController.controllerVehicle
                    }
                }
            }
        }
    }
}
