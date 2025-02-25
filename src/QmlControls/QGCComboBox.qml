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
    leftPadding:    padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding:   padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width)

    property bool   centeredLabel:  false
    property bool   sizeToContents: false
    property string alternateText:  ""

    property real   _popupWidth:    width
    property bool   _onCompleted:   false
    property bool   _showBorder:    qgcPal.globalTheme === QGCPalette.Light

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
        width:  _popupWidth
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
            color:                  control.currentIndex === index ? qgcPal.buttonHighlightText : qgcPal.buttonText
            verticalAlignment:      Text.AlignVCenter
        }

        background: Rectangle {
            color:                  control.currentIndex === index ? qgcPal.buttonHighlight : qgcPal.button
        }

        highlighted:                control.highlightedIndex === index
    }

    indicator: QGCColoredImage {
        anchors.rightMargin:    control.padding
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
        color:          qgcPal.button
        border.color:   qgcPal.buttonBorder
        border.width:   _showBorder ? 1 : 0
        radius:         ScreenTools.buttonBorderRadius
    }

    popup: T.Popup {
        x:              control.width - _popupWidth
        y:              control.height
        width:          _popupWidth
        height:         Math.min(contentItem.implicitHeight, control.Window.height - topMargin - bottomMargin)
        topMargin:      6
        bottomMargin:   6

        contentItem: ListView {
            clip:                   true
            implicitHeight:         contentHeight
            model:                  control.delegateModel
            currentIndex:           control.highlightedIndex
            highlightMoveDuration:  0

            Rectangle {
                z:              10
                width:          parent.width
                height:         parent.height
                color:          "transparent"
                border.color:   qgcPal.text
            }

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: qgcPal.window
        }
    }
}
