/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs         1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Item {
    id: stepper
    property bool enabled: true
    property alias fact: factInput.fact
    property alias showHelp: factInput.showHelp
    property alias showUnits: factInput.showUnits

    property string _valueText:          fact ? fact.valueString : "N/A"
    property double _factValue:          Number(_valueText)

    property double minimumValue:       !fact || isNaN(fact.min) ? 0 : fact.min
    property double maximumValue:       !fact || isNaN(fact.max) ? 1 : fact.max
    property double stepSize:           !fact || isNaN(fact.increment) ? 0 : fact.increment
    property double stepRatio:          0.1

    implicitHeight: 50
    implicitWidth: implicitHeight * 4

    signal validationError(string text)
    signal helpClicked

    Component.onCompleted: {
        factInput.validationError.connect(validationError)
        factInput.helpClicked.connect(helpClicked)
   }

    function incrementWithScale(multiplier) {
        var step = stepSize;
        if (step == 0) {
            step = Math.abs(stepRatio * _factValue);
        }

        factInput.setFactValue(_factValue + (step * multiplier));
    }

    Timer {
        id: factValueChangeDelay

        interval: 250
        repeat: false
        running: false

        onTriggered: {
            factInput.setFactValueImpl(_factValue)
        }
    }

    Row {
        anchors.fill:   parent

        QGCButton {
            id: minusButton
            width : parent.width / 4
            height: parent.height
            text: "-"
            enabled: _factValue > minimumValue

            onClicked: {
                incrementWithScale(-1)
            }
        }

        FactTextField {
            id: factInput
            width : parent.width / 2
            height: parent.height

            horizontalAlignment: TextInput.AlignHCenter

            text:       _valueText

            fact: _fact

            function setFactValue(newValue) {
                if (newValue < fact.min) {
                    newValue = fact.min;
                }
                if (newValue > fact.max) {
                    newValue = fact.max;
                }

                factValueChangeDelay.stop()
                _valueText = newValue.toLocaleString(Qt.locale(), 'f', fact.decimalPlaces)

                factValueChangeDelay.start()
            }

            onValidationError: {
                // remove default handler that shows dialog
            }

            onHelpClicked: {
                // remove default handler that shows dialog
            }
        }

        QGCButton {
            id: plusButton
            width : parent.width / 4
            height: parent.height
            text: "+"
            enabled: _factValue < maximumValue

            onClicked: {
                incrementWithScale(1);
            }
        }
    }
}
