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
    height: _colapsed ? colapsedRect.height : expandedRect.height

    property bool   showColapse:        true

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property color  _airspaceColor:     _validAdvisories ? getAispaceColor(_activeVehicle.airspaceController.advisories.airspaceColor) : _colorGray
    property bool   _validRules:        _activeVehicle ? _activeVehicle.airspaceController.rulesets.valid : false
    property bool   _validAdvisories:   _activeVehicle ? _activeVehicle.airspaceController.advisories.valid : false
    property color  _textColor:         qgcPal.text
    property bool   _colapsed:          _activeVehicle ? !_activeVehicle.airspaceController.airspaceVisible : true

    readonly property real      _radius:            ScreenTools.defaultFontPixelWidth * 0.5
    readonly property color     _colorOrange:       "#d75e0d"
    readonly property color     _colorBrown:        "#3c2b24"
    readonly property color     _colorLightBrown:   "#5a4e49"
    readonly property color     _colorGray:         "#615c61"
    readonly property color     _colorLightGray:    "#a0a0a0"
    readonly property color     _colorMidBrown:     "#3a322f"
    readonly property color     _colorYellow:       "#d7c61d"
    readonly property color     _colorWhite:        "#ffffff"
    readonly property color     _colorRed:          "#aa1200"
    readonly property color     _colorGreen:        "#125f00"

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    function getAispaceColor(color) {
        if(color === AirspaceAdvisoryProvider.Green)  return _colorGreen;
        if(color === AirspaceAdvisoryProvider.Yellow) return _colorYellow;
        if(color === AirspaceAdvisoryProvider.Orange) return _colorOrange;
        if(color === AirspaceAdvisoryProvider.Red)    return _colorRed;
        return _colorGray;
    }

    on_AirspaceColorChanged: {
       if(_validAdvisories) {
           if(_activeVehicle.airspaceController.advisories.airspaceColor === AirspaceAdvisoryProvider.Yellow) {
               _textColor = "#000000"
               return
           }
       }
       _textColor = _colorWhite
    }

    //---------------------------------------------------------------
    //-- Colapsed State
    Rectangle {
        id:         colapsedRect
        width:      parent.width
        height:     _colapsed ? colapsedRow.height + ScreenTools.defaultFontPixelHeight : 0
        color:      _validAdvisories ? getAispaceColor(_activeVehicle.airspaceController.advisories.airspaceColor) : _colorGray
        radius:     _radius
        visible:    _colapsed
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
                source:                 _activeVehicle ? "qrc:/airmap/advisory-icon.svg" : "qrc:/airmap/unavailable.svg"
                color:                  _activeVehicle ? _textColor : _colorLightGray
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:                   qsTr("Airspace")
                color:                  _activeVehicle ? _textColor : _colorLightGray
                anchors.verticalCenter: parent.verticalCenter
            }
            Item {
                width:  ScreenTools.defaultFontPixelWidth
                height: 1
            }
            AirspaceWeather {
                iconHeight:             ScreenTools.defaultFontPixelWidth * 2.5
                visible:                _activeVehicle && _activeVehicle.airspaceController.weatherInfo.valid
                contentColor:           _activeVehicle ? _textColor : _colorLightGray
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCColoredImage {
            width:                  height
            height:                 ScreenTools.defaultFontPixelWidth * 2.5
            sourceSize.height:      height
            source:                 "qrc:/airmap/expand.svg"
            color:                  _textColor
            visible:                _activeVehicle
            anchors.right:          parent.right
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
        }
        MouseArea {
            anchors.fill:   parent
            enabled:        _activeVehicle
            onClicked:  {
                _activeVehicle.airspaceController.airspaceVisible = true
            }
        }
    }
    //---------------------------------------------------------------
    //-- Expanded State
    Rectangle {
        id:         expandedRect
        width:      parent.width
        height:     !_colapsed ? expandedCol.height + ScreenTools.defaultFontPixelHeight : 0
        color:      _validAdvisories ? getAispaceColor(_activeVehicle.airspaceController.advisories.airspaceColor) : _colorGray
        radius:     _radius
        visible:    !_colapsed
        MouseArea {
            anchors.fill:   parent
            onWheel:        { wheel.accepted = true; }
            onPressed:      { mouse.accepted = true; }
            onReleased:     { mouse.accepted = true; }
        }
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
                        color:                  _textColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Column {
                        spacing:                0
                        anchors.verticalCenter: parent.verticalCenter
                        QGCLabel {
                            text:               qsTr("Airspace")
                            color:              _textColor
                        }
                        QGCLabel {
                            text:               _validAdvisories ? _activeVehicle.airspaceController.advisories.airspaces.count + qsTr(" Advisories") : ""
                            color:              _textColor
                            visible:            _validAdvisories
                            font.pointSize:     ScreenTools.smallFontPointSize
                        }
                    }
                    Item {
                        width:  ScreenTools.defaultFontPixelWidth
                        height: 1
                    }
                    AirspaceWeather {
                        visible:                _activeVehicle && _activeVehicle.airspaceController.weatherInfo.valid && showColapse
                        contentColor:           _textColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                QGCColoredImage {
                    width:                  height
                    height:                 ScreenTools.defaultFontPixelWidth * 2.5
                    sourceSize.height:      height
                    source:                 "qrc:/airmap/colapse.svg"
                    color:                  _textColor
                    visible:                showColapse
                    anchors.right:          parent.right
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        showColapse && _activeVehicle
                        onClicked:      _activeVehicle.airspaceController.airspaceVisible = false
                    }
                }
                AirspaceWeather {
                    visible:                _activeVehicle && _activeVehicle.airspaceController.weatherInfo.valid && !showColapse
                    contentColor:           _textColor
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
                                        id:                 pencilIcon
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
                                    height:                 ScreenTools.defaultFontPixelHeight * 1.5
                                    radius:                 2
                                    color:                  _colorMidBrown
                                    Layout.fillWidth:       true
                                    QGCLabel {
                                        id:                     regLabel
                                        text:                   _validRules ? _activeVehicle.airspaceController.ruleSets.selectedRuleSets : qsTr("None")
                                        elide:                  Text.ElideRight
                                        horizontalAlignment:    Text.AlignHCenter
                                        color:                  _colorWhite
                                        anchors.left:           parent.left
                                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth  * 0.5
                                        anchors.right:          parent.right
                                        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth  * 0.5
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                    Flickable {
                        clip:               true
                        height:             ScreenTools.defaultFontPixelHeight * 8
                        contentHeight:      advisoryCol.height
                        flickableDirection: Flickable.VerticalFlick
                        anchors.left:       parent.left
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.right:      parent.right
                        anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.5
                        Column {
                            id:             advisoryCol
                            spacing:        ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.right:  parent.right
                            anchors.left:   parent.left
                            Repeater {
                                model:      _validAdvisories ? _activeVehicle.airspaceController.advisories.airspaces : []
                                delegate: AirspaceRegulation {
                                    regTitle:            object.typeStr
                                    regText:             object.name
                                    regColor:            getAispaceColor(object.color)
                                    anchors.right:       parent.right
                                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                                    anchors.left:        parent.left
                                    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                                }
                            }
                        }
                    }
                }
            }
            //-- Footer
            QGCLabel {
                text:           qsTr("Powered by <b>AIRMAP</b>")
                color:          _textColor
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
                onWheel:   { wheel.accepted = true; }
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
                x:                  0
                y:                  0
                color:              qgcPal.window
                width:              rulesCol.width  + ScreenTools.defaultFontPixelWidth
                height:             rulesCol.height + ScreenTools.defaultFontPixelHeight
                radius:             ScreenTools.defaultFontPixelWidth
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
                            spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                            anchors.right: parent.right
                            anchors.left:  parent.left
                            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 2
                            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 2
                            Item {
                                width:  1
                                height: 1
                            }
                            ExclusiveGroup { id: rulesGroup }
                            QGCLabel {
                                text:           qsTr("PICK ONE REGULATION")
                                font.pointSize: ScreenTools.smallFontPointSize
                                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                            }
                            Repeater {
                                model:    _validRules ? _activeVehicle.airspaceController.rulesets.ruleSets : []
                                delegate: RuleSelector {
                                    visible:             object.selectionType === AirspaceRuleSet.Pickone
                                    rule:                object
                                    exclusiveGroup:      rulesGroup
                                    anchors.right:       parent.right
                                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                                    anchors.left:        parent.left
                                    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                                }
                            }
                            Item {
                                width:  1
                                height: 1
                            }
                            QGCLabel {
                                text:           qsTr("OPTIONAL")
                                font.pointSize: ScreenTools.smallFontPointSize
                                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                            }
                            Repeater {
                                model:    _validRules ? _activeVehicle.airspaceController.rulesets.ruleSets : []
                                delegate: RuleSelector {
                                    visible:             object.selectionType === AirspaceRuleSet.Optional
                                    rule:                object
                                    anchors.right:       parent.right
                                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                                    anchors.left:        parent.left
                                    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                                }
                            }
                            Item {
                                width:  1
                                height: 1
                            }
                            QGCLabel {
                                text:           qsTr("REQUIRED")
                                font.pointSize: ScreenTools.smallFontPointSize
                                anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                            }
                            Repeater {
                                model:    _validRules ? _activeVehicle.airspaceController.rulesets.ruleSets : []
                                delegate: RuleSelector {
                                    visible:             object.selectionType === AirspaceRuleSet.Required
                                    rule:                object
                                    required:            true
                                    anchors.right:       parent.right
                                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                                    anchors.left:        parent.left
                                    anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                                }
                            }
                        }
                    }
                }
            }
            //-- Arrow
            QGCColoredImage {
                id:                 arrowIconShadow
                anchors.fill:       arrowIcon
                sourceSize.height:  height
                source:             "qrc:/airmap/right-arrow.svg"
                color:              qgcPal.window
                visible:            false
            }
            DropShadow {
                anchors.fill:       arrowIconShadow
                visible:            ruleSelectorRect.visible && qgcPal.globalTheme === QGCPalette.Dark
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             arrowIconShadow
            }
            QGCColoredImage {
                id:                 arrowIcon
                width:              height
                height:             ScreenTools.defaultFontPixelHeight * 2
                sourceSize.height:  height
                source:             "qrc:/airmap/right-arrow.svg"
                color:              ruleSelectorRect.color
                anchors.left:       ruleSelectorRect.right
                anchors.top:        ruleSelectorRect.top
                anchors.topMargin:  (ScreenTools.defaultFontPixelHeight * 4) - (height * 0.5) + (pencilIcon.height * 0.5)
            }
            Component.onCompleted: {
                mainWindow.disableToolbar()
                var target = mainWindow.mapFromItem(pencilIcon, 0, 0)
                ruleSelectorRect.x = target.x - ruleSelectorRect.width - (ScreenTools.defaultFontPixelWidth * 7)
                ruleSelectorRect.y = target.y - (ScreenTools.defaultFontPixelHeight * 4)
            }
        }
    }
}
