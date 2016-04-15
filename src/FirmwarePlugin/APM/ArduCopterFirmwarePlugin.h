/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef ArduCopterFirmwarePlugin_H
#define ArduCopterFirmwarePlugin_H

#include "APMFirmwarePlugin.h"

class APMCopterMode : public APMCustomMode
{
public:
    enum Mode {
        STABILIZE   = 0,   // hold level position
        ACRO        = 1,   // rate control
        ALT_HOLD    = 2,   // AUTO control
        AUTO        = 3,   // AUTO control
        GUIDED      = 4,   // AUTO control
        LOITER      = 5,   // Hold a single location
        RTL         = 6,   // AUTO control
        CIRCLE      = 7,   // AUTO control
        POSITION    = 8,   // AUTO control
        LAND        = 9,   // AUTO control
        OF_LOITER   = 10,  // Hold a single location using optical flow
                           // sensor
        DRIFT       = 11,  // Drift 'Car Like' mode
        RESERVED_12 = 12,  // RESERVED FOR FUTURE USE
        SPORT       = 13,  // [TODO] Verify this is correct.
        FLIP        = 14,
        AUTOTUNE    = 15,
        POS_HOLD    = 16, // HYBRID LOITER.
        BRAKE       = 17
    };
    static const int modeCount = 18;

    APMCopterMode(uint32_t mode, bool settable);
};

class ArduCopterFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT
    
public:
    ArduCopterFirmwarePlugin(void);

    // Overrides from FirmwarePlugin
    bool isCapable(FirmwareCapabilities capabilities) final;
    bool isPaused(const Vehicle* vehicle) const final;
    void setGuidedMode(Vehicle* vehicle, bool guidedMode) final;
    void pauseVehicle(Vehicle* vehicle) final;
    void guidedModeRTL(Vehicle* vehicle) final;
    void guidedModeLand(Vehicle* vehicle) final;
    void guidedModeTakeoff(Vehicle* vehicle, double altitudeRel) final;
    void guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord) final;
    void guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel) final;
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const final { return _remapParamName; }
    virtual int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
};

#endif
