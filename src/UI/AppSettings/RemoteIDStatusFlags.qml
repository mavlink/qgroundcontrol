import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    Layout.fillWidth: true
    implicitHeight:   flagsColumn.height
    visible:          _activeVehicle !== null

    property var  _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property var  _remoteIDManager:  _activeVehicle ? _activeVehicle.remoteIDManager : null
    property bool _commsGood:        _remoteIDManager ? _remoteIDManager.commsGood : false
    property real _flagWidth:        ScreenTools.defaultFontPixelWidth * 15
    property real _flagHeight:       ScreenTools.defaultFontPixelWidth * 7
    property int  _flagRadius:       5

    QGCPalette { id: qgcPal }

    ColumnLayout {
        id:                         flagsColumn
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    ScreenTools.defaultFontPixelWidth

        Rectangle {
            Layout.preferredHeight: statusGrid.height + (ScreenTools.defaultFontPixelWidth * 2)
            Layout.preferredWidth:  statusGrid.width + (ScreenTools.defaultFontPixelWidth * 2)
            Layout.fillWidth:       true
            color:                  qgcPal.windowShade

            GridLayout {
                id:                         statusGrid
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.top:                parent.top
                anchors.horizontalCenter:   parent.horizontalCenter
                rows:                       1
                rowSpacing:                 ScreenTools.defaultFontPixelWidth * 3
                columnSpacing:              ScreenTools.defaultFontPixelWidth * 2

                Rectangle {
                    Layout.preferredHeight: _flagHeight
                    Layout.preferredWidth:  _flagWidth
                    color:                  _remoteIDManager ? (_remoteIDManager.armStatusGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                    radius:                 _flagRadius
                    visible:                _commsGood

                    QGCLabel {
                        anchors.fill:           parent
                        text:                   qsTr("ARM STATUS")
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.bold:              true
                    }
                }

                Rectangle {
                    Layout.preferredHeight: _flagHeight
                    Layout.preferredWidth:  _flagWidth
                    color:                  _remoteIDManager ? (_remoteIDManager.commsGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                    radius:                 _flagRadius

                    QGCLabel {
                        anchors.fill:           parent
                        text:                   _remoteIDManager && _remoteIDManager.commsGood ? qsTr("RID COMMS") : qsTr("NOT CONNECTED")
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.bold:              true
                    }
                }

                Rectangle {
                    Layout.preferredHeight: _flagHeight
                    Layout.preferredWidth:  _flagWidth
                    color:                  _remoteIDManager ? (_remoteIDManager.gcsGPSGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                    radius:                 _flagRadius
                    visible:                _commsGood

                    QGCLabel {
                        anchors.fill:           parent
                        text:                   qsTr("GCS GPS")
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.bold:              true
                    }
                }

                Rectangle {
                    Layout.preferredHeight: _flagHeight
                    Layout.preferredWidth:  _flagWidth
                    color:                  _remoteIDManager ? (_remoteIDManager.basicIDGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                    radius:                 _flagRadius
                    visible:                _commsGood

                    QGCLabel {
                        anchors.fill:           parent
                        text:                   qsTr("BASIC ID")
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.bold:              true
                    }
                }

                Rectangle {
                    Layout.preferredHeight: _flagHeight
                    Layout.preferredWidth:  _flagWidth
                    color:                  _remoteIDManager ? (_remoteIDManager.operatorIDGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                    radius:                 _flagRadius
                    visible:                _commsGood && (_remoteIDManager ? (QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value
                                            || QGroundControl.settingsManager.remoteIDSettings.region.rawValue === RemoteIDSettings.RegionOperation.EU) : false)

                    QGCLabel {
                        anchors.fill:           parent
                        text:                   qsTr("OPERATOR ID")
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.bold:              true
                    }
                }
            }
        }
    }
}
