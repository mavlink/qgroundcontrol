/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12

/// Object which exposes vehicle distanceSensors FactGroup information for use in UI
QtObject {
    property var    vehicle

    property bool   telemetryAvailable: vehicle && vehicle.distanceSensors.telemetryAvailable

    signal rotationValueChanged ///< Signalled when any available rotation value changes

    property real   rotationNoneValue:      _distanceSensors ? _distanceSensors.rotationNone.value : NaN
    property real   rotationYaw45Value:     _distanceSensors ? _distanceSensors.rotationYaw45.value : NaN
    property real   rotationYaw90Value:     _distanceSensors ? _distanceSensors.rotationYaw90.value : NaN
    property real   rotationYaw135Value:    _distanceSensors ? _distanceSensors.rotationYaw135.value : NaN
    property real   rotationYaw180Value:    _distanceSensors ? _distanceSensors.rotationYaw180.value : NaN
    property real   rotationYaw225Value:    _distanceSensors ? _distanceSensors.rotationYaw225.value : NaN
    property real   rotationYaw270Value:    _distanceSensors ? _distanceSensors.rotationYaw270.value : NaN
    property real   rotationYaw315Value:    _distanceSensors ? _distanceSensors.rotationYaw315.value : NaN
    property real   maxDistance:            _distanceSensors ? _distanceSensors.maxDistance.value : NaN

    property string rotationNoneValueString:    _distanceSensors ? _distanceSensors.rotationNone.valueString : _noValueStr
    property string rotationYaw45ValueString:   _distanceSensors ? _distanceSensors.rotationYaw45.valueString : _noValueStr
    property string rotationYaw90ValueString:   _distanceSensors ? _distanceSensors.rotationYaw90.valueString : _noValueStr
    property string rotationYaw135ValueString:  _distanceSensors ? _distanceSensors.rotationYaw135.valueString : _noValueStr
    property string rotationYaw180ValueString:  _distanceSensors ? _distanceSensors.rotationYaw180.valueString : _noValueStr
    property string rotationYaw225ValueString:  _distanceSensors ? _distanceSensors.rotationYaw225.valueString : _noValueStr
    property string rotationYaw270ValueString:  _distanceSensors ? _distanceSensors.rotationYaw270.valueString : _noValueStr
    property string rotationYaw315ValueString:  _distanceSensors ? _distanceSensors.rotationYaw315.valueString : _noValueStr

    property var    rgRotationValues:           [ rotationNoneValue, rotationYaw45Value, rotationYaw90Value, rotationYaw135Value, rotationYaw180Value, rotationYaw225Value, rotationYaw270Value, rotationYaw315Value ]
    property var    rgRotationValueStrings:     [ rotationNoneValueString, rotationYaw45ValueString, rotationYaw90ValueString, rotationYaw135ValueString, rotationYaw180ValueString, rotationYaw225ValueString, rotationYaw270ValueString, rotationYaw315ValueString ]

    property var    _distanceSensors:       vehicle ? vehicle.distanceSensors : null
    property string _noValueStr:            qsTr("--.--")

    onRotationNoneValueChanged:     rotationValueChanged()
    onRotationYaw45ValueChanged:    rotationValueChanged()
    onRotationYaw90ValueChanged:    rotationValueChanged()
    onRotationYaw135ValueChanged:   rotationValueChanged()
    onRotationYaw180ValueChanged:   rotationValueChanged()
    onRotationYaw225ValueChanged:   rotationValueChanged()
    onRotationYaw270ValueChanged:   rotationValueChanged()
    onRotationYaw315ValueChanged:   rotationValueChanged()
}

