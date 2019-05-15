import QtQuick                  2.11
import QtQuick.Controls         2.4
import QtQuick.Layouts          1.11
import QtQuick.Dialogs          1.3
import QtQuick.Controls.Styles  1.4
import QtQml                    2.2
import QtGraphicalEffects       1.0

import QGroundControl               1.0
import QGroundControl.Airmap        1.0
import QGroundControl.Airspace      1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

Item {
    id:     _root
    width:  parent.width
    height: _colapsed ? colapsedRect.height : expandedRect.height

    property bool   showColapse:        true
    property bool   planView:           false

    property color  _airspaceColor:     _validAdvisories ? getAispaceColor(QGroundControl.airspaceManager.advisories.airspaceColor) : _colorGray
    property bool   _validRules:        QGroundControl.airspaceManager.connected && QGroundControl.airspaceManager.ruleSets.valid
    property bool   _validAdvisories:   QGroundControl.airspaceManager.connected && QGroundControl.airspaceManager.advisories.valid
    property color  _textColor:         qgcPal.text
    property bool   _colapsed:          !QGroundControl.airspaceManager.airspaceVisible || !QGroundControl.airspaceManager.connected
    property int    _flightPermit:      QGroundControl.airspaceManager.flightPlan.flightPermitStatus

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
    readonly property real      _baseHeight:        ScreenTools.defaultFontPixelHeight * 22
    readonly property real      _baseWidth:         ScreenTools.defaultFontPixelWidth  * 40

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

    function hasBriefRules() {
        if(QGroundControl.airspaceManager.flightPlan.rulesViolation.count > 0)
            return true;
        if(QGroundControl.airspaceManager.flightPlan.rulesInfo.count > 0)
            return true;
        if(QGroundControl.airspaceManager.flightPlan.rulesReview.count > 0)
            return true;
        if(QGroundControl.airspaceManager.flightPlan.rulesFollowing.count > 0)
            return true;
        return false;
    }

    on_AirspaceColorChanged: {
       if(_validAdvisories) {
           if(QGroundControl.airspaceManager.advisories.airspaceColor === AirspaceAdvisoryProvider.Yellow) {
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
        color:      QGroundControl.airspaceManager.connected ? (_validAdvisories ? getAispaceColor(QGroundControl.airspaceManager.advisories.airspaceColor) : _colorGray) : _colorGray
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
                    text:               _validAdvisories ? QGroundControl.airspaceManager.advisories.advisories.count + qsTr(" Advisories") : ""
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
                iconHeight:             ScreenTools.defaultFontPixelHeight * 2
                visible:                QGroundControl.airspaceManager.weatherInfo.valid && QGroundControl.airspaceManager.connected
                contentColor:           _textColor
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCLabel {
                text:                   qsTr("Not Connected")
                color:                  qgcPal.text
                visible:                !QGroundControl.airspaceManager.connected
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCColoredImage {
            width:                  height
            height:                 ScreenTools.defaultFontPixelWidth * 2.5
            sourceSize.height:      height
            source:                 "qrc:/airmap/expand.svg"
            color:                  _textColor
            fillMode:               Image.PreserveAspectFit
            visible:                QGroundControl.airspaceManager.connected
            anchors.right:          parent.right
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
        }
        MouseArea {
            anchors.fill:           parent
            enabled:                QGroundControl.airspaceManager.connected
            onClicked:  {
                QGroundControl.airspaceManager.airspaceVisible = true
            }
        }
    }
    //---------------------------------------------------------------
    //-- Expanded State
    Rectangle {
        id:         expandedRect
        width:      parent.width
        height:     !_colapsed ? expandedCol.height + ScreenTools.defaultFontPixelHeight : 0
        color:      _validAdvisories ? getAispaceColor(QGroundControl.airspaceManager.advisories.airspaceColor) : _colorGray
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
                        fillMode:               Image.PreserveAspectFit
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
                            text:               _validAdvisories ? QGroundControl.airspaceManager.advisories.advisories.count + qsTr(" Advisories") : ""
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
                        visible:                QGroundControl.airspaceManager.weatherInfo.valid && showColapse
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
                    fillMode:               Image.PreserveAspectFit
                    anchors.right:          parent.right
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        showColapse
                        onClicked:      QGroundControl.airspaceManager.airspaceVisible = false
                    }
                }
                AirspaceWeather {
                    visible:                QGroundControl.airspaceManager.weatherInfo.valid && !showColapse
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
                            spacing:                    ScreenTools.defaultFontPixelHeight * 0.25
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
                                text:                       qsTr("Advisories based on the selected rules.")
                                color:                      _colorWhite
                                anchors.horizontalCenter:   parent.horizontalCenter
                                font.pointSize:             ScreenTools.smallFontPointSize
                            }
                            Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.125; }
                            GridLayout {
                                columns:                2
                                anchors.left:           parent.left
                                anchors.right:          parent.right
                                Rectangle {
                                    width:                  regButton.height
                                    height:                 width
                                    radius:                 2
                                    color:                  _colorGray
                                    Layout.alignment:       Qt.AlignVCenter
                                    QGCColoredImage {
                                        id:                 pencilIcon
                                        width:              height
                                        height:             parent.height * 0.5
                                        sourceSize.height:  height
                                        source:             "qrc:/airmap/pencil.svg"
                                        color:              _colorWhite
                                        fillMode:           Image.PreserveAspectFit
                                        anchors.centerIn:   parent
                                        MouseArea {
                                            anchors.fill:   parent
                                            onClicked: {
                                                ruleSelector.open()
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
                                        text:                   _validRules ? QGroundControl.airspaceManager.ruleSets.selectedRuleSets : qsTr("None")
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
                                model:      _validAdvisories ? QGroundControl.airspaceManager.advisories.advisories : []
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
            QGCButton {
                text:           planView ? qsTr("File Flight Plan") : qsTr("Flight Brief")
                backRadius:     4
                heightFactor:   0.3333
                showBorder:     true
                width:          ScreenTools.defaultFontPixelWidth * 16
                visible:        _flightPermit !== AirspaceFlightPlanProvider.PermitNone
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    planView ? flightDetails.open() : flightBrief.open()
                }
            }
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
    Popup {
        id:                 ruleSelector
        width:              rulesCol.width  + ScreenTools.defaultFontPixelWidth
        height:             rulesCol.height + ScreenTools.defaultFontPixelHeight
        modal:              true
        focus:              true
        parent:             Overlay.overlay
        closePolicy:        Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property var _popupTarget: null
        property var _arrowTarget: null

        onVisibleChanged: {
            if(visible) {
                _popupTarget = mainWindow.contentItem.mapFromItem(_root, 0, 0)
                _arrowTarget = mainWindow.contentItem.mapFromItem(pencilIcon, 0, 0)
            }
        }

        x:                  _popupTarget ? _popupTarget.x - width - (ScreenTools.defaultFontPixelWidth * 5) : 0
        y:                  _popupTarget ? _popupTarget.y + mainWindow.header.height : 0

        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            radius:         ScreenTools.defaultFontPixelWidth
        }

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
                    QGCLabel {
                        text:           qsTr("PICK ONE REGULATION")
                        font.pointSize: ScreenTools.smallFontPointSize
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                    }
                    Repeater {
                        model:    _validRules ? QGroundControl.airspaceManager.ruleSets.ruleSets : []
                        delegate: RuleSelector {
                            visible:             object.selectionType === AirspaceRuleSet.Pickone
                            rule:                object
                            autoExclusive:       true
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
                        model:    _validRules ? QGroundControl.airspaceManager.ruleSets.ruleSets : []
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
                        model:    _validRules ? QGroundControl.airspaceManager.ruleSets.ruleSets : []
                        delegate: RuleSelector {
                            visible:             object.selectionType === AirspaceRuleSet.Required
                            rule:                object
                            enabled:             false
                            anchors.right:       parent.right
                            anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                            anchors.left:        parent.left
                            anchors.leftMargin:  ScreenTools.defaultFontPixelWidth
                        }
                    }
                }
            }
        }

        //-- Arrow
        QGCColoredImage {
            id:                 arrowIcon
            width:              height
            height:             ScreenTools.defaultFontPixelHeight * 2
            sourceSize.height:  height
            source:             "qrc:/airmap/right-arrow.svg"
            color:              qgcPal.window
            anchors.left:       parent.right
            y:                  ruleSelector._arrowTarget ? (ruleSelector._arrowTarget.y - height) : 0
        }
    }

    //---------------------------------------------------------------
    //-- Flight Details
    Popup {
        id:                 flightDetails
        width:              flDetailsRow.width  + (ScreenTools.defaultFontPixelWidth  * 4)
        height:             flDetailsRow.height + (ScreenTools.defaultFontPixelHeight * 2)
        modal:              true
        focus:              true
        parent:             Overlay.overlay
        x:                  Math.round((mainWindow.width  - width)  * 0.5)
        y:                  Math.round((mainWindow.height - height) * 0.5)
        closePolicy:        Popup.NoAutoClose
        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            radius:         ScreenTools.defaultFontPixelWidth
        }
        Row {
            id:             flDetailsRow
            spacing:        ScreenTools.defaultFontPixelWidth
            anchors.centerIn: parent
            //---------------------------------------------------------
            //-- Flight Details
            FlightDetails {
                id:         _flightDetails
                baseHeight: _baseHeight
                baseWidth:  _baseWidth
            }
            //---------------------------------------------------------
            //-- Divider
            Rectangle {
                color:      qgcPal.text
                width:      1
                height:     parent.height
                opacity:    0.25
                anchors.verticalCenter: parent.verticalCenter
            }
            //---------------------------------------------------------
            //-- Flight Brief
            FlightBrief {
                baseHeight: _baseHeight
                baseWidth:  _baseWidth
                onClosed:   flightDetails.close()
            }
        }
    }
    //---------------------------------------------------------------
    //-- Flight Brief
    Popup {
        id:                 flightBrief
        width:              flightBriedItem.width  + (ScreenTools.defaultFontPixelWidth  * 4)
        height:             flightBriedItem.height + (ScreenTools.defaultFontPixelHeight * 2)
        modal:              true
        focus:              true
        parent:             Overlay.overlay
        x:                  Math.round((mainWindow.width  - width)  * 0.5)
        y:                  Math.round((mainWindow.height - height) * 0.5)
        closePolicy:        Popup.NoAutoClose
        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            radius:         ScreenTools.defaultFontPixelWidth
        }
        //---------------------------------------------------------
        //-- Flight Brief
        FlightBrief {
            id:             flightBriedItem
            baseHeight:     _baseHeight
            baseWidth:      _baseWidth
            onClosed:       flightBrief.close()
            anchors.centerIn: parent
        }
    }
}
