import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:             airframePage
    pageComponent:  (controller && controller.showCustomConfigPanel) ? customFrame : pageComponent

    readonly property string _unknownImage:     "qrc:/qmlimages/Airframe/AirframeUnknown.svg"
    // Keys match AirframeType::category
    readonly property var _categoryKeys:        [ "", "Copter", "Plane", "VTOL", "Rover", "Other" ]
    readonly property var _categoryLabels:      [ qsTr("All"), qsTr("Copter"), qsTr("Plane"), qsTr("VTOL"), qsTr("Rover"), qsTr("Other") ]
    readonly property real _cardWidth:          ScreenTools.defaultFontPixelWidth * 22
    readonly property real _detailPanelWidth:   ScreenTools.defaultFontPixelWidth * 40
    // The airframe picker sits beside the cards when two card columns and the picker fit, otherwise it stacks below
    readonly property bool _wide:               availableWidth >= _cardWidth * 2 + _detailPanelWidth + _margins * 4

    property int    _categoryIndex:     _currentCategoryIndex()
    property var    _selectedType:      _typeByName(controller.currentAirframeType)
    property var    _pendingAirframe:   null

    readonly property var _currentType:             _typeByName(controller.currentAirframeType)
    readonly property var _visibleTypes:            controller.airframeTypes.filter(type => _typeShown(type))
    readonly property int _effectiveAutostartId:    _pendingAirframe ? _pendingAirframe.autostartId : controller.currentAutostartId

    function _typeByName(name) {
        for (const type of controller.airframeTypes) {
            if (type.name === name) {
                return type
            }
        }
        return null
    }

    function _currentCategoryIndex() {
        const type = _typeByName(controller.currentAirframeType)
        return type ? Math.max(_categoryKeys.indexOf(type.category), 0) : 0
    }

    function _categoryLabel(key) {
        const index = _categoryKeys.indexOf(key)
        return index > 0 ? _categoryLabels[index] : _categoryLabels[_categoryLabels.length - 1]
    }

    function _typeShown(type) {
        const key = _categoryKeys[_categoryIndex]
        return key === "" || type.category === key
    }

    function _pick(airframe) {
        if (airframe.autostartId === controller.currentAutostartId) {
            _clearPending()
            return
        }
        _pendingAirframe = airframe
        controller.autostartId = airframe.autostartId
    }

    function _clearPending() {
        _pendingAirframe = null
        controller.autostartId = controller.currentAutostartId
    }

    AirframeComponentController {
        id:         controller
    }

    QGCPalette {
        id: qgcPal
    }

    Component {
        id: customFrame
        Column {
            width:          availableWidth
            spacing:        ScreenTools.defaultFontPixelHeight * 4
            Item {
                width:      1
                height:     1
            }
            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                width:      parent.width * 0.5
                height:     ScreenTools.defaultFontPixelHeight * 4
                wrapMode:   Text.WordWrap
                text:       qsTr("Your vehicle is using a custom airframe configuration. ") +
                            qsTr("This configuration can only be modified through the Parameter Editor.\n\n") +
                            qsTr("If you want to reset your airframe configuration and select a standard configuration, click 'Reset' below.")
            }
            QGCButton {
                text:       qsTr("Reset")
                enabled:    sys_autostart
                anchors.horizontalCenter: parent.horizontalCenter
                property Fact sys_autostart: controller.getParameterFact(-1, "SYS_AUTOSTART")
                onClicked: {
                    if(sys_autostart) {
                        sys_autostart.value = 0
                    }
                }
            }
        }
    }

    component ImageWell: Rectangle {
        property alias source: wellImage.source

        radius: ScreenTools.defaultBorderRadius
        color:  "white" // The airframe artwork is black line art on white

        Image {
            id:                 wellImage
            anchors.fill:       parent
            anchors.margins:    parent.width * 0.06
            fillMode:           Image.PreserveAspectFit
            smooth:             true
            mipmap:             true
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            height:  airframePage._wide ? Math.max(availableHeight, implicitHeight) : implicitHeight
            spacing: _margins

            ConfigRowsPanel {
                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelWidth * 2

                    ImageWell {
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 6
                        Layout.preferredHeight: Layout.preferredWidth
                        source:                 airframePage._currentType ? airframePage._currentType.imageResource : _unknownImage
                    }

                    ColumnLayout {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.25

                        QGCLabel {
                            text:                   qsTr("Connected airframe")
                            font.pointSize:         ScreenTools.smallFontPointSize
                            font.bold:              true
                            font.capitalization:    Font.AllUppercase
                            color:                  qgcPal.textSecondary
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            font.bold:          true
                            font.pointSize:     ScreenTools.largeFontPointSize
                            text:               airframePage._currentType ? controller.currentVehicleName : qsTr("Airframe is not set")
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            wrapMode:           Text.WordWrap
                            font.family:        airframePage._currentType ? ScreenTools.fixedFontFamily : ScreenTools.normalFontFamily
                            color:              qgcPal.textSecondary
                            text:               airframePage._currentType ?
                                                    qsTr("SYS_AUTOSTART %1 - %2").arg(controller.currentAutostartId).arg(controller.currentAirframeType) :
                                                    qsTr("Select a frame type below, then choose your airframe.")
                        }
                    }
                }
            }

            ConfigRowsPanel {
                ConfigRowsFilterBar {
                    objectName:         "airframe_filter"
                    filterLabels:       airframePage._categoryLabels
                    filterIndex:        airframePage._categoryIndex
                    summary:            qsTr("Showing %1 of %2 frame types.").arg(airframePage._visibleTypes.length).arg(controller.airframeTypes.length)
                    onFilterActivated:  (index) => { airframePage._categoryIndex = index }
                }
            }

            GridLayout {
                Layout.fillWidth:   true
                Layout.fillHeight:  airframePage._wide
                columns:            airframePage._wide ? 2 : 1
                columnSpacing:      _margins
                rowSpacing:         _margins

                ConfigRowsPanel {
                    Layout.fillHeight: airframePage._wide

                    QGCLabel {
                        Layout.fillWidth:   true
                        color:              qgcPal.textSecondary
                        visible:            airframePage._visibleTypes.length === 0
                        text:               qsTr("No frame types match.")
                    }

                    QGCFlickable {
                        id:                     cardFlickable
                        Layout.fillWidth:       true
                        Layout.fillHeight:      airframePage._wide
                        Layout.preferredHeight: airframePage._wide ? -1 : contentHeight
                        contentWidth:           width
                        contentHeight:          cardFlow.height
                        interactive:            airframePage._wide

                        Flow {
                            id:         cardFlow
                            width:      cardFlickable.width
                            spacing:    _margins

                            Repeater {
                                model: airframePage._visibleTypes

                                Rectangle {
                                    id:             card
                                    objectName:     "airframeTypeBox_" + index
                                    width:          airframePage._cardWidth
                                    implicitHeight: cardLayout.implicitHeight + _margins * 2
                                    radius:         ScreenTools.defaultBorderRadius * 2
                                    color:          airframeTypeSelected ? qgcPal.accentSubtle : qgcPal.windowCard
                                    border.color:   airframeTypeSelected || cardMouseArea.containsMouse ? qgcPal.accent : qgcPal.borderSubtle

                                    required property var modelData
                                    required property int index

                                    property string airframeTypeName:       modelData.name
                                    property bool   airframeTypeSelected:   airframePage._selectedType === modelData

                                    MouseArea {
                                        id:             cardMouseArea
                                        anchors.fill:   parent
                                        hoverEnabled:   true
                                        onClicked: {
                                            if (airframePage._selectedType !== card.modelData) {
                                                airframePage._clearPending()
                                                airframePage._selectedType = card.modelData
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        id:                 cardLayout
                                        anchors.fill:       parent
                                        anchors.margins:    _margins
                                        spacing:            _margins / 2

                                        ImageWell {
                                            Layout.fillWidth:       true
                                            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 6
                                            source:                 card.modelData.imageResource

                                            QGCChip {
                                                x:          _margins
                                                y:          _margins
                                                text:       qsTr("Current")
                                                textColor:  qgcPal.colorGreen
                                                visible:    card.modelData.name === controller.currentAirframeType
                                            }
                                        }

                                        QGCLabel {
                                            Layout.fillWidth:   true
                                            elide:              Text.ElideRight
                                            font.bold:          true
                                            text:               card.modelData.name
                                        }

                                        QGCLabel {
                                            color:          qgcPal.textSecondary
                                            font.pointSize: ScreenTools.smallFontPointSize
                                            text:           airframePage._categoryLabel(card.modelData.category) + " - " +
                                                            qsTr("%n airframe(s)", "", card.modelData.airframes.length)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                ConfigRowsPanel {
                    Layout.fillWidth:       !airframePage._wide
                    Layout.fillHeight:      airframePage._wide
                    Layout.preferredWidth:  airframePage._wide ? airframePage._detailPanelWidth : -1

                    ImageWell {
                        Layout.fillWidth:       airframePage._wide
                        Layout.preferredWidth:  airframePage._wide ? -1 : Layout.preferredHeight
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 8
                        visible:                airframePage._selectedType !== null
                        source:                 airframePage._selectedType ? airframePage._selectedType.imageResource : ""
                    }

                    QGCLabel {
                        Layout.fillWidth:       true
                        wrapMode:               Text.WordWrap
                        font.bold:              true
                        font.capitalization:    Font.AllUppercase
                        text:                   airframePage._selectedType ? airframePage._selectedType.name : qsTr("Airframes")
                    }

                    QGCLabel {
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        color:              qgcPal.textSecondary
                        text:               airframePage._selectedType ?
                                                airframePage._categoryLabel(airframePage._selectedType.category) + " - " +
                                                    qsTr("%n airframe(s)", "", airframePage._selectedType.airframes.length) :
                                                qsTr("Select a frame type to choose an airframe.")
                    }

                    QGCFlickable {
                        id:                     optionFlickable
                        Layout.fillWidth:       true
                        Layout.fillHeight:      airframePage._wide
                        Layout.preferredHeight: airframePage._wide ? -1 : contentHeight
                        Layout.minimumHeight:   airframePage._wide ? ScreenTools.defaultFontPixelHeight * 6 : 0
                        contentWidth:           width
                        contentHeight:          optionColumn.height
                        interactive:            airframePage._wide

                        Column {
                            id:         optionColumn
                            width:      optionFlickable.width
                            spacing:    _margins / 2

                            Repeater {
                                model: airframePage._selectedType ? airframePage._selectedType.airframes : []

                                Rectangle {
                                    id:             option
                                    objectName:     "airframeOption_" + index
                                    width:          optionColumn.width
                                    height:         optionRow.implicitHeight + _margins
                                    radius:         ScreenTools.defaultBorderRadius
                                    color:          checked ? qgcPal.accentSubtle : qgcPal.windowCard
                                    border.color:   checked || optionMouseArea.containsMouse ? qgcPal.accent : qgcPal.borderSubtle

                                    required property var modelData
                                    required property int index

                                    readonly property int   autostartId:    modelData.autostartId
                                    readonly property bool  checked:        autostartId === airframePage._effectiveAutostartId

                                    MouseArea {
                                        id:             optionMouseArea
                                        anchors.fill:   parent
                                        hoverEnabled:   true
                                        onClicked:      airframePage._pick(option.modelData)
                                    }

                                    RowLayout {
                                        id:                 optionRow
                                        anchors.left:       parent.left
                                        anchors.right:      parent.right
                                        anchors.margins:    _margins / 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing:            _margins

                                        QGCLabel {
                                            Layout.fillWidth:   true
                                            wrapMode:           Text.WordWrap
                                            text:               option.modelData.text
                                        }

                                        QGCChip {
                                            text:       qsTr("Current")
                                            textColor:  qgcPal.colorGreen
                                            visible:    option.autostartId === controller.currentAutostartId
                                        }

                                        QGCLabel {
                                            font.family:    ScreenTools.fixedFontFamily
                                            font.pointSize: ScreenTools.smallFontPointSize
                                            color:          qgcPal.textSecondary
                                            text:           option.autostartId
                                        }
                                    }
                                }
                            }
                        }
                    }

                    QGCLabel {
                        objectName:         "airframeSetup_pendingLabel"
                        Layout.fillWidth:   true
                        wrapMode:           Text.WordWrap
                        color:              airframePage._pendingAirframe ? qgcPal.warningText : qgcPal.textSecondary
                        text:               airframePage._pendingAirframe ?
                                                qsTr("Selected: %1 (%2). Click 'Apply and Restart' to change the airframe.")
                                                    .arg(airframePage._pendingAirframe.text)
                                                    .arg(airframePage._pendingAirframe.autostartId) :
                                                qsTr("Choose an airframe, then click 'Apply and Restart'.")
                    }

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            _margins

                        Item {
                            Layout.fillWidth: true
                        }

                        QGCButton {
                            objectName: "airframeSetup_discardButton"
                            text:       qsTr("Discard")
                            visible:    airframePage._pendingAirframe !== null
                            onClicked:  airframePage._clearPending()
                        }

                        QGCButton {
                            objectName: "airframeSetup_applyButton"
                            text:       qsTr("Apply and Restart")
                            primary:    true
                            visible:    airframePage._pendingAirframe !== null
                            onClicked:  QGroundControl.showMessageDialog(airframePage, qsTr("Apply and Restart"),
                                                                         qsTr("Clicking 'Apply' will save the changes you have made to your airframe configuration.<br><br>\
                                                                                All vehicle parameters other than Radio Calibration will be reset.<br><br>\
                                                                                Your vehicle will also be restarted in order to complete the process."),
                                                                         Dialog.Apply | Dialog.Cancel,
                                                                         function() { controller.changeAutostart() })
                        }
                    }
                }
            }
        }
    }
} // SetupPage
