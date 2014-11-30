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

#ifndef PX4RCCALIBRATIONTEST_H
#define PX4RCCALIBRATIONTEST_H

#include "UnitTest.h"
#include "MockUASManager.h"
#include "MockUAS.h"
#include "MultiSignalSpy.h"
#include "px4_configuration/PX4RCCalibration.h"

/// @file
///     @brief PX4RCCalibration Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

///     @brief PX4RCCalibration Widget unit test
class PX4RCCalibrationTest : public UnitTest
{
    Q_OBJECT
    
public:
    PX4RCCalibrationTest(void);
    
private slots:
    void initTestCase(void);
    void init(void);
    void cleanup(void);
    
    void _setUAS_test(void);
    void _minRCChannels_test(void);
    void _fullCalibration_test(void);
    
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
    
    void _channelHomePosition(void);
    void _minRCChannels(void);
    void _beginCalibration(void);
    void _stickMoveWaitForSettle(int channel, int value);
    void _stickMoveAutoStep(const char* functionStr, enum PX4RCCalibration::rcCalFunctions function, enum MoveToDirection direction, bool identifyStep);
    void _switchMinMaxStep(void);
    void _flapsDetectStep(void);
    void _switchSelectAutoStep(const char* functionStr, PX4RCCalibration::rcCalFunctions function);
    
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
    
    void _validateParameters(void);
    
    MockUASManager*     _mockUASManager;
    MockUAS*            _mockUAS;
    
    PX4RCCalibration*   _calWidget;
    
    enum {
        nextButtonMask =        1 << 0,
        cancelButtonMask =      1 << 1,
        skipButtonMask =        1 << 2
    };
    
    QPushButton*    _nextButton;
    QPushButton*    _cancelButton;
    QPushButton*    _skipButton;
    QLabel*         _statusLabel;
    
    RCValueWidget* _rgValueWidget[PX4RCCalibration::_chanMax];
    
    const char*         _rgSignals[1];
    MultiSignalSpy*     _multiSpyNextButtonMessageBox;
    
    // When settings values into min/max/trim we set them slightly different than the defaults so that
    // we can distinguish between the two values.
    static const int _testMinValue;
    static const int _testMaxValue;
    static const int _testCenterValue;
    
	static const int _availableChannels = 18;	///< Simulate 18 channel RC Transmitter
    static const int _stickSettleWait;
	
    static const struct ChannelSettings _rgChannelSettings[_availableChannels];
    static const struct ChannelSettings _rgChannelSettingsValidate[PX4RCCalibration::_chanMax];
	
	static const int _rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionMax];
};

#endif
