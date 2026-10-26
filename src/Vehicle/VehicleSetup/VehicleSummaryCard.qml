import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// A component's card on the Summary page: its name, whether it still needs setup and its values. A component that
/// does not supply summaryItems shows its summaryQmlSource instead.
Rectangle {
    id: control

    required property var vehicleComponent

    objectName: "vehicleSummary_card_" + vehicleComponent.name

    signal clicked()

    readonly property bool  hasContent:     _items.length > 0 || vehicleComponent.summaryQmlSource.toString() !== ""
    readonly property bool  _needsSetup:    vehicleComponent.requiresSetup && !vehicleComponent.setupComplete
    readonly property bool  _clickable:     vehicleComponent.setupSource.toString() !== ""
    readonly property var   _items:         vehicleComponent.summaryItems
    readonly property var   _sections:      _buildSections(_items)
    readonly property real  _margins:       ScreenTools.defaultFontPixelWidth * 1.5

    implicitWidth:  cardLayout.implicitWidth + _margins * 2
    implicitHeight: cardLayout.implicitHeight + _margins * 2
    radius:         ScreenTools.defaultBorderRadius * 2
    color:          qgcPal.windowCard
    border.color:   _needsSetup ? qgcPal.colorRed : (_clickable && mouseArea.containsMouse) ? qgcPal.accent : qgcPal.borderSubtle

    QGCPalette { id: qgcPal }

    // Items grouped by their group, in order of first appearance. Items without a group make up a section with no title.
    function _buildSections(items) {
        const sections = []
        const byTitle = {}
        for (let i = 0; i < items.length; i++) {
            const title = items[i].group
            if (!(title in byTitle)) {
                byTitle[title] = { title: title, items: [] }
                sections.push(byTitle[title])
            }
            byTitle[title].items.push(items[i])
        }
        return sections
    }

    MouseArea {
        id:             mouseArea
        anchors.fill:   parent
        hoverEnabled:   true
        cursorShape:    control._clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (control._clickable) {
                control.clicked()
            }
        }
    }

    ColumnLayout {
        id:                 cardLayout
        anchors.fill:       parent
        anchors.margins:    control._margins
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            Rectangle {
                implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
                implicitHeight: implicitWidth
                radius:         implicitWidth / 2
                color:          !control.vehicleComponent.requiresSetup ? "transparent" : control._needsSetup ? qgcPal.colorRed : qgcPal.colorGreen
                border.color:   control.vehicleComponent.requiresSetup ? color : qgcPal.textSecondary
            }

            VehicleSummaryLabel {
                Layout.fillWidth:       true
                text:                   control.vehicleComponent.name
                font.bold:              true
                font.capitalization:    Font.AllUppercase
                font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
                elide:                  Text.ElideRight
            }

            QGCChip {
                text:       qsTr("Setup required")
                textColor:  qgcPal.errorText
                visible:    control._needsSetup
            }

            QGCLabel {
                text:       "›"
                color:      qgcPal.textSecondary
                visible:    control._clickable
            }
        }

        Repeater {
            model: control._sections

            Rectangle {
                id:                 section
                Layout.fillWidth:   true
                implicitHeight:     sectionLayout.implicitHeight + _padding * 2
                radius:             ScreenTools.defaultBorderRadius
                color:              "transparent"
                border.color:       _titled ? qgcPal.borderSubtle : "transparent"

                required property var modelData

                readonly property bool  _titled:    modelData.title !== ""
                readonly property real  _padding:   _titled ? ScreenTools.defaultFontPixelWidth : 0

                ColumnLayout {
                    id:                 sectionLayout
                    anchors.fill:       parent
                    anchors.margins:    section._padding
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.3

                    QGCLabel {
                        text:       section.modelData.title
                        font.bold:  true
                        visible:    section._titled
                    }

                    Flow {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelWidth * 0.5

                        Repeater {
                            model: section.modelData.items

                            Rectangle {
                                width:          Math.min(implicitWidth, parent.width)
                                implicitWidth:  cellLayout.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
                                implicitHeight: cellLayout.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.6
                                radius:         ScreenTools.defaultBorderRadius
                                color:          qgcPal.windowInset
                                border.color:   modelData.state === VehicleSummaryState.Attention ? qgcPal.warningText : qgcPal.borderSubtle

                                required property var modelData

                                ColumnLayout {
                                    id:                 cellLayout
                                    anchors.centerIn:   parent
                                    width:              parent.width - ScreenTools.defaultFontPixelWidth * 2
                                    spacing:            0

                                    QGCCaptionLabel {
                                        Layout.fillWidth:   true
                                        text:               modelData.caption
                                        wrapMode:           Text.WordWrap
                                    }

                                    QGCLabel {
                                        Layout.fillWidth:   true
                                        text:               modelData.value
                                        wrapMode:           Text.WordWrap
                                        font.bold:          modelData.state === VehicleSummaryState.Attention
                                        color:              modelData.state === VehicleSummaryState.Attention ? qgcPal.warningText :
                                                                modelData.state === VehicleSummaryState.Muted ? qgcPal.textSecondary : qgcPal.textPrimary
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Loader {
            Layout.fillWidth:       true
            Layout.preferredHeight: item ? item.implicitHeight : 0
            source:                 control._items.length === 0 ? control.vehicleComponent.summaryQmlSource : ""

            property var vehicleComponent: control.vehicleComponent
        }
    }
}
