import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
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

    property color  _airspaceColor:     _validAdvisories ? getAispaceColor(QGroundControl.airspaceManager.advisories.airspaceColor) : _colorGray
    property bool   _validRules:        QGroundControl.airspaceManager.ruleSets.valid
    property bool   _validAdvisories:   QGroundControl.airspaceManager.advisories.valid
    property color  _textColor:         qgcPal.text
    property bool   _colapsed:          !QGroundControl.airspaceManager.airspaceVisible
    property var    _flightPermit:      QGroundControl.airspaceManager.flightPlan.flightPermitStatus

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
        color:      _validAdvisories ? getAispaceColor(QGroundControl.airspaceManager.advisories.airspaceColor) : _colorGray
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
                visible:                QGroundControl.airspaceManager.weatherInfo.valid
                contentColor:           _textColor
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
            anchors.right:          parent.right
            anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
        }
        MouseArea {
            anchors.fill:   parent
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
                                    anchors.verticalCenter: parent.verticalCenter
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
                        height:             ScreenTools.defaultFontPixelHeight * 6
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
                text:           qsTr("File Flight Plan")
                backRadius:     4
                heightFactor:   0.3333
                showBorder:     true
                width:          ScreenTools.defaultFontPixelWidth * 16
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    rootLoader.sourceComponent = flightDetails
                    mainWindow.disableToolbar()
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
                                model:    _validRules ? QGroundControl.airspaceManager.ruleSets.ruleSets : []
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
    //---------------------------------------------------------------
    //-- Flight Details
    Component {
        id:             flightDetails
        Rectangle {
            id:         flightDetailsRoot
            width:      mainWindow.width
            height:     mainWindow.height
            color:      Qt.rgba(0,0,0,0.1)
            property real flickHeight: ScreenTools.defaultFontPixelHeight * 22
            property real flickWidth:  ScreenTools.defaultFontPixelWidth  * 40
            MouseArea {
                anchors.fill:   parent
                hoverEnabled:   true
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             flightDetailsShadow
                anchors.fill:   flightDetailsRect
                radius:         flightDetailsRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       flightDetailsShadow
                visible:            flightDetailsRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             flightDetailsShadow
            }
            Rectangle {
                id:                 flightDetailsRect
                color:              qgcPal.window
                width:              flDetailsRow.width  + (ScreenTools.defaultFontPixelWidth  * 4)
                height:             flDetailsRow.height + (ScreenTools.defaultFontPixelHeight * 2)
                radius:             ScreenTools.defaultFontPixelWidth
                anchors.centerIn:   parent
                Row {
                    id:             flDetailsRow
                    spacing:        ScreenTools.defaultFontPixelWidth
                    anchors.centerIn: parent
                    //---------------------------------------------------------
                    //-- Flight Details
                    Column {
                        spacing:        ScreenTools.defaultFontPixelHeight * 0.25
                        Rectangle {
                            color:          qgcPal.windowShade
                            anchors.right:  parent.right
                            anchors.left:   parent.left
                            height:         detailsLabel.height + ScreenTools.defaultFontPixelHeight
                            QGCLabel {
                                id:             detailsLabel
                                text:           qsTr("Flight Details")
                                font.pointSize: ScreenTools.mediumFontPointSize
                                font.family:    ScreenTools.demiboldFontFamily
                                anchors.centerIn: parent
                            }
                        }
                        Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.5; }
                        Flickable {
                            clip:           true
                            width:          flightDetailsRoot.flickWidth
                            height:         flightDetailsRoot.flickHeight
                            contentHeight:  flContextCol.height
                            flickableDirection: Flickable.VerticalFlick
                            Column {
                                id:                 flContextCol
                                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                                anchors.right:      parent.right
                                anchors.left:       parent.left
                                QGCLabel {
                                    text:           qsTr("Flight Date & Time")
                                }
                                Rectangle {
                                    id:             dateRect
                                    color:          qgcPal.windowShade
                                    anchors.right:  parent.right
                                    anchors.left:   parent.left
                                    height:         datePickerCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                                    Column {
                                        id:                 datePickerCol
                                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                                        anchors.right:      parent.right
                                        anchors.left:       parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        QGCButton {
                                            text: {
                                                var today = new Date();
                                                if(datePicker.selectedDate.setHours(0,0,0,0) === today.setHours(0,0,0,0)) {
                                                    return qsTr("Today")
                                                } else {
                                                    return datePicker.selectedDate.toLocaleDateString(Qt.locale())
                                                }
                                            }
                                            iconSource:     "qrc:/airmap/expand.svg"
                                            anchors.right:  parent.right
                                            anchors.left:   parent.left
                                            onClicked:      datePicker.visible = true
                                        }
                                        Item {
                                            anchors.right:  parent.right
                                            anchors.left:   parent.left
                                            height:         timeSlider.height
                                            QGCLabel {
                                                id:         timeLabel
                                                text:       ('00' + hour).slice(-2) + ":" + ('00' + minute).slice(-2)
                                                width:      ScreenTools.defaultFontPixelWidth * 5
                                                anchors.left:  parent.left
                                                anchors.verticalCenter: parent.verticalCenter
                                                property int hour:   Math.floor(timeSlider.value * 0.25)
                                                property int minute: (timeSlider.value * 15) % 60
                                            }
                                            QGCSlider {
                                                id:             timeSlider
                                                width:          parent.width - timeLabel.width - ScreenTools.defaultFontPixelWidth
                                                stepSize:       1
                                                minimumValue:   0
                                                maximumValue:   95 // 96 blocks of 15 minutes in 24 hours
                                                anchors.right:  parent.right
                                                anchors.verticalCenter: parent.verticalCenter
                                                Component.onCompleted: {
                                                    var today = new Date()
                                                    var val = (((today.getHours() * 60) + today.getMinutes()) * (96/1440)) + 1
                                                    if(val > 95) val = 95
                                                    value = Math.ceil(val)
                                                }
                                            }
                                        }
                                    }
                                }
                                Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.25; }
                                QGCLabel {
                                    text:           qsTr("Flight Context")
                                }
                                Repeater {
                                    model:          [1, 2, 3, 4, 5, 6]
                                    delegate: FlightFeature {
                                        anchors.right:  parent.right
                                        anchors.left:   parent.left
                                    }
                                }
                            }
                        }
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
                    Column {
                        spacing:        ScreenTools.defaultFontPixelHeight * 0.25
                        Rectangle {
                            color:          qgcPal.windowShade
                            anchors.right:  parent.right
                            anchors.left:   parent.left
                            height:         briefLabel.height + ScreenTools.defaultFontPixelHeight
                            QGCLabel {
                                id:             briefLabel
                                text:           qsTr("Flight Brief")
                                font.pointSize: ScreenTools.mediumFontPointSize
                                font.family:    ScreenTools.demiboldFontFamily
                                anchors.centerIn: parent
                            }
                        }
                        Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.5; }
                        Flickable {
                            clip:           true
                            width:          flightDetailsRoot.flickWidth
                            height:         flightDetailsRoot.flickHeight - buttonRow.height - ScreenTools.defaultFontPixelHeight
                            contentHeight:  briefCol.height
                            flickableDirection: Flickable.VerticalFlick
                            Column {
                                id:                 briefCol
                                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                                anchors.right:      parent.right
                                anchors.left:       parent.left
                                QGCLabel {
                                    text:           qsTr("Authorizations")
                                }
                                Rectangle {
                                    color:          qgcPal.windowShade
                                    anchors.right:  parent.right
                                    anchors.left:   parent.left
                                    height:         dateRect.height
                                    Column {
                                        id:                 authCol
                                        spacing:            ScreenTools.defaultFontPixelHeight * 0.25
                                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                                        anchors.right:      parent.right
                                        anchors.left:       parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        QGCLabel {
                                            text:           qsTr("Federal Aviation Administration")
                                        }
                                        QGCLabel {
                                            text:           qsTr("Automatic authorization to fly in controlled airspace")
                                            font.pointSize: ScreenTools.smallFontPointSize
                                        }
                                    }
                                }
                                Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.25; }
                                QGCLabel {
                                    text:           qsTr("Rules & Compliance")
                                }
                                ComplianceRules {
                                    text:           qsTr("Rules you may be violating")
                                    anchors.right:  parent.right
                                    anchors.left:   parent.left
                                }
                                ComplianceRules {
                                    text:           qsTr("Rules needing more information")
                                    anchors.right:  parent.right
                                    anchors.left:   parent.left
                                }
                                ComplianceRules {
                                    text:           qsTr("Rules you should review")
                                    anchors.right:  parent.right
                                    anchors.left:   parent.left
                                }
                                ComplianceRules {
                                    text:           qsTr("Rules you are following")
                                    anchors.right:  parent.right
                                    anchors.left:   parent.left
                                }
                            }
                        }
                        //-------------------------------------------------------------
                        //-- File Flight Plan or Close
                        Item { width: 1; height: ScreenTools.defaultFontPixelHeight; }
                        Row {
                            id:         buttonRow
                            spacing: ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCButton {
                                text:           qsTr("Update Plan")
                                backRadius:     4
                                heightFactor:   0.3333
                                showBorder:     true
                                enabled:        _flightPermit !== AirspaceFlightPlanProvider.PermitNone
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                onClicked: {
                                    //-- TODO: Update Plan
                                    mainWindow.enableToolbar()
                                    rootLoader.sourceComponent = null
                                }
                            }
                            QGCButton {
                                text:           qsTr("Submit Plan")
                                backRadius:     4
                                heightFactor:   0.3333
                                showBorder:     true
                                enabled:        _flightPermit !== AirspaceFlightPlanProvider.PermitNone
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                onClicked: {
                                    //-- TODO: File Plan
                                    mainWindow.enableToolbar()
                                    rootLoader.sourceComponent = null
                                }
                            }
                            QGCButton {
                                text:           qsTr("Close")
                                backRadius:     4
                                heightFactor:   0.3333
                                showBorder:     true
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                onClicked: {
                                    mainWindow.enableToolbar()
                                    rootLoader.sourceComponent = null
                                }
                            }
                        }
                    }
                }
            }
            Calendar {
                id: datePicker
                anchors.centerIn: parent
                visible: false;
                minimumDate: {
                    return new Date()
                }
                onClicked: {
                    visible = false;
                }
            }
            Component.onCompleted: {
                mainWindow.disableToolbar()
            }
        }
    }
}
