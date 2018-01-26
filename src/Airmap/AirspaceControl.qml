import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtQml                    2.2
import QtGraphicalEffects       1.0

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

    property bool   colapsed:           true
    property bool   showColapse:        true

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   validRules:         _activeVehicle ? _activeVehicle.airspaceController.rulesets.valid : false

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
            Item {
                width:  ScreenTools.defaultFontPixelWidth
                height: 1
            }
            AirspaceWeather {
                iconHeight:             ScreenTools.defaultFontPixelWidth * 2.5
                visible:                _activeVehicle && _activeVehicle.airspaceController.weatherInfo.valid
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
                    Item {
                        width:  ScreenTools.defaultFontPixelWidth
                        height: 1
                    }
                    AirspaceWeather {
                        visible:                _activeVehicle && _activeVehicle.airspaceController.weatherInfo.valid && showColapse
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                QGCColoredImage {
                    width:                  height
                    height:                 ScreenTools.defaultFontPixelWidth * 2.5
                    sourceSize.height:      height
                    source:                 "qrc:/airmap/colapse.svg"
                    color:                  _colorWhite
                    visible:                showColapse
                    anchors.right:          parent.right
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        showColapse
                        onClicked:      colapsed = true
                    }
                }
                AirspaceWeather {
                    visible:                _activeVehicle && _activeVehicle.airspaceController.weatherInfo.valid && !showColapse
                    anchors.right:          parent.right
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.verticalCenter: parent.verticalCenter
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
                                        MouseArea {
                                            anchors.fill:   parent
                                            onClicked: {
                                                rootLoader.sourceComponent = ruleSelector
                                                mainWindow.disableToolbar()
                                            }
                                        }
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
                                        text:               validRules ? _activeVehicle.airspaceController.rulesets.rules.get(currentIndex).name : qsTr("None")
                                        color:              _colorWhite
                                        anchors.centerIn:   parent
                                        property int    currentIndex:   validRules ? _activeVehicle.airspaceController.rulesets.currentIndex : 0
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
    //---------------------------------------------------------------
    //-- Rule Selector
    Component {
        id:             ruleSelector
        Rectangle {
            width:      mainWindow.width
            height:     mainWindow.height
            color:      Qt.rgba(0,0,0,0.1)
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    mainWindow.enableToolbar()
                    rootLoader.sourceComponent = null
                }
            }
            Rectangle {
                id:             ruleSelectorShadow
                anchors.fill:   ruleSelectorRect
                radius:         ruleSelectorRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       ruleSelectorShadow
                visible:            ruleSelectorRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             ruleSelectorShadow
            }
            Rectangle {
                id:                 ruleSelectorRect
                color:              qgcPal.window
                width:              rulesCol.width  + ScreenTools.defaultFontPixelWidth
                height:             rulesCol.height + ScreenTools.defaultFontPixelHeight
                radius:             ScreenTools.defaultFontPixelWidth
                anchors.centerIn:   parent
                Column {
                    id:             rulesCol
                    spacing:        ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.centerIn: parent
                    //-- Regulations
                    Rectangle {
                        color:      qgcPal.windowShade
                        height:     rulesTitle.height + ScreenTools.defaultFontPixelHeight
                        width:      parent.width * 0.95
                        radius:     _radius
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            id:         rulesTitle
                            text:       qsTr("Airspace Regulation Options")
                            anchors.centerIn: parent
                        }
                    }
                    Flickable {
                        clip:           true
                        width:          ScreenTools.defaultFontPixelWidth  * 30
                        height:         ScreenTools.defaultFontPixelHeight * 14
                        contentHeight:  rulesetCol.height
                        flickableDirection: Flickable.VerticalFlick
                        Column {
                            id:         rulesetCol
                            spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.right: parent.right
                            anchors.left:  parent.left
                            QGCLabel {
                                text:      qsTr("PICK ONE REGULATION")
                                font.pointSize: ScreenTools.smallFontPointSize
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Repeater {
                                model:      validRules ? _activeVehicle.airspaceController.rulesets.rules : []
                                delegate: Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    anchors.right: parent.right
                                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                                    anchors.left:  parent.left
                                    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                                    Rectangle {
                                        width:      ScreenTools.defaultFontPixelWidth * 0.75
                                        height:     ScreenTools.defaultFontPixelHeight
                                        color:      qgcPal.colorGreen
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    QGCLabel {
                                        text:  object.name
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                mainWindow.disableToolbar()
            }
        }
    }
}
