/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Compass Widget
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick              2.3
import QtGraphicalEffects   1.0

import QGroundControl              1.0
import QGroundControl.Controls     1.0
import QGroundControl.ScreenTools  1.0
import QGroundControl.Vehicle      1.0
import QGroundControl.Palette      1.0

Item {
    id:     root
    width:  size
    height: size

    property real size:     _defaultSize
    property var  vehicle:  null

    property real _defaultSize:     ScreenTools.defaultFontPixelHeight * (10)
    property real _sizeRatio:       ScreenTools.isTinyScreen ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property int  _fontSize:        ScreenTools.defaultFontPointSize * _sizeRatio
    property real _heading:         vehicle ? vehicle.heading.rawValue : 0
    property real _headingToHome:   vehicle ? vehicle.headingToHome.rawValue : 0
    property real _groundSpeed:     vehicle ? vehicle.groundSpeed.rawValue : 0
    property real _headingToNextWP: vehicle ? vehicle.headingToNextWP.rawValue : 0
    property real _courseOverGround:activeVehicle ? activeVehicle.gps.courseOverGround.rawValue : 0

    property bool usedByMultipleVehicleList:  false

    function isCOGAngleOK(){
        if(_groundSpeed < 0.5){
            return false
        }
        else{
            return vehicle && _showAdditionalIndicatorsCompass
        }
    }

    function isHeadingHomeOK(){
        return vehicle && _showAdditionalIndicatorsCompass && !isNaN(_headingToHome)
    }

    function isHeadingToNextWPOK(){
        return vehicle && _showAdditionalIndicatorsCompass && !isNaN(_headingToNextWP)
    }

    function isNoseUpLocked(){
        return _lockNoseUpCompass
    }

    readonly property bool _showAdditionalIndicatorsCompass:     QGroundControl.settingsManager.flyViewSettings.showAdditionalIndicatorsCompass.value && !usedByMultipleVehicleList
    readonly property bool _lockNoseUpCompass:        QGroundControl.settingsManager.flyViewSettings.lockNoseUpCompass.value

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Rectangle {
        id:             borderRect
        anchors.fill:   parent
        radius:         width / 2
        color:          qgcPal.window
        border.color:   qgcPal.text
        border.width:   1
    }

    Item {
        id:             instrument
        anchors.fill:   parent
        visible:        false


        Image {
            id:                 cOGPointer
            source:             isCOGAngleOK() ? "/qmlimages/cOGPointer.svg" : ""
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height

            transform: Rotation {
                property var _angle:isNoseUpLocked()?_courseOverGround-_heading:_courseOverGround
                origin.x:       cOGPointer.width  / 2
                origin.y:       cOGPointer.height / 2
                angle:         _angle
            }
        }

        Image {
            id:                 nextWPPointer
            source:             isHeadingToNextWPOK() ? "/qmlimages/compassDottedLine.svg":"" 
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height

            transform: Rotation {
                property var _angle: isNoseUpLocked()?_headingToNextWP-_heading:_headingToNextWP
                origin.x:       cOGPointer.width  / 2
                origin.y:       cOGPointer.height / 2
                angle:         _angle
            }
        }

        Image {
            id:                     homePointer
            width:                  size * 0.1
            source:                 isHeadingHomeOK()  ? "/qmlimages/Home.svg" : ""
            mipmap:                 true
            fillMode:               Image.PreserveAspectFit
            anchors.centerIn:   	parent
            sourceSize.width:       width

            transform: Translate {
                property double _angle: isNoseUpLocked()?-_heading+_headingToHome:_headingToHome
                x: size/2.3 * Math.sin((_angle)*(3.14/180))
                y: - size/2.3 * Math.cos((_angle)*(3.14/180))
            }
        }

        Image {
            id:                 pointer
            width:              size * 0.65
            source:             vehicle ? vehicle.vehicleImageCompass : ""
            mipmap:             true
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
            anchors.centerIn:   parent
            transform: Rotation {
                origin.x:       pointer.width  / 2
                origin.y:       pointer.height / 2
                angle:          isNoseUpLocked()?0:_heading
            }
        }


        QGCColoredImage {
            id:                 compassDial
            source:             "/qmlimages/compassInstrumentDial.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
            color:              qgcPal.text
            transform: Rotation {
                origin.x:       compassDial.width  / 2
                origin.y:       compassDial.height / 2
                angle:          isNoseUpLocked()?-_heading:0
            }
        }


        Rectangle {
            anchors.centerIn:   parent
            width:              size * 0.35
            height:             size * 0.2
            border.color:       qgcPal.text
            color:              qgcPal.window
            opacity:            0.65

            QGCLabel {
                text:               _headingString3
                font.family:        vehicle ? ScreenTools.demiboldFontFamily : ScreenTools.normalFontFamily
                font.pointSize:     _fontSize < 8 ? 8 : _fontSize;
                color:              qgcPal.text
                anchors.centerIn:   parent

                property string _headingString: vehicle ? _heading.toFixed(0) : "OFF"
                property string _headingString2: _headingString.length === 1 ? "0" + _headingString : _headingString
                property string _headingString3: _headingString2.length === 2 ? "0" + _headingString2 : _headingString2
            }
        }
    }

    Rectangle {
        id:             mask
        anchors.fill:   instrument
        radius:         width / 2
        color:          "black"
        visible:        false
    }

    OpacityMask {
        anchors.fill:   instrument
        source:         instrument
        maskSource:     mask
    }

}
