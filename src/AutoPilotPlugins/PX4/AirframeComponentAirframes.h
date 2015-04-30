/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef AIRFRAMECOMPONENTAIRFRAMES_H
#define AIRFRAMECOMPONENTAIRFRAMES_H

#include <QObject>
#include <QQuickItem>
#include <QList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// MVC Controller for AirframeComponent.qml.
class AirframeComponentAirframes
{
public:
    typedef struct {
        const char* name;
        int         autostartId;
    } AirframeInfo_t;
    
    typedef struct {
        const char* name;
        const char* imageResource;
        const AirframeInfo_t* rgAirframeInfo;
    } AirframeType_t;
    
public:
    static const AirframeType_t rgAirframeTypes[];
    
private:
    static const AirframeInfo_t _rgAirframeInfoStandardPlane[];
    static const AirframeInfo_t _rgAirframeInfoFlyingWing[];
    static const AirframeInfo_t _rgAirframeInfoQuadRotorX[];
    static const AirframeInfo_t _rgAirframeInfoQuadRotorPlus[];
    static const AirframeInfo_t _rgAirframeInfoOctoRotorX[];
    static const AirframeInfo_t _rgAirframeInfoOctoRotorPlus[];
    static const AirframeInfo_t _rgAirframeInfoHexaRotorX[];
    static const AirframeInfo_t _rgAirframeInfoHexaRotorPlus[];
    static const AirframeInfo_t _rgAirframeInfoQuadRotorH[];
    static const AirframeInfo_t _rgAirframeInfoSimulation[];
};

#endif
