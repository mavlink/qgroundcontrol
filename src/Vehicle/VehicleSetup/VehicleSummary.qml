import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:                     _summaryRoot
    anchors.fill:           parent
    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
    anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
    color:                  qgcPal.window

    signal componentRequested(var vehicleComponent)

    readonly property var   _vehicle:               QGroundControl.multiVehicleManager.activeVehicle
    readonly property var   _components:            _vehicle ? _vehicle.autopilotPlugin.vehicleComponents : []
    // Components are listed in the order recommended for setting up a vehicle
    readonly property var   _requiredComponents:    _components.filter(vehicleComponent => vehicleComponent.requiresSetup).sort((a, b) => a.setupOrder - b.setupOrder)
    readonly property int   _readyCount:            _requiredComponents.filter(vehicleComponent => vehicleComponent.setupComplete).length
    readonly property int   _remainingCount:        _requiredComponents.length - _readyCount
    readonly property var   _nextComponent:         _findNextComponent()
    readonly property real  _minCardWidth:          ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelWidth * 28 : ScreenTools.defaultFontPixelWidth * 36
    readonly property real  _spacing:               ScreenTools.defaultFontPixelHeight * 0.5
    // Column counts come from the page width rather than the grid's own, which a grid cannot size itself from
    readonly property real  _panelContentWidth:     width - _spacing * 2
    readonly property real  _readinessWidth:        ScreenTools.defaultFontPixelWidth * 34
    readonly property bool  _wideTop:               width >= ScreenTools.defaultFontPixelWidth * 90
    readonly property real  _checklistWidth:        _panelContentWidth - _spacing * 2 - (_wideTop ? _readinessWidth + _spacing : 0)

    function _findNextComponent() {
        for (let i = 0; i < _requiredComponents.length; i++) {
            const vehicleComponent = _requiredComponents[i]
            if (!vehicleComponent.setupComplete && vehicleComponent.setupSource.toString() !== "") {
                return vehicleComponent
            }
        }
        return null
    }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    QGCFlickable {
        anchors.fill:       parent
        contentHeight:      summaryColumn.height
        contentWidth:       _summaryRoot.width
        flickableDirection: Flickable.VerticalFlick

        ColumnLayout {
            id:         summaryColumn
            width:      _summaryRoot.width
            spacing:    _summaryRoot._spacing

            GridLayout {
                Layout.fillWidth:   true
                columns:            _summaryRoot._wideTop ? 2 : 1
                columnSpacing:      _summaryRoot._spacing
                rowSpacing:         _summaryRoot._spacing
                visible:            _summaryRoot._requiredComponents.length > 0

                Rectangle {
                    Layout.fillHeight:      true
                    Layout.preferredWidth:  _summaryRoot._readinessWidth
                    implicitHeight:         readinessLayout.implicitHeight + _summaryRoot._spacing * 2
                    color:                  qgcPal.windowPanel
                    border.color:           qgcPal.borderSubtle
                    radius:                 ScreenTools.defaultBorderRadius * 2

                    ColumnLayout {
                        id:                 readinessLayout
                        anchors.fill:       parent
                        anchors.margins:    _summaryRoot._spacing
                        spacing:            _summaryRoot._spacing

                        QGCCaptionLabel {
                            Layout.alignment:   Qt.AlignHCenter
                            text:               qsTr("Vehicle setup")
                        }

                        Canvas {
                            id:                     ring
                            Layout.alignment:       Qt.AlignHCenter
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 7
                            Layout.preferredHeight: Layout.preferredWidth

                            readonly property real  progress:   _summaryRoot._readyCount / Math.max(1, _summaryRoot._requiredComponents.length)
                            readonly property real  _lineWidth: ScreenTools.defaultFontPixelHeight * 0.6

                            onProgressChanged:  requestPaint()
                            onWidthChanged:     requestPaint()
                            onPaint: {
                                const context = getContext("2d")
                                const center = width / 2
                                const radius = center - _lineWidth / 2
                                context.reset()
                                context.lineWidth = _lineWidth
                                context.strokeStyle = qgcPal.borderSubtle
                                context.beginPath()
                                context.arc(center, center, radius, 0, 2 * Math.PI)
                                context.stroke()
                                if (progress > 0) {
                                    context.strokeStyle = qgcPal.colorGreen
                                    context.beginPath()
                                    context.arc(center, center, radius, -Math.PI / 2, -Math.PI / 2 + 2 * Math.PI * progress)
                                    context.stroke()
                                }
                            }

                            Connections {
                                target: qgcPal
                                function onPaletteChanged() {
                                    ring.requestPaint()
                                }
                            }

                            ColumnLayout {
                                anchors.centerIn:   parent
                                spacing:            0

                                QGCLabel {
                                    Layout.alignment:   Qt.AlignHCenter
                                    text:               qsTr("%1/%2").arg(_summaryRoot._readyCount).arg(_summaryRoot._requiredComponents.length)
                                    font.pointSize:     ScreenTools.largeFontPointSize
                                    font.bold:          true
                                }

                                QGCLabel {
                                    Layout.alignment:   Qt.AlignHCenter
                                    text:               qsTr("required")
                                    font.pointSize:     ScreenTools.smallFontPointSize
                                    color:              qgcPal.textSecondary
                                }
                            }
                        }

                        QGCChip {
                            Layout.alignment:   Qt.AlignHCenter
                            text:               _summaryRoot._remainingCount > 0 ?
                                                    qsTr("%n component(s) need setup", "", _summaryRoot._remainingCount) :
                                                    qsTr("Required setup complete")
                            textColor:          _summaryRoot._remainingCount > 0 ? qgcPal.errorText : qgcPal.successText
                        }

                        QGCButton {
                            objectName:         "vehicleSummary_continueSetup"
                            Layout.alignment:   Qt.AlignHCenter
                            primary:            true
                            text:               _summaryRoot._nextComponent ? qsTr("Continue setup: %1").arg(_summaryRoot._nextComponent.name) : ""
                            visible:            _summaryRoot._nextComponent !== null
                            onClicked:          _summaryRoot.componentRequested(_summaryRoot._nextComponent)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true
                    implicitHeight:     checklistLayout.implicitHeight + _summaryRoot._spacing * 2
                    color:              qgcPal.windowPanel
                    border.color:       qgcPal.borderSubtle
                    radius:             ScreenTools.defaultBorderRadius * 2

                    ColumnLayout {
                        id:                 checklistLayout
                        anchors.fill:       parent
                        anchors.margins:    _summaryRoot._spacing
                        spacing:            _summaryRoot._spacing

                        VehicleSummaryHeader {
                            text: qsTr("Required setup")
                        }

                        GridLayout {
                            Layout.fillWidth:   true
                            columns:            Math.max(1, Math.floor(_summaryRoot._checklistWidth / (ScreenTools.defaultFontPixelWidth * 28)))
                            columnSpacing:      ScreenTools.defaultFontPixelWidth
                            rowSpacing:         ScreenTools.defaultFontPixelWidth

                            Repeater {
                                model: _summaryRoot._requiredComponents

                                VehicleSummaryTile {
                                    objectName: "vehicleSummary_tile_" + modelData.name
                                    text:       modelData.name
                                    done:       modelData.setupComplete
                                    next:       modelData === _summaryRoot._nextComponent
                                    clickable:  modelData.setupSource.toString() !== ""
                                    onClicked:  _summaryRoot.componentRequested(modelData)

                                    required property var modelData
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            Layout.fillHeight:  true
                            wrapMode:           Text.WordWrap
                            font.pointSize:     ScreenTools.smallFontPointSize
                            color:              qgcPal.textSecondary
                            text:               qsTr("Select a component to open its setup page. The other components are listed below.")
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     settingsLayout.implicitHeight + _summaryRoot._spacing * 2
                color:              qgcPal.windowPanel
                border.color:       qgcPal.borderSubtle
                radius:             ScreenTools.defaultBorderRadius * 2

                ColumnLayout {
                    id:                 settingsLayout
                    anchors.fill:       parent
                    anchors.margins:    _summaryRoot._spacing
                    spacing:            _summaryRoot._spacing

                    VehicleSummaryHeader {
                        text: qsTr("Current settings")
                    }

                    VehicleSummaryMasonry {
                        Layout.fillWidth:   true
                        minColumnWidth:     _summaryRoot._minCardWidth
                        spacing:            _summaryRoot._spacing
                        model:              _summaryRoot._components

                        delegate: VehicleSummaryCard {
                            vehicleComponent:   modelData
                            visible:            hasContent
                            onClicked:          _summaryRoot.componentRequested(modelData)

                            required property var modelData
                        }
                    }
                }
            }
        }
    }
}
