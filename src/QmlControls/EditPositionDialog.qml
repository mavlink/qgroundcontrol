import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

QGCPopupDialog {
    id:         root
    title:      qsTr("Edit Position")
    buttons:    Dialog.Close

    property alias coordinate:      controller.coordinate
    property var    altitudeFact:   null
    property int    altitudeFrame:  QGroundControl.AltitudeFrameNone

    property real _margin:          ScreenTools.defaultFontPixelWidth / 2
    property real _textFieldWidth:  ScreenTools.defaultFontPixelWidth * 20
    property bool _showGeographic:  coordinateSystemCombo.comboBox.currentIndex === 0
    property bool _showUTM:         coordinateSystemCombo.comboBox.currentIndex === 1
    property bool _showMGRS:        coordinateSystemCombo.comboBox.currentIndex === 2
    property bool _showVehicle:     coordinateSystemCombo.comboBox.currentIndex === 3
    property bool _supportsAltitude: altitudeFact !== null

    function _isFiniteAltitude(value) {
        const numericValue = Number(value)
        return !isNaN(numericValue) && isFinite(numericValue)
    }

    TransformPositionController {
        id: controller

        Component.onCompleted: initValues()
    }

    ColumnLayout {
        spacing: _margin

        LabelledComboBox {
            id:                 coordinateSystemCombo
            Layout.fillWidth:   true
            label:              qsTr("Coordinate System")
            model:              globals.activeVehicle ?
                                    [ qsTr("Geographic"), qsTr("Universal Transverse Mercator"), qsTr("Military Grid Reference"), qsTr("Vehicle Position") ] :
                                    [ qsTr("Geographic"), qsTr("Universal Transverse Mercator"), qsTr("Military Grid Reference") ]
        }

        LabelledFactTextField {
            label:              qsTr("Latitude")
            fact:               controller.latitude
            textFieldPreferredWidth: _textFieldWidth
            Layout.fillWidth:   true
            visible:            _showGeographic
        }

        LabelledFactTextField {
            label:              qsTr("Longitude")
            fact:               controller.longitude
            textFieldPreferredWidth: _textFieldWidth
            Layout.fillWidth:   true
            visible:            _showGeographic
        }

        LabelledFactTextField {
            label:              qsTr("Zone")
            fact:               controller.zone
            textFieldPreferredWidth: _textFieldWidth
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        LabelledFactComboBox {
            label:              qsTr("Hemisphere")
            fact:               controller.hemisphere
            indexModel:         false
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        LabelledFactTextField {
            label:              qsTr("Easting")
            fact:               controller.easting
            textFieldPreferredWidth: _textFieldWidth
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        LabelledFactTextField {
            label:              qsTr("Northing")
            fact:               controller.northing
            textFieldPreferredWidth: _textFieldWidth
            Layout.fillWidth:   true
            visible:            _showUTM
        }

        LabelledFactTextField {
            label:              qsTr("MGRS")
            fact:               controller.mgrs
            visible:            _showMGRS
            textFieldPreferredWidth: _textFieldWidth
            Layout.fillWidth:   true
        }

        LabelledLabel {
            label:              qsTr("Latitude")
            labelText:          globals.activeVehicle ? globals.activeVehicle.coordinate.latitude.toFixed(7) : ""
            Layout.fillWidth:   true
            visible:            _showVehicle
        }

        LabelledLabel {
            label:              qsTr("Longitude")
            labelText:          globals.activeVehicle ? globals.activeVehicle.coordinate.longitude.toFixed(7) : ""
            Layout.fillWidth:   true
            visible:            _showVehicle
        }

        LabelledLabel {
            label:              qsTr("Alt (AMSL)")
            labelText:          globals.activeVehicle ? globals.activeVehicle.altitudeAMSL.valueString + " " + globals.activeVehicle.altitudeAMSL.units : ""
            Layout.fillWidth:   true
            visible:            _showVehicle && _supportsAltitude && altitudeFrame === QGroundControl.AltitudeFrameAbsolute
        }

        LabelledLabel {
            label:              qsTr("Alt (Rel)")
            labelText:          globals.activeVehicle ? globals.activeVehicle.altitudeRelative.valueString + " " + globals.activeVehicle.altitudeRelative.units : ""
            Layout.fillWidth:   true
            visible:            _showVehicle && _supportsAltitude && altitudeFrame === QGroundControl.AltitudeFrameRelative
        }

        LabelledLabel {
            label:              qsTr("Alt (AGL)")
            labelText:          globals.activeVehicle ? globals.activeVehicle.altitudeAboveTerr.valueString + " " + globals.activeVehicle.altitudeAboveTerr.units : ""
            Layout.fillWidth:   true
            visible:            _showVehicle && _supportsAltitude && (altitudeFrame === QGroundControl.AltitudeFrameTerrain || altitudeFrame === QGroundControl.AltitudeFrameCalcAboveTerrain)
        }

        QGCCheckBox {
            id:         setPositionCheckBox
            text:       qsTr("Set position from vehicle")
            checked:    true
            visible:    _showVehicle
        }

        QGCCheckBox {
            id:         setAltitudeCheckBox
            text:       qsTr("Set altitude from vehicle")
            checked:    false
            visible:    _showVehicle && _supportsAltitude
        }

        LabelledButton {
            label:               qsTr("Set position")
            buttonText:          qsTr("Move")
            onClicked: {
                if (_showGeographic)
                    controller.setFromGeo()
                else if (_showUTM)
                    controller.setFromUTM()
                else if (_showMGRS)
                    controller.setFromMGRS()
                else if (_showVehicle) {
                    if (setPositionCheckBox.checked)
                        controller.setFromVehicle()
                    if (setAltitudeCheckBox.checked && _supportsAltitude && globals.activeVehicle) {
                        let sourceAltitude = NaN

                        if (altitudeFrame === QGroundControl.AltitudeFrameRelative)
                            sourceAltitude = globals.activeVehicle.altitudeRelative.rawValue
                        else if (altitudeFrame === QGroundControl.AltitudeFrameAbsolute)
                            sourceAltitude = globals.activeVehicle.altitudeAMSL.rawValue
                        else if (altitudeFrame === QGroundControl.AltitudeFrameTerrain || altitudeFrame === QGroundControl.AltitudeFrameCalcAboveTerrain)
                            sourceAltitude = globals.activeVehicle.altitudeAboveTerr.rawValue

                        if (_isFiniteAltitude(sourceAltitude))
                            altitudeFact.rawValue = sourceAltitude
                    }
                }
                root.close()
            }
        }
    }
}
