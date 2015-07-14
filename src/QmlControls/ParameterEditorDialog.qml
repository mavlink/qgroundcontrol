/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

QGCViewDialog {
    property Fact   fact
    property bool   validate:       false
    property string validateValue

    ParameterEditorController { id: controller; factPanel: parent }

    function accept() {
        var errorString = fact.validate(valueField.text, forceSave.checked)
        if (errorString == "") {
            fact.value = valueField.text
            fact.valueChanged(fact.value)
            hideDialog()
        } else {
            validationError.text = errorString
            forceSave.visible = true
        }
    }

    Component.onCompleted: {
        if (validate) {
            validationError.text = fact.validate(validateValue, false /* convertOnly */)
            forceSave.visible = true
        }
        valueField.forceActiveFocus();
    }

    Column {
        spacing:        defaultTextHeight
        anchors.left:   parent.left
        anchors.right:  parent.right

        QGCLabel {
            width:      parent.width
            wrapMode:   Text.WordWrap
            text:       fact.shortDescription ? fact.shortDescription : "Description missing"
        }

        QGCLabel {
            width:      parent.width
            wrapMode:   Text.WordWrap
            visible:    fact.longDescription
            text:       fact.longDescription
        }

        QGCTextField {
            id:     valueField
            text:   validate ? validateValue : fact.valueString

            onAccepted: accept()

            Keys.onReleased: {
                if (event.key == Qt.Key_Escape) {
                    reject()
                }
            }
        }

        QGCLabel { text: fact.name }

        Row {
            spacing: defaultTextWidth

            QGCLabel { text: "Units:" }
            QGCLabel { text: fact.units ? fact.units : "none" }
        }

        Row {
            spacing: defaultTextWidth
            visible: !fact.minIsDefaultForType

            QGCLabel { text: "Minimum value:" }
            QGCLabel { text: fact.min }
        }

        Row {
            spacing: defaultTextWidth
            visible: !fact.maxIsDefaultForType

            QGCLabel { text: "Maximum value:" }
            QGCLabel { text: fact.max }
        }

        Row {
            spacing: defaultTextWidth

            QGCLabel { text: "Default value:" }
            QGCLabel { text: fact.defaultValueAvailable ? fact.defaultValue : "none" }
        }

        QGCLabel {
            width:      parent.width
            wrapMode:   Text.WordWrap
            text:       "Warning: Modifying parameters while vehicle is in flight can lead to vehicle instability and possible vehicle loss. " +
                            "Make sure you know what you are doing and double-check your values before Save!"
        }

        QGCLabel {
            id:         validationError
            width:      parent.width
            wrapMode:   Text.WordWrap
            color:      "yellow"     
        }

        QGCCheckBox {
            id:         forceSave
            visible:    false
            text:       "Force save (dangerous!)"
        }
    } // Column - Fact information


    QGCButton {
        id:                     bottomButton
        anchors.rightMargin:    defaultTextWidth
        anchors.right:          rcButton.left
        anchors.bottom:         parent.bottom
        visible:                fact.defaultValueAvailable
        text:                   "Reset to default"

        onClicked: {
            fact.value = fact.defaultValue
            fact.valueChanged(fact.value)
            hideDialog()
        }
    }

    QGCButton {
        id:             rcButton
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        text:           "Set RC to Param..."
        visible:        !validate
        onClicked:      controller.setRCToParam(fact.name)
    }
} // QGCViewDialog
