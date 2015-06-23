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

#include "AirframeComponentAirframes.h"

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoStandardPlane[] = {
    { "Multiplex Easystar 1/2", 2100 },
    { "Hobbyking Bixler 1/2",   2101 },
    { "3DR Skywalker",          2102 },
    { "Skyhunter (1800 mm)",    2103 },
    { NULL,                     0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoSimulation[] = {
    { "Plane (HilStar, X-Plane)",   1000 },
    { "Plane (Rascal, FlightGear)", 1004 },
    { "Quad X HIL",                 1001 },
    { "Quad + HIL",                 1003 },
    { NULL,                         0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoFlyingWing[] = {
    { "Z-84 Wing Wing (845 mm)",        3033 },
    { "TBS Caipirinha (850 mm)",        3100 },
    { "Bormatec Camflyer Q (800 mm)",   3030 },
    { "FX-61 Phantom FPV (1550 mm)",    3031 },
    { "FX-79 Buffalo (2000 mm)",        3034 },
    { "Skywalker X5 (1180 mm)",         3032 },
    { "Viper v2 (3000 mm)",             3035 },
    { NULL,                             0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoQuadRotorX[] = {
    { "DJI F330 8\" Quad",      4010 },
    { "DJI F450 10\" Quad",     4011 },
    { "X frame Quad UAVCAN",    4012 },
    { "AR.Drone Frame Quad",    4008 },
    { NULL,                     0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoQuadRotorPlus[] = {
    { "Generic 10\" Quad +", 5001 },
    { NULL,                     0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoHexaRotorX[] = {
    { "Standard 10\" Hexa X",   6001 },
    { "Coaxial 10\" Hexa X",    11001 },
    { NULL,                     0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoHexaRotorPlus[] = {
    { "Standard 10\" Hexa",     7001 },
    { NULL,                     0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoOctoRotorX[] = {
    { "Standard 10\" Octo", 8001 },
    { "Coaxial 10\" Octo",  12001 },
    { NULL,                 0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoOctoRotorPlus[] = {
    { "Standard 10\" Octo", 9001 },
    { NULL,                 0 }
};

const AirframeComponentAirframes::AirframeInfo_t AirframeComponentAirframes::_rgAirframeInfoQuadRotorH[] = {
    { "3DR Iris",           10016 },
    { "TBS Discovery",      10015 },
    { "SteadiDrone QU4D",   10017 },
    { NULL,                 0 }
};

const AirframeComponentAirframes::AirframeType_t AirframeComponentAirframes::rgAirframeTypes[] = {
    { "Standard Airplane",  "qrc:/qmlimages/AirframeStandardPlane.png",   AirframeComponentAirframes::_rgAirframeInfoStandardPlane },
    { "Flying Wing",        "qrc:/qmlimages/AirframeFlyingWing.png",      AirframeComponentAirframes::_rgAirframeInfoFlyingWing },
    { "QuadRotor X",        "qrc:/qmlimages/AirframeQuadRotorX.png",      AirframeComponentAirframes::_rgAirframeInfoQuadRotorX },
    { "QuadRotor +",        "qrc:/qmlimages/AirframeQuadRotorPlus.png",   AirframeComponentAirframes::_rgAirframeInfoQuadRotorPlus },
    { "HexaRotor X",        "qrc:/qmlimages/AirframeHexaRotorX.png",      AirframeComponentAirframes::_rgAirframeInfoHexaRotorX },
    { "HexaRotor +",        "qrc:/qmlimages/AirframeHexaRotorPlus.png",   AirframeComponentAirframes::_rgAirframeInfoHexaRotorPlus },
    { "OctoRotor X",        "qrc:/qmlimages/AirframeOctoRotorX.png",      AirframeComponentAirframes::_rgAirframeInfoOctoRotorX },
    { "OctoRotor +",        "qrc:/qmlimages/AirframeOctoRotorPlus.png",   AirframeComponentAirframes::_rgAirframeInfoOctoRotorPlus },
    { "QuadRotor H",        "qrc:/qmlimages/AirframeQuadRotorH.png",      AirframeComponentAirframes::_rgAirframeInfoQuadRotorH },
    { "Simulation",         "qrc:/qmlimages/AirframeSimulation.png",      AirframeComponentAirframes::_rgAirframeInfoSimulation },
    { NULL,                 NULL,                                   NULL }
};
