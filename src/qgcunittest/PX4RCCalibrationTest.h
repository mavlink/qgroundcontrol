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
///     @brief QGCPX4RCCAlibration Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

class PX4RCCalibrationTest : public QObject
{
    Q_OBJECT
    
public:
    PX4RCCalibrationTest(void);
    
private slots:
    void init(void);
    void cleanup(void);
    
    void _setUAS_test(void);
    void _minRCChannels_test(void);
    //void _liveRC_test(void);
    void _beginState_test(void);
    void _identifyState_test(void);
    void _identifyStateSkipOptional_test(void);
    void _minMaxState_test(void);
    void _centerThrottleState_test(void);
    void _detectInversionState_test(void);
    void _trimsState_test(void);
    void _fullCalibration_test(void);
    
private:
    void _centerAllChannels(void);
    void _beginState_worker(bool standaloneTest);
    void _identifyState_worker(bool standaloneTest, bool skipOptional);
    void _minMaxState_worker(bool standaloneTest);
    void _centerThrottleState_worker(bool standaloneTest);
    void _detectInversionState_worker(bool standaloneTest);
    void _trimsState_worker(bool standaloneTest);
    
    enum {
        validateMinMaxMask =    1 << 0,
        validateTrimsMask =     1 << 1,
        validateReversedMask =  1 << 2,
        validateMappingMask =   1 << 3,
        validateAllMask = 0xFFFF
    };
    void _validateParameters(int validateMask, bool skipOptional = false);
    
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
    
    QGCRadioChannelDisplay* _rgAttitudeRadioWidget[4];
    QGCRadioChannelDisplay* _rgRadioWidget[PX4RCCalibration::_chanMax];
};

DECLARE_TEST(PX4RCCalibrationTest)

#endif
