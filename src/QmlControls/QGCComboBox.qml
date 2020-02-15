/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.11
import QtQuick.Window           2.3
import QtQuick.Controls         2.4
import QtQuick.Controls.impl    2.4
import QtQuick.Templates        2.4 as T

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0

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
    leftPadding:    padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding:   padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width)

    property bool   centeredLabel:  false
    property bool   sizeToContents: false
    property string alternateText:  ""

    property var    _qgcPal:            QGCPalette { colorGroupEnabled: enabled }
    property real   _largestTextWidth:  0
    property real   _popupWidth:        sizeToContents ? _largestTextWidth + itemDelegateMetrics.leftPadding + itemDelegateMetrics.rightPadding : control.width
    property bool   _onCompleted:       false

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

    function _adjustSizeToContents() {
        if (_onCompleted && sizeToContents) {
            _largestTextWidth = 0
            for (var i = 0; i < model.length; i++){
                textMetrics.text = model[i]
                _largestTextWidth = Math.max(textMetrics.width, _largestTextWidth)
            }
        }
    }

    onModelChanged: _adjustSizeToContents()

    Component.onCompleted: {
        _onCompleted = true
        _adjustSizeToContents()
    }

    // The items in the popup
    delegate: ItemDelegate {
        width:  _popupWidth
        height: Math.round(popupItemMetrics.height * 1.75)

        property string _text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData

        TextMetrics {
            id:             popupItemMetrics
            font:           control.font
            text:           _text
        }

        contentItem: Text {
            text:                   _text
            font:                   control.font
            color:                  control.currentIndex === index ? _qgcPal.buttonHighlightText : _qgcPal.buttonText
            verticalAlignment:      Text.AlignVCenter
        }

        background: Rectangle {
            color:                  control.currentIndex === index ? _qgcPal.buttonHighlight : _qgcPal.button
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
        color:                  _qgcPal.text
    }

    // The label of the button
    contentItem: Item {
        implicitWidth:                  text.implicitWidth
        implicitHeight:                 text.implicitHeight

        QGCLabel {
            id:                         text
            anchors.verticalCenter:     parent.verticalCenter
            anchors.horizontalCenter:   centeredLabel ? parent.horizontalCenter : undefined
            text:                       control.alternateText === "" ? control.currentText : control.alternateText
            font:                       control.font
            color:                      _qgcPal.text
        }
    }

    background: Rectangle {
        implicitWidth:  ScreenTools.implicitComboBoxWidth
        implicitHeight: ScreenTools.implicitComboBoxHeight
        color:          _qgcPal.window
        border.width:   enabled ? 1 : 0
        border.color:   "#999"
    }

    popup: T.Popup {
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
                border.color:   _qgcPal.text
            }

            T.ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: control.palette.window
        }
    }
}
