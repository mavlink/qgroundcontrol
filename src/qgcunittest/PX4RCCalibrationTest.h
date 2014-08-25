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

#include "AutoTest.h"
#include "MockUASManager.h"
#include "MockUAS.h"
#include "px4_configuration/PX4RCCalibration.h"

/// @file
///     @brief PX4RCCalibration Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

///     @brief PX4RCCalibration Widget unit test
class PX4RCCalibrationTest : public QObject
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
    //void _liveRC_test(void);
    void _beginState_test(void);
    void _identifyState_test(void);
    void _minMaxState_test(void);
    void _centerThrottleState_test(void);
    void _detectInversionState_test(void);
    void _trimsState_test(void);
    void _fullCalibration_test(void);
    
private:
    /// @brief Modes to run worker functions
    enum TestMode {
        testModeStandalone,     ///< Perform standalone test of calibration state
        testModePrerequisite,   ///< Setup prequisites for subsequent calibration state
        testModeFullSequence,   ///< Run as full calibration sequence
    };
    
    void _centerChannels(void);
    void _beginState_worker(enum TestMode mode);
    void _identifyState_worker(enum TestMode mode);
    void _minMaxState_worker(enum TestMode mode);
    void _centerThrottleState_worker(enum TestMode mode);
    void _detectInversionState_worker(enum TestMode mode);
    void _trimsState_worker(enum TestMode mode);
    
    enum {
        validateMinMaxMask =    1 << 0,
        validateTrimsMask =     1 << 1,
        validateReversedMask =  1 << 2,
        validateMappingMask =   1 << 3,
        validateAllMask = 0xFFFF
    };

    struct ChannelSettings {
        int rcMin;
        int rcMax;
        int rcTrim;
        int reversed;
        bool isMinMaxShown;
        bool isMinValid;
        bool isMaxValid;
    };
    
    void _validateParameters(int validateMask);
    void _validateWidgets(int validateMask, const struct ChannelSettings* rgChannelSettings);
    
    MockUASManager*     _mockUASManager;
    MockUAS*            _mockUAS;
    
    PX4RCCalibration*   _calWidget;
    
    enum {
        nextButtonMask =        1 << 0,
        cancelButtonMask =      1 << 1,
        skipButtonMask =        1 << 2,
        tryAgainButtonMask =    1 << 3
    };
    
    QPushButton*    _nextButton;
    QPushButton*    _cancelButton;
    QPushButton*    _skipButton;
    QPushButton*    _tryAgainButton;
    QLabel*         _statusLabel;
    
    RCChannelWidget* _rgRadioWidget[PX4RCCalibration::_chanMax];
    
    // When settings values into min/max/trim we set them slightly different than the defaults so that
    // we can distinguish between the two values.
    static const int _testMinValue;
    static const int _testMaxValue;
    static const int _testTrimValue;
    static const int _testThrottleTrimValue;
    
    static const int _availableChannels = 8;    ///< 8 channel RC Trasmitter
    static const int _requiredChannels = 5;     ///< Required channels are 0-4
    static const int _minMaxChannels = _requiredChannels + 1;    ///< Send min/max to channels 0-5
    static const int _attitudeChannels = 4;     ///< Attitude channels are 0-3
    
    static const struct ChannelSettings _rgChannelSettingsPreValidate[_availableChannels];
    static const struct ChannelSettings _rgChannelSettingsPostValidate[PX4RCCalibration::_chanMax];
};

DECLARE_TEST(PX4RCCalibrationTest)

#endif
