/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FlightMap

/// UTMSPGeoFence map visuals
Item {
    id: _root
    z: QGroundControl.zOrderMapItems

    property var    map
    property var    myGeoFenceController
    property var    currentMissionItems
    property bool   interactive:            false   ///< true: user can interact with items
    property bool   planView:               false   ///< true: visuals showing in plan view
    property var    homePosition
    property var    _breachReturnPointComponent
    property var    _breachReturnDragComponent
    property var    _paramCircleFenceComponent
    property var    _utmspPolygon:              myGeoFenceController.polygons
    property color  _borderColor:               "orange"
    property int    _borderWidthInclusion:      2
    property int    _borderWidthExclusion:      0
    property color  _interiorColorExclusion:    "orange"
    property color  _interiorColorInclusion:    "transparent"
    property real   _interiorOpacityExclusion:  0.2 * opacity
    property real   _interiorOpacityInclusion:  1 * opacity
    property bool    resetCheck

    Instantiator {
        model: _utmspPolygon

        delegate : UTMSPMapPolygonVisuals {
            parent:             _root
            mapControl:         map
            mapPolygon:         object
            borderWidth:        object.inclusion ? _borderWidthExclusion : _borderWidthInclusion
            borderColor:        _borderColor
            interiorColor:      object.inclusion ? _interiorColorExclusion : _interiorColorInclusion
            interiorOpacity:    object.inclusion ? _interiorOpacityExclusion : _interiorOpacityInclusion
            interactive:        _root.interactive && mapPolygon && mapPolygon.interactive
            resetChecked:       resetCheck
            missionItems:       currentMissionItems
        }
    }
}
