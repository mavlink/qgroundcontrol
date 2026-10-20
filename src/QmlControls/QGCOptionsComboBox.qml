import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ComboBox {
    id:         control
    padding:    ScreenTools.comboBoxPadding
    topPadding:     ScreenTools.controlVerticalPadding
    bottomPadding:  ScreenTools.controlVerticalPadding

    property string labelText:  qsTr("Options")

    signal itemClicked(int index)

    property var    _controlQGCPal: QGCPalette { colorGroupEnabled: enabled }
    property bool   _flashChecked
    property string _flashText
    property bool   _showFlash:     false

    Component.onCompleted: indicator.color = Qt.binding(function() { return _controlQGCPal.textPrimary })

    background: Rectangle {
        implicitWidth:                  ScreenTools.implicitComboBoxWidth
        implicitHeight:                 ScreenTools.implicitComboBoxHeight
        color:                          _controlQGCPal.windowInset
        border.width:                   1
        border.color:                   control.enabled && (control.hovered || control.pressed || control.popup.visible) ? _controlQGCPal.accent : _controlQGCPal.borderSubtle
        radius:                         ScreenTools.defaultBorderRadius
    }

    /*! Adding the Combobox list item to the theme.  */

    delegate: ItemDelegate {
        implicitHeight: modelData.visible ?
                            (Math.max(background ? background.implicitHeight : 0, Math.max(contentItem.implicitHeight, indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)) :
                            0
        width:      control.width
        checkable:  true
        enabled:    modelData.enabled
        text:       modelData.text

        property var _checkedValue:     1
        property var _uncheckedValue:   0
        property var _itemQGCPal:       QGCPalette { colorGroupEnabled: enabled }
        property var _control:          control

        Binding on checked { value: modelData.fact ?
                                         (modelData.fact.typeIsBool ? (modelData.fact.value === false ? Qt.Unchecked : Qt.Checked) : (modelData.fact.value === 0 ? Qt.Unchecked : Qt.Checked)) :
                                         modelData.checked }

        contentItem: RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            Rectangle {
                height:         ScreenTools.defaultFontPixelHeight
                width:          height
                border.color:   _itemQGCPal.textPrimary
                border.width:   1
                color:          _itemQGCPal.windowCard

                QGCColoredImage {
                    anchors.centerIn:   parent
                    width:              parent.width * 0.75
                    height:             width
                    source:             "/qmlimages/checkbox-check.svg"
                    color:              _itemQGCPal.textPrimary
                    mipmap:             true
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    visible:            checked
                }
            }

            Text {
                text:   modelData.text
                color:  _itemQGCPal.textPrimary
            }

        }

        background: Rectangle {
            color: hovered ? _controlQGCPal.windowCard : _controlQGCPal.windowPanel
        }

        onClicked: {
            if (modelData.fact) {
                modelData.fact.value = (checked ? _checkedValue : _uncheckedValue)
            } else {
                itemClicked(index)
            }
            _control._flashChecked = checked
            _control._flashText = text
            _control._showFlash = true
            _control.popup.close()
        }
    }

    /*! This defines the label of the button.  */
    contentItem: Item {
        implicitWidth:                  _showFlash ? flash.implicitWidth : text.implicitWidth
        implicitHeight:                 _showFlash ? flash.implicitHeight : text.implicitHeight

        QGCLabel {
            id:                         text
            anchors.verticalCenter:     parent.verticalCenter
            text:                       labelText
            color:                      _controlQGCPal.textPrimary
            visible:                    !_showFlash
        }

        RowLayout {
            id:                     flash
            anchors.verticalCenter: parent.verticalCenter
            spacing:                ScreenTools.defaultFontPixelWidth
            visible:                _showFlash

            onVisibleChanged: {
                if (visible) {
                    flashTimer.restart()
                }
            }

            Timer {
                id:             flashTimer
                interval:       1500
                repeat:         false
                running:        false
                onTriggered:    _showFlash = false
            }

            Rectangle {
                height:         ScreenTools.defaultFontPixelHeight
                width:          height
                border.color:   _controlQGCPal.textPrimary
                border.width:   1
                color:          _controlQGCPal.window

                QGCColoredImage {
                    anchors.centerIn:   parent
                    width:              parent.width * 0.75
                    height:             width
                    source:             "/qmlimages/checkbox-check.svg"
                    color:              _controlQGCPal.textPrimary
                    mipmap:             true
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    visible:            _flashChecked
                }
            }

            Text {
                text:   _flashText
                color:  _controlQGCPal.textPrimary
            }

        }
    }
}
