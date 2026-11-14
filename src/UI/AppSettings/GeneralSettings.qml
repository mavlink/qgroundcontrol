/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import QGroundControl.Palette

SettingsPage {
    id:page
    property var    _settingsManager:           QGroundControl.settingsManager
    property var    _appSettings:               _settingsManager.appSettings
    property var    _brandImageSettings:        _settingsManager.brandImageSettings
    property Fact   _appFontPointSize:          _appSettings.appFontPointSize
    property Fact   _userBrandImageIndoor:      _brandImageSettings.userBrandImageIndoor
    property Fact   _userBrandImageOutdoor:     _brandImageSettings.userBrandImageOutdoor
    property Fact   _appSavePath:               _appSettings.savePath
    // card width follows the window: 80% of the page, never below 50 or above 110 character-widths
    readonly property real _cardW: Math.max(ScreenTools.defaultFontPixelWidth * 50,
                                            Math.min(page.width * 0.8, ScreenTools.defaultFontPixelWidth * 110))

    // Settings card: gray outer box (heading) + inner box (rows)
    component AppCard: Rectangle {
        id: card

        default property alias content: contentLayout.data
        property string heading:            ""
        property string headingDescription: ""
        property bool   showDividers:       true
        property real   targetWidth:        0

        readonly property bool  _light:  QGroundControl.globalPalette.globalTheme === QGCPalette.Light
        readonly property color _cOuter: _light ? "#F5F5F5" : "#404040"
        readonly property color _cInner: _light ? "#FFFFFF" : "#202020"
        readonly property color _cLine:  _light ? "#E8E8E8" : "#606060"
        readonly property color _cEdge:  _light ? "#D6D7DC" : "#2A394A"
        readonly property real  _m:      ScreenTools.defaultFontPixelHeight / 2

        Layout.fillWidth:   true
        implicitWidth:       Math.max(outerLayout.implicitWidth + 24, targetWidth)
        implicitHeight:     outerLayout.implicitHeight + 24
        radius:             20
        color:              _cOuter
        border.color:       _cEdge
        border.width:       1

        ColumnLayout {
            id:                 outerLayout
            anchors.fill:       parent
            anchors.margins:    6
            spacing:            0

            // Heading inside the outer box
            ColumnLayout {
                Layout.fillWidth:       true
                Layout.leftMargin:      10
                Layout.topMargin:       8
                Layout.bottomMargin:    10
                spacing:                2
                visible:                card.heading !== ""

                QGCLabel {
                    text:           card.heading
                    font.pointSize: ScreenTools.defaultFontPointSize + 3
                    font.bold:      true
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               card.headingDescription
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.smallFontPointSize
                    opacity:            0.6
                    visible:            card.headingDescription !== ""
                }
            }

            // Inner box with the rows
            Rectangle {
                id:                 innerCard
                Layout.fillWidth:   true
                implicitWidth:      contentLayout.implicitWidth + card._m * 2
                implicitHeight:     contentLayout.implicitHeight + card._m * 2
                radius:             14
                color:              card._cInner
                border.color:       card._cLine
                border.width:       card._light ? 0 : 1

                // soft shadow under the inner box
                Rectangle {
                    z:                  -1
                    anchors.fill:       parent
                    anchors.topMargin:  2
                    anchors.bottomMargin: -2
                    radius:             parent.radius
                    color:              "#000000"
                    opacity:            card._light ? 0.07 : 0.25
                }

                // divider lines between rows
                Repeater {
                    model: card.showDividers ? contentLayout.children.length : 0

                    Rectangle {
                        x:          card._m
                        y:          contentLayout.y + _item.y + _item.height + card._m
                        width:      innerCard.width - card._m * 2
                        height:     1
                        color:      card._cLine
                        visible:    _item.visible && _item.width !== 0 && _item.height !== 0 &&
                                        index < contentLayout.children.length - 1

                        property var _item: contentLayout.children[index]
                    }
                }

                ColumnLayout {
                    id:         contentLayout
                    x:          card._m
                    y:          card._m
                    width:      innerCard.width - card._m * 2
                    spacing:    card._m * 2
                }
            }
        }
    }



    Item { Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5 }

    AppCard {
        Layout.fillWidth:   true
        targetWidth:        page._cardW
        heading:            qsTr("General")

        LabelledFactComboBox {
            label:      qsTr("Language")
            fact:       _appSettings.qLocaleLanguage
            indexModel: false
            visible:    _appSettings.qLocaleLanguage.visible
        }

        LabelledFactComboBox {
            label:      qsTr("Color Scheme")
            fact:       _appSettings.indoorPalette
            indexModel: false
            visible:    _appSettings.indoorPalette.visible
        }

        LabelledFactComboBox {
            label:       qsTr("Stream GCS Position")
            fact:       _appSettings.followTarget
            indexModel: false
            visible:    _appSettings.followTarget.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text:           qsTr("Mute all audio output")
            fact:       _audioMuted
            visible:    _audioMuted.visible
            property Fact _audioMuted: _appSettings.audioMuted
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text:       qsTr("Save application data to SD Card")
            fact:       _androidSaveToSDCard
            visible:    _androidSaveToSDCard.visible
            property Fact _androidSaveToSDCard: _appSettings.androidSaveToSDCard
        }

        QGCCheckBoxSlider {
            Layout.fillWidth: true
            text:       qsTr("Clear all settings on next start")
            checked:    false
            onClicked: {
                if (checked) {
                    QGroundControl.deleteAllSettingsNextBoot()
                } else {
                    QGroundControl.clearDeleteAllSettingsNextBoot()
                }
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth * 2
            visible:            _appFontPointSize.visible

            QGCLabel { 
                Layout.fillWidth:   true
                text:               qsTr("UI Scaling") 
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth * 2

                QGCButton {
                    Layout.preferredWidth:  height
                    height:                 baseFontEdit.height * 1.5
                    text:                   "-"
                    onClicked: {
                        if (_appFontPointSize.value > _appFontPointSize.min) {
                            _appFontPointSize.value = _appFontPointSize.value - 1
                        }
                    }
                }

                QGCLabel {
                    id:                     baseFontEdit
                    width:                  ScreenTools.defaultFontPixelWidth * 6
                    text:                   (QGroundControl.settingsManager.appSettings.appFontPointSize.value / ScreenTools.platformFontPointSize * 100).toFixed(0) + "%"
                }

                QGCButton {
                    Layout.preferredWidth:  height
                    height:                 baseFontEdit.height * 1.5
                    text:                   "+"
                    onClicked: {
                        if (_appFontPointSize.value < _appFontPointSize.max) {
                            _appFontPointSize.value = _appFontPointSize.value + 1
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth * 2
            visible:            _appSavePath.visible && !ScreenTools.isMobile

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            0

                QGCLabel { text: qsTr("Application Load/Save Path") }
                QGCLabel { 
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               _appSavePath.rawValue === "" ? qsTr("<default location>") : _appSavePath.value
                    elide:              Text.ElideMiddle
                }
            }

            QGCButton {
                text:       qsTr("Browse")
                onClicked:  savePathBrowseDialog.openForLoad()
                QGCFileDialog {
                    id:                 savePathBrowseDialog
                    title:              qsTr("Choose the location to save/load files")
                    folder:             _appSavePath.rawValue
                    selectFolder:       true
                    onAcceptedForLoad:  (file) => _appSavePath.rawValue = file
                }
            }
        }
    }

    AppCard {
        Layout.fillWidth:   true
        targetWidth:        page._cardW
        heading:            qsTr("Units")
        visible:            QGroundControl.settingsManager.unitsSettings.visible

        Repeater {
            model: [ QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits, QGroundControl.settingsManager.unitsSettings.verticalDistanceUnits, QGroundControl.settingsManager.unitsSettings.areaUnits, QGroundControl.settingsManager.unitsSettings.speedUnits, QGroundControl.settingsManager.unitsSettings.temperatureUnits ]

            LabelledFactComboBox {
                label:                  modelData.shortDescription
                fact:                   modelData
                indexModel:             false
            }
        }
    }

    AppCard {
        Layout.fillWidth:   true
        targetWidth:        page._cardW
        heading:            qsTr("Brand Image")
        visible:            _brandImageSettings.visible && !ScreenTools.isMobile
        
        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth * 2
            visible:            _userBrandImageIndoor.visible

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            0

                QGCLabel { 
                    Layout.fillWidth:   true
                    text:               qsTr("Indoor Image") 
                }
                QGCLabel { 
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               _userBrandImageIndoor.valueString.replace("file:///", "") 
                    elide:              Text.ElideMiddle
                    visible:            _userBrandImageIndoor.valueString.length > 0
                }
            }

            QGCButton {
                text:       qsTr("Browse")
                onClicked:  userBrandImageIndoorBrowseDialog.openForLoad()

                QGCFileDialog {
                    id:                 userBrandImageIndoorBrowseDialog
                    title:              qsTr("Choose custom brand image file")
                    folder:             _userBrandImageIndoor.rawValue.replace("file:///", "")
                    selectFolder:       false
                    onAcceptedForLoad:  (file) => _userBrandImageIndoor.rawValue = "file:///" + file
                }
            }
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth * 2
            visible:            _userBrandImageOutdoor.visible

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            0

                QGCLabel { 
                    Layout.fillWidth:   true
                    text:               qsTr("Outdoor Image") 
                }
                QGCLabel { 
                    Layout.fillWidth:   true
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               _userBrandImageOutdoor.valueString.replace("file:///", "") 
                    elide:              Text.ElideMiddle
                    visible:            _userBrandImageOutdoor.valueString.length > 0
                }
            }

            QGCButton {
                text:       qsTr("Browse")
                onClicked:  userBrandImageOutdoorBrowseDialog.openForLoad()

                QGCFileDialog {
                    id:                 userBrandImageOutdoorBrowseDialog
                    title:              qsTr("Choose custom brand image file")
                    folder:             _userBrandImageOutdoor.rawValue.replace("file:///", "")
                    selectFolder:       false
                    onAcceptedForLoad:  (file) => _userBrandImageOutdoor.rawValue = "file:///" + file
                }
            }
        }

        LabelledButton {
            label:      qsTr("Reset Images")
            buttonText: qsTr("Reset")
            onClicked:  {
                _userBrandImageIndoor.rawValue = ""
                _userBrandImageOutdoor.rawValue = ""
            }
        }
    }
}
