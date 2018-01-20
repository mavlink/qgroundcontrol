import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtQml                    2.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Airmap        1.0

Item {
    id:     _root
    width:  parent.width
    height: colapsed ? colapsedRect.height : expandedRect.height

    property var    colapsed:           true

    readonly property real      _radius:            ScreenTools.defaultFontPixelWidth * 0.5
    readonly property color     _colorOrange:       "#d75e0d"
    readonly property color     _colorBrown:        "#3c2b24"
    readonly property color     _colorLightBrown:   "#5a4e49"
    readonly property color     _colorGray:         "#615c61"
    readonly property color     _colorMidBrown:     "#3a322f"
    readonly property color     _colorYellow:       "#d7c61d"
    readonly property color     _colorWhite:        "#ffffff"

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    //---------------------------------------------------------------
    //-- Colapsed State
    Rectangle {
        id:         colapsedRect
        width:      parent.width
        height:     colapsed ? colapsedRow.height + ScreenTools.defaultFontPixelHeight : 0
        color:      _colorOrange
        radius:     _radius
        visible:    colapsed
        Row {
            id:                     colapsedRow
            spacing:                ScreenTools.defaultFontPixelWidth
            anchors.left:           parent.left
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
            QGCColoredImage {
                width:                  height
                height:                 ScreenTools.defaultFontPixelWidth * 2.5
                sourceSize.height:      height
                source:                 "qrc:/airmap/advisory-icon.svg"
                color:                  _colorWhite
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:                   qsTr("Airspace")
                color:                  _colorWhite
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCColoredImage {
            width:                  height
            height:                 ScreenTools.defaultFontPixelWidth * 2.5
            sourceSize.height:      height
            source:                 "qrc:/airmap/expand.svg"
            color:                  _colorWhite
            anchors.right:          parent.right
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
        }
        MouseArea {
            anchors.fill:   parent
            onClicked:      colapsed = false
        }
    }
    //---------------------------------------------------------------
    //-- Expanded State
    Rectangle {
        id:         expandedRect
        width:      parent.width
        height:     !colapsed ? expandedCol.height + ScreenTools.defaultFontPixelHeight : 0
        color:      _colorOrange
        radius:     _radius
        visible:    !colapsed
        Column {
            id:                     expandedCol
            spacing:                ScreenTools.defaultFontPixelHeight * 0.5
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            //-- Header
            Item {
                height:             expandedRow.height
                anchors.left:       parent.left
                anchors.right:      parent.right
                Row {
                    id:                         expandedRow
                    spacing:                    ScreenTools.defaultFontPixelWidth
                    anchors.left:               parent.left
                    anchors.leftMargin:         ScreenTools.defaultFontPixelWidth
                    QGCColoredImage {
                        width:                  height
                        height:                 ScreenTools.defaultFontPixelWidth * 2.5
                        sourceSize.height:      height
                        source:                 "qrc:/airmap/advisory-icon.svg"
                        color:                  _colorWhite
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Column {
                        spacing:                0
                        anchors.verticalCenter: parent.verticalCenter
                        QGCLabel {
                            text:               qsTr("Airspace")
                            color:              _colorWhite
                        }
                        QGCLabel {
                            text:               qsTr("3 Advisories")
                            color:              _colorWhite
                            font.pointSize:     ScreenTools.smallFontPointSize
                        }
                    }
                }
            }
            //-- Contents (Brown Box)
            Rectangle {
                color:                      _colorBrown
                height:                     airspaceCol.height + ScreenTools.defaultFontPixelHeight
                radius:                     _radius
                anchors.left:               parent.left
                anchors.leftMargin:         ScreenTools.defaultFontPixelWidth * 0.5
                anchors.right:              parent.right
                anchors.rightMargin:        ScreenTools.defaultFontPixelWidth * 0.5
                Column {
                    id:                     airspaceCol
                    spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.left:           parent.left
                    anchors.leftMargin:     ScreenTools.defaultFontPixelWidth  * 0.5
                    anchors.right:          parent.right
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth  * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    //-- Regulations
                    Rectangle {
                        color:                          _colorLightBrown
                        height:                         regCol.height + ScreenTools.defaultFontPixelHeight
                        radius:                         _radius
                        anchors.left:                   parent.left
                        anchors.leftMargin:             ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.right:                  parent.right
                        anchors.rightMargin:            ScreenTools.defaultFontPixelWidth * 0.5
                        Column {
                            id:                         regCol
                            spacing:                    ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.left:               parent.left
                            anchors.leftMargin:         ScreenTools.defaultFontPixelWidth  * 0.5
                            anchors.right:              parent.right
                            anchors.rightMargin:        ScreenTools.defaultFontPixelWidth  * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            QGCLabel {
                                text:                   qsTr("Airspace Regulations")
                                color:                  _colorWhite
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            QGCLabel {
                                text:                   qsTr("Airspace advisories based on the selected rules.")
                                color:                  _colorWhite
                                anchors.left:           parent.left
                                anchors.right:          parent.right
                                wrapMode:               Text.WordWrap
                                font.pointSize:         ScreenTools.smallFontPointSize
                            }
                            GridLayout {
                                columns:                2
                                anchors.left:           parent.left
                                anchors.right:          parent.right
                                Rectangle {
                                    width:                  regButton.height
                                    height:                 width
                                    radius:                 2
                                    color:                  _colorGray
                                    anchors.verticalCenter: parent.verticalCenter
                                    QGCColoredImage {
                                        width:              height
                                        height:             parent.height * 0.5
                                        sourceSize.height:  height
                                        source:             "qrc:/airmap/pencil.svg"
                                        color:              _colorWhite
                                        anchors.centerIn:   parent
                                    }
                                }
                                Rectangle {
                                    id:                     regButton
                                    height:                 regLabel.height + ScreenTools.defaultFontPixelHeight * 0.5
                                    radius:                 2
                                    color:                  _colorMidBrown
                                    Layout.fillWidth:       true
                                    QGCLabel {
                                        id:                 regLabel
                                        text:               qsTr("FAA-107, Airmap")
                                        color:              _colorWhite
                                        anchors.centerIn:   parent
                                    }
                                }
                            }
                        }
                    }
                    AirspaceRegulation {
                        regTitle:                   qsTr("Controlled Aispace (1)")
                        regText:                    qsTr("Santa Monica Class D requires FAA Authorization, permissible below 100ft.")
                        regColor:                   _colorOrange
                        textColor:                  _colorWhite
                        anchors.left:               parent.left
                        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.right:              parent.right
                        anchors.rightMargin:        ScreenTools.defaultFontPixelWidth * 0.5
                    }
                    AirspaceRegulation {
                        regTitle:                   qsTr("Schools (2)")
                        regText:                    qsTr("Santa Monica School of Something.")
                        regColor:                   _colorYellow
                        textColor:                  _colorWhite
                        anchors.left:               parent.left
                        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.right:              parent.right
                        anchors.rightMargin:        ScreenTools.defaultFontPixelWidth * 0.5
                    }
                }
            }
            //-- Footer
            QGCLabel {
                text:           qsTr("Powered by <b>AIRMAP</b>")
                color:          _colorWhite
                font.pointSize: ScreenTools.smallFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
