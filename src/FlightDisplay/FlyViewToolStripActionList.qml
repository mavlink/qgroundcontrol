/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQml.Models
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Palette
import QGroundControl.ScreenTools

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist

    property var flightModeDisplay: FlightModeDisplay { }

    model: [
        ToolStripAction {
            property bool _is3DViewOpen:            viewer3DWindow.isOpen
            property bool   _viewer3DEnabled:       QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue

            id: view3DIcon
            property bool nonOperationAction: true
            visible: _viewer3DEnabled
            text:           qsTr("3D View")
            iconSource:     "/res/flyview-3d.svg"
            onTriggered:{
                if(_is3DViewOpen === false){
                    viewer3DWindow.open()
                }else{
                    viewer3DWindow.close()
                }
            }

            on_Is3DViewOpenChanged: {
                if(_is3DViewOpen === true){
                    view3DIcon.iconSource =     "/res/flyview-fly.svg"
                    text=           qsTr("Fly")
                }else{
                    iconSource =     "/res/flyview-3d.svg"
                    text =           qsTr("3D View")
                }
            }
        },
        ToolStripAction {
            property bool nonOperationAction: true

            text:       qsTr("Plan")
            iconSource: "/res/flyview-plan.svg"

            onTriggered: {
                mainWindow.showPlanView()
            }
        },
        PreFlightCheckListShowAction {
            property bool nonOperationAction: true
            onTriggered: displayPreFlightChecklist()
        },
        ToolStripAction {
            id: modeAction

            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
            property string _currentModeDisplayText: _activeVehicle ? flightModeDisplay.shortModeText(_activeVehicle, _activeVehicle.flightMode, qsTr("Mode")) : qsTr("Mode")
            property string cornerBadgeText: flightModeDisplay.badgeText(_currentModeDisplayText)
            property bool statusAction: true

            text:       flightModeDisplay.labelText(_currentModeDisplayText)
            iconSource: "/res/flyview-mode.svg"
            visible:    _activeVehicle && _activeVehicle.flightModeSetAvailable
            enabled:    visible

            dropPanelComponent: Component {
                Rectangle {
                    id:     flightModePanel
                    width:  Math.max(ScreenTools.defaultFontPixelWidth * 15.4, ScreenTools.minTouchPixels * 2.55)
                    height: Math.min(_maxListHeight, _contentHeight)
                    color:  "transparent"
                    clip:   true

                    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
                    property real _panelMargin: ScreenTools.defaultFontPixelWidth * 0.46
                    property real _rowHeight: ScreenTools.defaultFontPixelHeight * 1.62
                    property real _rowSpacing: ScreenTools.defaultFontPixelHeight * 0.30
                    property int  _maxVisibleRows: 7
                    property var  _modeList: flightModeDisplay.sortedModes(_activeVehicle)
                    property int  _modeCount: _modeList.length
                    property real _contentHeight: modeColumn.implicitHeight + (_panelMargin * 2)
                    property real _maxListHeight: (_panelMargin * 2) +
                                                  (_rowHeight * Math.min(_modeCount, _maxVisibleRows)) +
                                                  (_rowSpacing * Math.max(0, Math.min(_modeCount, _maxVisibleRows) - 1))

                    QGCPalette { id: qgcPal }

                    QGCFlickable {
                        anchors.fill:           parent
                        anchors.margins:        flightModePanel._panelMargin
                        contentWidth:           width
                        contentHeight:          modeColumn.implicitHeight
                        flickableDirection:     Flickable.VerticalFlick
                        boundsBehavior:         Flickable.StopAtBounds
                        interactive:            contentHeight > height
                        clip:                   true
                        indicatorColor:         qgcPal.buttonText

                        Column {
                            id:                 modeColumn
                            width:              parent.width
                            spacing:            flightModePanel._rowSpacing

                            Repeater {
                                model: flightModePanel._modeList

                                Rectangle {
                                    id: modeRow

                                    property string modeDisplayText: flightModeDisplay.modeText(flightModePanel._activeVehicle, modelData, qsTr("Mode"))

                                    width:      modeColumn.width
                                    height:     flightModePanel._rowHeight
                                    radius:     Math.round(ScreenTools.defaultFontPixelWidth * 0.30)
                                    color:      modeMouse.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.48) :
                                                (modeMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.060) : Qt.rgba(0.070, 0.076, 0.084, 0.66))
                                    border.color: flightModePanel._activeVehicle && flightModePanel._activeVehicle.flightMode === modelData ?
                                                      qgcPal.primaryButton :
                                                      (modeMouse.containsMouse ? Qt.rgba(0.82, 0.90, 0.95, 0.26) : Qt.rgba(0.82, 0.90, 0.95, 0.18))
                                    border.width: 1

                                    Item {
                                        id:                 modeTextHost
                                        anchors.fill:           parent
                                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 0.72
                                        anchors.rightMargin:    anchors.leftMargin
                                        anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.08
                                        anchors.bottomMargin:   anchors.topMargin

                                        RowLayout {
                                            id:                 modeTextRow
                                            anchors.centerIn:   parent
                                            spacing:            modeTypeBadge.visible ? ScreenTools.defaultFontPixelWidth * 0.28 : 0

                                            QGCLabel {
                                                id:                     modeTextLabel
                                                Layout.preferredWidth:  Math.max(0, Math.min(implicitWidth,
                                                                                             modeTextHost.width -
                                                                                             (modeTypeBadge.visible ? modeTypeBadge.width + modeTextRow.spacing : 0)))
                                                Layout.maximumWidth:    Layout.preferredWidth
                                                text:                   flightModeDisplay.labelText(modeRow.modeDisplayText)
                                                color:                  qgcPal.text
                                                font.bold:              flightModePanel._activeVehicle && flightModePanel._activeVehicle.flightMode === modelData
                                                font.pointSize:         ScreenTools.controlFontPointSize
                                                horizontalAlignment:    Text.AlignHCenter
                                                verticalAlignment:      Text.AlignVCenter
                                                elide:                  Text.ElideRight
                                                maximumLineCount:       1
                                            }

                                            Rectangle {
                                                id:                 modeTypeBadge
                                                Layout.alignment:   Qt.AlignVCenter
                                                Layout.preferredWidth: modeTypeBadgeText.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.46
                                                Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 0.70,
                                                                                 modeTypeBadgeText.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.06)
                                                radius:             Math.round(height * 0.28)
                                                color:              Qt.rgba(0.82, 0.88, 0.94, 0.10)
                                                border.color:       Qt.rgba(0.82, 0.88, 0.94, 0.18)
                                                border.width:       1
                                                visible:            modeTypeBadgeText.text !== ""

                                                QGCLabel {
                                                    id:                     modeTypeBadgeText
                                                    anchors.centerIn:       parent
                                                    text:                   flightModeDisplay.badgeText(modeRow.modeDisplayText)
                                                    color:                  qgcPal.buttonText
                                                    font.bold:              true
                                                    font.pointSize:         Math.max(7, ScreenTools.captionFontPointSize - 1)
                                                    horizontalAlignment:    Text.AlignHCenter
                                                    verticalAlignment:      Text.AlignVCenter
                                                    maximumLineCount:       1
                                                }
                                            }
                                        }
                                    }

                                    QGCMouseArea {
                                        id:             modeMouse
                                        anchors.fill:   parent
                                        hoverEnabled:   !ScreenTools.isMobile
                                        enabled:        flightModePanel._activeVehicle && flightModePanel._activeVehicle.flightModeSetAvailable

                                        onClicked: {
                                            if (flightModePanel._activeVehicle) {
                                                flightModePanel._activeVehicle.flightMode = modelData
                                            }
                                            dropPanel.hide()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionPause { },
        GuidedActionContinueMission { },
        GuidedActionRTL { },
        FlyViewAdditionalActionsButton { },
        GuidedActionGripper { }
    ]
}
