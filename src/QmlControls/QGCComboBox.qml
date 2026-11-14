/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Templates as T

import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls

T.ComboBox {
    id:             control
    padding:        ScreenTools.comboBoxPadding
    spacing:        ScreenTools.defaultFontPixelWidth
    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily
    implicitWidth:  Math.max(background ? background.implicitWidth : 0,
                             contentItem.implicitWidth + leftPadding + rightPadding + padding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(contentItem.implicitHeight, indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)
    baselineOffset: contentItem.y + text.baselineOffset
    leftPadding:    padding + (_settingsStyle ? ScreenTools.defaultFontPixelWidth : 0) + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding:   padding + (_settingsStyle ? ScreenTools.defaultFontPixelWidth : 0) + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width)

    property bool   centeredLabel:  false
    property bool   sizeToContents: false
    property string alternateText:  ""

    property real   _popupWidth:    width
    property bool   _onCompleted:   false
    property bool   _settingsStyle: (typeof mainWindow !== "undefined") && mainWindow.settingsPageOpen
    property bool   _showBorder:    _settingsStyle || qgcPal.globalTheme === QGCPalette.Light

    readonly property bool  _light:         qgcPal.globalTheme === QGCPalette.Light
    readonly property color _popBg:         _light ? "#FFFFFF" : "#404040"
    readonly property color _popBorder:     _light ? "#E2E2E2" : "#606060"
    readonly property color _popSelected:   _light ? "#EBEBEB" : "#606060"
    readonly property color _popHover:      _light ? "#F4F4F4" : "#505050"

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    TextMetrics {
        id:                 textMetrics
        font.family:        control.font.family
        font.pointSize:     control.font.pointSize
    }

    ItemDelegate {
        id:             itemDelegateMetrics
        visible:        false
        font.family:    control.font.family
        font.pointSize: control.font.pointSize
    }

    function _calcPopupWidth() {
        if (_onCompleted && sizeToContents && model) {
            var largestTextWidth = 0
            for (var i = 0; i < model.length; i++){
                textMetrics.text = control.textRole ? model[i][control.textRole] : model[i]
                largestTextWidth = Math.max(textMetrics.width, largestTextWidth)
            }
            _popupWidth = largestTextWidth + itemDelegateMetrics.leftPadding + itemDelegateMetrics.rightPadding
        }
    }

    onModelChanged: _calcPopupWidth()

    Component.onCompleted: {
        _onCompleted = true
        _calcPopupWidth()
    }

    // The items in the popup
    delegate: ItemDelegate {
        id:     itemDel
        width:  control._settingsStyle ? _popupWidth - 12 : _popupWidth
        height: Math.round(popupItemMetrics.height * 1.75)

        property string _text: control.textRole ?
                                    (model.hasOwnProperty(control.textRole) ? model[control.textRole] : modelData[control.textRole]) :
                                    modelData

        TextMetrics {
            id:             popupItemMetrics
            font:           control.font
            text:           _text
        }

        contentItem: Text {
            text:                   _text
            font:                   control.font
            color:                  control._settingsStyle ? qgcPal.text : (control.currentIndex === index ? qgcPal.buttonHighlightText : qgcPal.buttonText)
            verticalAlignment:      Text.AlignVCenter
        }

        background: Rectangle {
            radius: control._settingsStyle ? 8 : 0
            color:  control._settingsStyle ?
                        (control.currentIndex === index ? control._popSelected : (itemDel.highlighted ? control._popHover : "transparent")) :
                        (control.currentIndex === index ? qgcPal.buttonHighlight : qgcPal.button)
        }

        highlighted:                control.highlightedIndex === index
    }

    indicator: QGCColoredImage {
        anchors.rightMargin:    control.padding + (control._settingsStyle ? ScreenTools.defaultFontPixelWidth : 0)
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        height:                 ScreenTools.defaultFontPixelWidth
        width:                  height
        source:                 "/qmlimages/arrow-down.png"
        color:                  qgcPal.buttonText
    }

    // The label of the button
    contentItem: QGCLabel {
        id:                         text
        anchors.verticalCenter:     parent.verticalCenter
        anchors.horizontalCenter:   centeredLabel ? parent.horizontalCenter : undefined
        text:                       control.alternateText === "" ? control.currentText : control.alternateText
        font:                       control.font
        color:                      qgcPal.buttonText
    }

    background: Rectangle {
        color:          (_settingsStyle && qgcPal.globalTheme === QGCPalette.Dark) ? "#404040" : qgcPal.button
        border.color:   (_settingsStyle && qgcPal.globalTheme === QGCPalette.Dark) ? "#606060" : qgcPal.buttonBorder
        border.width:   _showBorder ? 1 : 0
        radius:         _settingsStyle ? height / 2 : ScreenTools.buttonBorderRadius
    }

    popup: T.Popup {
        x:                  control.width - _popupWidth
        y:                  control.height + (control._settingsStyle ? 6 : 0)
        width:              _popupWidth
        height:             Math.min(contentItem.implicitHeight + topPadding + bottomPadding, control.Window.height - topMargin - bottomMargin)
        topMargin:          6
        bottomMargin:       6
        padding:            control._settingsStyle ? 6 : 0
        transformOrigin:    Popup.Top

        // Application Settings only: fade + slight scale animation
        enter: Transition {
            enabled: control._settingsStyle
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0.0;  to: 1.0; duration: 150; easing.type: Easing.OutCubic }
                NumberAnimation { property: "scale";   from: 0.92; to: 1.0; duration: 170; easing.type: Easing.OutCubic }
            }
        }
        exit: Transition {
            enabled: control._settingsStyle
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0;  duration: 110 }
                NumberAnimation { property: "scale";   from: 1.0; to: 0.96; duration: 110 }
            }
        }

        contentItem: ListView {
            clip:                   true
            implicitHeight:         contentHeight
            model:                  control.delegateModel
            currentIndex:           control.highlightedIndex
            highlightMoveDuration:  0
            spacing:                control._settingsStyle ? 2 : 0

            Rectangle {
                z:              10
                width:          parent.width
                height:         parent.height
                color:          "transparent"
                border.color:   qgcPal.text
                visible:        !control._settingsStyle
            }

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius:         control._settingsStyle ? 12 : 0
            color:          control._settingsStyle ? control._popBg : qgcPal.window
            border.color:   control._popBorder
            border.width:   control._settingsStyle ? 1 : 0
        }
    }
}
