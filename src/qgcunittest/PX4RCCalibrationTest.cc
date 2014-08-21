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

#include "PX4RCCalibrationTest.h"
#include "UASManager.h"
#include "MockQGCUASParamManager.h"

/// @file
///     @brief QGCPX4RCCAlibration Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

// This will check for the wizard buttons being enabled of disabled according to the mask you pass in.
// We use a macro instead of a method so that we get better line number reporting on failure.
#define CHK_BUTTONS(mask) \
{ \
    if (_nextButton->isEnabled() != !!((mask) & nextButtonMask) || \
        _skipButton->isEnabled() != !!((mask) & skipButtonMask) || \
        _cancelButton->isEnabled() != !!((mask) & cancelButtonMask) || \
        _tryAgainButton->isEnabled() != !!((mask) & tryAgainButtonMask)) { \
        qDebug() << _statusLabel->text(); \
    } \
    QCOMPARE(_nextButton->isEnabled(), !!((mask) & nextButtonMask)); \
    QCOMPARE(_skipButton->isEnabled(), !!((mask) & skipButtonMask)); \
    QCOMPARE(_cancelButton->isEnabled(), !!((mask) & cancelButtonMask)); \
    QCOMPARE(_tryAgainButton->isEnabled(), !!((mask) & tryAgainButtonMask)); \
}

// This allows you to write unit tests which will click the Cancel button the first time through, followed
// by the Next button on the second iteration.
#define NEXT_OR_CANCEL(cancelNum) \
{ \
    if (standaloneTest && tryCancel ## cancelNum) { \
        QTest::mouseClick(_cancelButton, Qt::LeftButton); \
        QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateChannelWait); \
        tryCancel ## cancelNum = false; \
        goto StartOver; \
    } else { \
        QTest::mouseClick(_nextButton, Qt::LeftButton); \
    } \
}

PX4RCCalibrationTest::PX4RCCalibrationTest(void) :
    _mockUASManager(NULL),
    _calWidget(NULL)
{
    
}

void PX4RCCalibrationTest::init(void)
{
    _mockUASManager = new MockUASManager();
    Q_ASSERT(_mockUASManager);
    
    UASManager::setMockUASManager(_mockUASManager);
    
    _mockUAS = new MockUAS();
    Q_CHECK_PTR(_mockUAS);
    
    // This will instatiate the widget with no active UAS set
    _calWidget = new PX4RCCalibration();
    Q_CHECK_PTR(_calWidget);
    
    _mockUASManager->setMockActiveUAS(_mockUAS);

    // Get pointers to the push buttons
    _cancelButton = _calWidget->findChild<QPushButton*>("rcCalCancel");
    _nextButton = _calWidget->findChild<QPushButton*>("rcCalNext");
    _skipButton = _calWidget->findChild<QPushButton*>("rcCalSkip");
    _tryAgainButton = _calWidget->findChild<QPushButton*>("rcCalTryAgain");
    
    Q_ASSERT(_cancelButton);
    Q_ASSERT(_nextButton);
    Q_ASSERT(_skipButton);
    Q_ASSERT(_tryAgainButton);
    
    _statusLabel = _calWidget->findChild<QLabel*>("rcCalStatus");
    Q_ASSERT(_statusLabel);
 
    // Need to make sure standard channel indices are less then 4. Otherwise our _rgRadioWidget array won't work correctly.
    Q_ASSERT(PX4RCCalibration::rcCalFunctionRoll >= 0 && PX4RCCalibration::rcCalFunctionRoll < 4);
    Q_ASSERT(PX4RCCalibration::rcCalFunctionPitch >= 0 && PX4RCCalibration::rcCalFunctionPitch < 4);
    Q_ASSERT(PX4RCCalibration::rcCalFunctionYaw >= 0 && PX4RCCalibration::rcCalFunctionYaw < 4);
    Q_ASSERT(PX4RCCalibration::rcCalFunctionThrottle >= 0 && PX4RCCalibration::rcCalFunctionThrottle < 4);
    
    _rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionRoll] = _calWidget->findChild<QGCRadioChannelDisplay*>("rollWidget");
    _rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionPitch] = _calWidget->findChild<QGCRadioChannelDisplay*>("pitchWidget");
    _rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionYaw] = _calWidget->findChild<QGCRadioChannelDisplay*>("yawWidget");
    _rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionThrottle] = _calWidget->findChild<QGCRadioChannelDisplay*>("throttleWidget");
    
    Q_ASSERT(_rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionRoll]);
    Q_ASSERT(_rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionPitch]);
    Q_ASSERT(_rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionYaw]);
    Q_ASSERT(_rgAttitudeRadioWidget[PX4RCCalibration::rcCalFunctionThrottle]);

    for (size_t i=0; i<PX4RCCalibration::_chanMax; i++) {
        QString radioWidgetName("radio%1Widget");
        QString radioWidgetUserName("Radio %1");
        
        QGCRadioChannelDisplay* radioWidget = _calWidget->findChild<QGCRadioChannelDisplay*>(radioWidgetName.arg(i+1));
        Q_ASSERT(radioWidget);
        
        radioWidget->setOrientation(Qt::Horizontal);
        radioWidget->setName(radioWidgetUserName.arg(i+1));
        
        _rgRadioWidget[i] = radioWidget;
    }
}

void PX4RCCalibrationTest::cleanup(void)
{
    Q_ASSERT(_mockUAS);
    delete _mockUAS;
    
    UASManager::setMockUASManager(NULL);

    Q_ASSERT(_mockUASManager);
    delete _mockUASManager;
    
    Q_ASSERT(_calWidget);
    delete _calWidget;
}

/// @brief Tests for correct behavior when active UAS is set into widget.
void PX4RCCalibrationTest::_setUAS_test(void)
{
    // Widget is initialized with UAS, so it should be enabled
    QCOMPARE(_calWidget->isEnabled(), true);

    // Take away the UAS and widget should disable
    _mockUASManager->setMockActiveUAS(NULL);
    QCOMPARE(_calWidget->isEnabled(), false);
}

/// @brief Test for correct behavior in determining minimum numbers of channels for fligth.
void PX4RCCalibrationTest::_minRCChannels_test(void)
{
    // Next button won't be enabled until we see the minimum number of channels.
    for (int i=0; i<PX4RCCalibration::_chanMinimum; i++) {
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
        if (i == PX4RCCalibration::_chanMinimum - 1) {
            // Last channel should trigger enable
            CHK_BUTTONS(nextButtonMask);
        } else {
            // Still less than the minimum channels
            CHK_BUTTONS(0);
        }
    }
}

#if 0
/// @brief Tests that even when not calibrating the channel display is live
void PX4RCCalibrationTest::_liveRC_test(void)
{
    for (int i=0; i<PX4RCCalibration::_chanMax; i++) {
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMValidMaxValue);        
    }
    
    for (int i=0; i<PX4RCCalibration::_chanMax; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        QCOMPARE(radioWidget->value(), PX4RCCalibration::_rcCalPWMValidMinValue);
        QCOMPARE(radioWidget->max(), PX4RCCalibration::_rcCalPWMValidMaxValue);
    }
}
#endif

void PX4RCCalibrationTest::_beginState_worker(bool standaloneTest)
{
    bool tryCancel1 = true;
    
StartOver:
    if (standaloneTest) {
        _centerAllChannels();

        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateBegin);
    }

    // Next button is always enabled in this state
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);

    // Click the next button:
    // We should now be waiting for movement on a channel to identify the first RC function. The Next button will stay
    // disabled until the sticks are moved enough to identify the channel. For required functions the Skip button is
    // disabled.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateIdentify);
    CHK_BUTTONS(cancelButtonMask);
}

void PX4RCCalibrationTest::_beginState_test(void)
{
    _beginState_worker(true /* standalone test */);
}

void PX4RCCalibrationTest::_identifyState_worker(bool standaloneTest, bool skipOptional)
{
    bool tryCancel1 = standaloneTest;
    
StartOver:
    if (standaloneTest) {
        _centerAllChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateIdentify);
        
    }
    
    // Loop over all functions setting them to a channel
    for (int i=0; i<PX4RCCalibration::rcCalFunctionMax; i++) {
        // If this function is required you can't skip it
        bool skipAllowed = !PX4RCCalibration::_rgFunctionInfo[i].required;
        int skipMask = skipAllowed ? skipButtonMask : 0;
        
        // We should now be waiting for movement on a channel to identify the RC function. The Next button will stay
        // disabled until the sticks are moved enough to identify the channel. For required functions the Skip button is
        // disabled.
        CHK_BUTTONS(cancelButtonMask | skipMask);

        // Move channel lower than delta to make sure function is not identified
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMCenterPoint - (PX4RCCalibration::_rcCalMoveDelta - 2.0f));
        CHK_BUTTONS(cancelButtonMask | skipMask);

        if (i != 0) {
            // Try to assign a channel 0 to more than one function. This is not allowed so Next button should not enable.
            _mockUAS->emitRemoteControlChannelRawChanged(0, (float)PX4RCCalibration::_rcCalPWMValidMinValue);
            _mockUAS->emitRemoteControlChannelRawChanged(0, (float)PX4RCCalibration::_rcCalPWMValidMaxValue);
            CHK_BUTTONS(cancelButtonMask | skipMask);
        }

        // Skip this mapping if allowed and requested
        if (skipAllowed && skipOptional) {
            QTest::mouseClick(_skipButton, Qt::LeftButton);
            continue;
        } else {
            
        }
        
        if (tryCancel1) {
            NEXT_OR_CANCEL(1);
        }
        
        // Move channel larger than delta to identify channel. We should now be sitting in a found state.
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMValidMinValue);
        CHK_BUTTONS(cancelButtonMask | tryAgainButtonMask | nextButtonMask);

        NEXT_OR_CANCEL(1);
    }
    
    // We should now be waiting for min/max values.
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateMinMax);
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);
    
    if (standaloneTest) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateMappingMask, skipOptional);
    }
}

void PX4RCCalibrationTest::_identifyState_test(void)
{
    _identifyState_worker(true /* standalone test */, false /* don't skip optional */);
}

void PX4RCCalibrationTest::_identifyStateSkipOptional_test(void)
{
    _identifyState_worker(true /* standalone test */, true /* skip optional */);
}

void PX4RCCalibrationTest::_minMaxState_worker(bool standaloneTest)
{
    bool tryCancel1 = true;
    
StartOver:
    if (standaloneTest) {
        // The Min/Max calibration updates the radio channel ui widgets with the min/max values as you move the sticks.
        // In order for the roll/pitch/yaw/throttle radio channel ui widgets to be updated correctly those fucntions
        // must be alread mapped to a channel. So we have to run the _identifyState_test first to set up the internal
        // state correctly.
        _identifyState_test();

        _centerAllChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateMinMax);
        
        // We should now be waiting for min/max values.
        CHK_BUTTONS(nextButtonMask | cancelButtonMask);
    }

    // Send min/max values for all channels
    for (int i=0; i<PX4RCCalibration::_chanMax; i++) {
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMValidMinValue);
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMValidMaxValue);
    }

    // Make sure throttle is at min
    _mockUAS->emitRemoteControlChannelRawChanged(PX4RCCalibration::rcCalFunctionThrottle, (float)PX4RCCalibration::_rcCalPWMValidMinValue);

    // Click the next button: We should now be waiting for center throttle in prep for inversion detection.
    // Throttle channel is at minimum so Next button should be disabled.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateCenterThrottle);
    CHK_BUTTONS(cancelButtonMask);

    // Also at this point the radio channel widgets should be displaying the current min/max we just set.
    // Check both the Attitude Control widgets as well as generic channel widgets.
    
    Q_ASSERT(PX4RCCalibration::rcCalFunctionFirstAttitudeFunction == 0);
    for (int i=0; i<PX4RCCalibration::rcCalFunctionLastAttitudeFunction; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgAttitudeRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        QCOMPARE(radioWidget->isMinMaxShown(), true);
        QCOMPARE(radioWidget->min(), PX4RCCalibration::_rcCalPWMValidMinValue);
        QCOMPARE(radioWidget->max(), PX4RCCalibration::_rcCalPWMValidMaxValue);
    }

    for (int i=0; i<PX4RCCalibration::_chanMax; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        QCOMPARE(radioWidget->isMinMaxShown(), true);
        QCOMPARE(radioWidget->min(), PX4RCCalibration::_rcCalPWMValidMinValue);
        QCOMPARE(radioWidget->max(), PX4RCCalibration::_rcCalPWMValidMaxValue);
    }
    
    if (standaloneTest) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateMinMaxMask);
    }
}

void PX4RCCalibrationTest::_minMaxState_test(void)
{
    _minMaxState_worker(true /* standalone test */);
}

void PX4RCCalibrationTest::_centerThrottleState_worker(bool standaloneTest)
{
    bool tryCancel1 = true;
    
StartOver:
    if (standaloneTest) {
        // In order to perform the center throttle state test the throttle channel has to have been identified.
        // So we have to run the _identifyState_test first to set up the internal state correctly.
        _identifyState_test();
        
        _centerAllChannels();
        _mockUAS->emitRemoteControlChannelRawChanged(PX4RCCalibration::rcCalFunctionThrottle, (float)PX4RCCalibration::_rcCalPWMValidMinValue);
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateCenterThrottle);
        
        // We should now be waiting for center throttle in prep for inversion detection.
        // Throttle channel is at minimum so Next button should be disabled.
        CHK_BUTTONS(cancelButtonMask);
    }

    // Move the throttle to just below rough center. Next should still be disabled
    _mockUAS->emitRemoteControlChannelRawChanged(PX4RCCalibration::rcCalFunctionThrottle, PX4RCCalibration::_rcCalPWMCenterPoint - PX4RCCalibration::_rcCalRoughCenterDelta - 1);
    CHK_BUTTONS(cancelButtonMask);
    
    // Center the throttle and make sure Next button gets enabled
    _mockUAS->emitRemoteControlChannelRawChanged(PX4RCCalibration::rcCalFunctionThrottle, PX4RCCalibration::_rcCalPWMCenterPoint);
    CHK_BUTTONS(cancelButtonMask | nextButtonMask);
    
    // Click the next button which should take us to our first channel inversion test. The Next button will stay disabled until
    // the stick for the specified channel is moved down.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateDetectInversion);
    CHK_BUTTONS(cancelButtonMask);
}

void PX4RCCalibrationTest::_centerThrottleState_test(void)
{
    _centerThrottleState_worker(true /* standalone test */);
}

void PX4RCCalibrationTest::_detectInversionState_worker(bool standaloneTest)
{
    bool tryCancel1 = true;
    bool tryCancel2 = true;
    
StartOver:
    if (standaloneTest) {
        // In order to perform the detect inversion test the roll/pitch/yaw/throttle functions must be mapped to a channel.
        // So we have to run the _identifyState_test first to set up the internal state correctly.
        _identifyState_test();
        
        _centerAllChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateDetectInversion);
        
        // We should now be at the first channel inversion test. The Next button will stay disabled until the stick for the specified
        // channel is moved in the appropriate direction.
        CHK_BUTTONS(cancelButtonMask);
    }

    // Loop over Attitude Control Functions (roll/yaw/pitch/throttle) to detect inversion
    for (int chanFunction=PX4RCCalibration::rcCalFunctionFirstAttitudeFunction; chanFunction<=PX4RCCalibration::rcCalFunctionLastAttitudeFunction; chanFunction++) {
        if (chanFunction != 0) {
            // Click next to move to next inversion to identify
            NEXT_OR_CANCEL(1);
            CHK_BUTTONS(cancelButtonMask);
        }
        
        // Move all channels except for the one we are trying to detect to min and max value to make sure there is no effect.
        for (int chan=0; chan<PX4RCCalibration::_chanMax; chan++) {
            if (chanFunction != chan) {
                _mockUAS->emitRemoteControlChannelRawChanged(chan, PX4RCCalibration::_rcCalPWMValidMaxValue);
                CHK_BUTTONS(cancelButtonMask);
            }
        }
        
        // Move the channel we are detecting inversion on to the min value which should indicate no inversion.
        // This should put us in the found state and enable the Next button.
        _mockUAS->emitRemoteControlChannelRawChanged(chanFunction, PX4RCCalibration::_rcCalPWMValidMinValue);
        CHK_BUTTONS(cancelButtonMask | tryAgainButtonMask | nextButtonMask);
    }
    
    // Click the next button: We should now be waiting for low throttle in prep for trim detection.
    // Throttle channel is at minimum so Next button should be disabled.
    _centerAllChannels();
    NEXT_OR_CANCEL(2);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateTrims);
    CHK_BUTTONS(cancelButtonMask);
    
    if (standaloneTest) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateMappingMask | validateReversedMask);
    }
}

void PX4RCCalibrationTest::_detectInversionState_test(void)
{
    _detectInversionState_worker(true /* standalone test */);
}

void PX4RCCalibrationTest::_trimsState_worker(bool standaloneTest)
{
    bool tryCancel1 = true;
    
StartOver:
    if (standaloneTest) {
        // In order to perform the trim state test the functions must be mapped and the min/max values must be set.
        // So we have to run the _identifyState_test and _minMaxState_test first to set up the internal state correctly.
        _identifyState_test();
        _minMaxState_test();
        
        _centerAllChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateTrims);
        
        // We should now be waiting for low throttle.
        CHK_BUTTONS(cancelButtonMask);
    }

    // Move the throttle to min. Next should enable
    _mockUAS->emitRemoteControlChannelRawChanged(PX4RCCalibration::rcCalFunctionThrottle, PX4RCCalibration::_rcCalPWMValidMinValue);
    CHK_BUTTONS(cancelButtonMask | nextButtonMask);
    
    // Click the next button which should set Trims and take us the Save step.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateSave);
    CHK_BUTTONS(cancelButtonMask | nextButtonMask);

    if (standaloneTest) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateTrimsMask);
    }
}

void PX4RCCalibrationTest::_trimsState_test(void)
{
    _trimsState_worker(true /* standalone test */);
}

void PX4RCCalibrationTest::_fullCalibration_test(void) {
    _centerAllChannels();
    QTest::mouseClick(_nextButton, Qt::LeftButton);

    _beginState_worker(false);
    _identifyState_worker(false, false);
    _minMaxState_worker(false);
    _centerThrottleState_worker(false);
    _detectInversionState_worker(false);
    _trimsState_worker(false);

    // One more click and the parameters should get saved
    QTest::mouseClick(_nextButton, Qt::LeftButton);
    
    _validateParameters(validateAllMask);
}

/// @brief Sends RC center point values on all channels.
void PX4RCCalibrationTest::_centerAllChannels(void)
{
    // We use all channels for testing. Initialize to center point. This should set the channel count above the minimum
    // such that we can enter the idle state.
    for (int i=0; i<PX4RCCalibration::_chanMax; i++) {
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
    }
}

void PX4RCCalibrationTest::_validateParameters(int validateMask, bool skipOptional)
{
    MockQGCUASParamManager* paramMgr = _mockUAS->getMockQGCUASParamManager();
    MockQGCUASParamManager::ParamMap_t mapParamsSet = paramMgr->getMockSetParameters();

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
    // Set Min/Max/Reversed parameter for all channels. Not that parameter name is 1-based.
    for (int chan = 1; chan <= PX4RCCalibration::_chanMax; chan++) {
        if (validateMask & validateMinMaxMask) {
            QCOMPARE(mapParamsSet.contains(minTpl.arg(chan)), true);
            QCOMPARE(mapParamsSet[minTpl.arg(chan)].toInt(), PX4RCCalibration::_rcCalPWMValidMinValue);
            QCOMPARE(mapParamsSet.contains(maxTpl.arg(chan)), true);
            QCOMPARE(mapParamsSet[maxTpl.arg(chan)].toInt(), PX4RCCalibration::_rcCalPWMValidMaxValue);
        }

        if (validateMask & validateReversedMask) {
            QCOMPARE(mapParamsSet.contains(revTpl.arg(chan)), true);
            QCOMPARE(mapParamsSet[revTpl.arg(chan)].toFloat(), 1.0f /* not reversed */);
        }
    }
    
    if (validateMask & validateMappingMask) {
        for (int chanFunction=0; chanFunction<PX4RCCalibration::rcCalFunctionMax; chanFunction++) {
            // All functions should be mapped to the same channel index
            QCOMPARE(mapParamsSet.contains(PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName), true);
            
            int expectedValue;
            if (skipOptional && !PX4RCCalibration::_rgFunctionInfo[chanFunction].required) {
                expectedValue = 0;  // 0 signals no mapping
            } else {
                expectedValue = chanFunction + 1; // 1-based
            }
            QCOMPARE(mapParamsSet[PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName].toInt(), expectedValue);
        }
    }

    if (validateMask & validateTrimsMask) {
        // Trims should be set for all Control Functions
        for (int chanFunction=0; chanFunction<PX4RCCalibration::rcCalFunctionMax; chanFunction++) {
            // For the unit test the function index == mapped channel. Also parameter name is 1-based.
            int mappedChannel = chanFunction + 1;
            
            QCOMPARE(mapParamsSet.contains(trimTpl.arg(mappedChannel)), true);
            
            int trimValue;
            if (chanFunction == PX4RCCalibration::rcCalFunctionThrottle) {
                trimValue = PX4RCCalibration::_rcCalPWMValidMinValue;
            } else {
                trimValue = PX4RCCalibration::_rcCalPWMCenterPoint;
            }
            QCOMPARE(mapParamsSet[trimTpl.arg(mappedChannel)].toInt(), trimValue);
        }
    }
}
