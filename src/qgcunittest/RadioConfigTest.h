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

#ifndef RadioConfigTest_H
#define RadioConfigTest_H

#include "UnitTest.h"
#include "MockLink.h"
#include "MultiSignalSpy.h"
#include "RadioComponentController.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"
#include "QGCQmlWidgetHolder.h"

/// @file
///     @brief Radio Config unit test
///
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(RadioConfigTestLog)

class RadioConfigTest : public UnitTest
{
    Q_OBJECT
    
public:
    RadioConfigTest(void);
    
private slots:
    void cleanup(void);
    
    void _fullCalibration_px4_test(void);
    void _fullCalibration_apm_test(void);

private:
    /// @brief Modes to run worker functions
    enum TestMode {
        testModeStandalone,     ///< Perform standalone test of calibration state
        testModePrerequisite,   ///< Setup prequisites for subsequent calibration state
        testModeFullSequence,   ///< Run as full calibration sequence
    };
    
    enum MoveToDirection {
        moveToMax,
        moveToMin,
        moveToCenter,
    };
    
    enum {
        validateMinMaxMask =    1 << 0,
        validateTrimsMask =     1 << 1,
        validateReversedMask =  1 << 2,
        validateMappingMask =   1 << 3,
        validateAllMask = 0xFFFF
    };

    struct ChannelSettings {
        int function;
        int rcMin;
        int rcMax;
        int rcTrim;
        int reversed;
    };

    void _init(MAV_AUTOPILOT firmwareType);
    void _channelHomePosition(void);
    void _minRCChannels(void);
    void _beginCalibration(void);
    void _stickMoveWaitForSettle(int channel, int value);
    void _stickMoveAutoStep(const char* functionStr, enum RadioComponentController::rcCalFunctions function, enum MoveToDirection direction, bool identifyStep);
    void _switchMinMaxStep(void);
    void _switchSelectAutoStep(const char* functionStr, RadioComponentController::rcCalFunctions function);
    bool _px4Vehicle(void) const;
    const struct RadioComponentController::FunctionInfo* _functionInfo(void) const;
    const struct ChannelSettings* _channelSettings(void) const;
    const struct ChannelSettings* _channelSettingsValidate(void) const;
    void _fullCalibrationWorker(MAV_AUTOPILOT firmwareType);
    int _chanMax(void) const;
    
    void _validateParameters(void);
    
    AutoPilotPlugin*    _autopilot;
    
    QGCQmlWidgetHolder* _calWidget;
    
    enum {
        nextButtonMask =        1 << 0,
        cancelButtonMask =      1 << 1,
        skipButtonMask =        1 << 2
    };
    
    const char*         _rgSignals[1];
    MultiSignalSpy*     _multiSpyNextButtonMessageBox;
    
    // When settings values into min/max/trim we set them slightly different than the defaults so that
    // we can distinguish between the two values.
    static const int _testMinValue;
    static const int _testMaxValue;
    static const int _testCenterValue;
    
    static const int _stickSettleWait;
	
    static const struct ChannelSettings _rgChannelSettingsPX4[RadioComponentController::_chanMaxPX4];
    static const struct ChannelSettings _rgChannelSettingsAPM[RadioComponentController::_chanMaxAPM];
    static const struct ChannelSettings _rgChannelSettingsValidatePX4[RadioComponentController::_chanMaxPX4];
    static const struct ChannelSettings _rgChannelSettingsValidateAPM[RadioComponentController::_chanMaxAPM];

	int _rgFunctionChannelMap[RadioComponentController::rcCalFunctionMax];
    
    RadioComponentController*   _controller;
};

#endif
