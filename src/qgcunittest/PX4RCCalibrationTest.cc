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
///     @brief QPX4RCCalibration Widget unit test
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
    if (mode == testModeStandalone && tryCancel ## cancelNum) { \
        QTest::mouseClick(_cancelButton, Qt::LeftButton); \
        QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateChannelWait); \
        tryCancel ## cancelNum = false; \
        goto StartOver; \
    } else { \
        QTest::mouseClick(_nextButton, Qt::LeftButton); \
    } \
}

const int PX4RCCalibrationTest::_testMinValue = PX4RCCalibration::_rcCalPWMDefaultMinValue + 10;
const int PX4RCCalibrationTest::_testMaxValue = PX4RCCalibration::_rcCalPWMDefaultMaxValue - 10;
const int PX4RCCalibrationTest::_testTrimValue = PX4RCCalibration::_rcCalPWMDefaultTrimValue + 10;
const int PX4RCCalibrationTest::_testThrottleTrimValue = PX4RCCalibration::_rcCalPWMDefaultMinValue + 10;

/// @brief Maps from function index to channel index. -1 signals no mapping. Channel indices are offset 1 from function index
/// to catch bugs where function index is incorrectly used as channel index.
const int PX4RCCalibrationTest::_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionMax]= { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, -1 };

const struct PX4RCCalibrationTest::ChannelSettings PX4RCCalibrationTest::_rgChannelSettingsPreValidate[PX4RCCalibrationTest::_availableChannels] = {
	// Function										Min Value									Max Value                                   Trim Value                                      Reversed    MinMaxShown MinValid MaxValid
	
	// Channel 0 : Not mapped to function, Simulate invalid Min/Max
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,		PX4RCCalibration::_rcCalPWMCenterPoint,     PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   false,  false },
	
    // Channels 1-10 are mapped to all available modes, except for Aux2 which is skipped
	{ PX4RCCalibration::rcCalFunctionRoll,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testTrimValue,           true,		true,   true,   true },
    { PX4RCCalibration::rcCalFunctionPitch,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testTrimValue,           false,      true,   true,   true },
    { PX4RCCalibration::rcCalFunctionYaw,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testTrimValue,           true,		true,   true,   true },
    { PX4RCCalibration::rcCalFunctionThrottle,		PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testThrottleTrimValue,   false,      true,   true,   true },
    { PX4RCCalibration::rcCalFunctionModeSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionPosCtlSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionLoiterSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionReturnSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionFlaps,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionAux1,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	
	// Channel 11 : Not mapped to function, Simulate invalid Min/Max
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,		PX4RCCalibration::_rcCalPWMCenterPoint,     PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   false,  false },
	
    // Channel 12 : Not mapped to function, Simulate invalid Min, valid Max
    { PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,		PX4RCCalibration::_rcCalPWMDefaultMaxValue, PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   false,  true },
    
	// Channel 13 : Not mapped to function, Simulate valid Min, invalid Max
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMCenterPoint, PX4RCCalibration::_rcCalPWMCenterPoint,				false,      true,   true,  false },
	
    // Channels 14-17: Not mapped to function, Simulate invalid Min/Max, since available channel Min/Max is still shown
    { PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,   PX4RCCalibration::_rcCalPWMCenterPoint,     PX4RCCalibration::_rcCalPWMCenterPoint,			false,      true,   false,  false },
    { PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,   PX4RCCalibration::_rcCalPWMCenterPoint,     PX4RCCalibration::_rcCalPWMCenterPoint,			false,      true,   false,  false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,   PX4RCCalibration::_rcCalPWMCenterPoint,     PX4RCCalibration::_rcCalPWMCenterPoint,			false,      true,   false,  false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMCenterPoint,   PX4RCCalibration::_rcCalPWMCenterPoint,     PX4RCCalibration::_rcCalPWMCenterPoint,			false,      true,   false,  false },
};

const struct PX4RCCalibrationTest::ChannelSettings PX4RCCalibrationTest::_rgChannelSettingsPostValidate[PX4RCCalibration::_chanMax] = {
    // Function										Min Value									Max Value									Trim Value										Reversed    MinMaxShown MinValid MaxValid
	
	// Channel 0 is not mapped and should be defaulted
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	
	// Channels 1-10 are mapped to all available modes, except for Aux2 which is skipped
	{ PX4RCCalibration::rcCalFunctionRoll,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testTrimValue,           true,		true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionPitch,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testTrimValue,           false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionYaw,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testTrimValue,           true,		true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionThrottle,		PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibrationTest::_testThrottleTrimValue,   false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionModeSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionPosCtlSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionLoiterSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionReturnSwitch,	PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionFlaps,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	{ PX4RCCalibration::rcCalFunctionAux1,			PX4RCCalibrationTest::_testMinValue,		PX4RCCalibrationTest::_testMaxValue,        PX4RCCalibration::_rcCalPWMCenterPoint,         false,      true,   true,   true },
	
	// Channels 11-17 are not mapped and should be set to defaults
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMDefaultTrimValue,	false,      true,   true,	true },
};

PX4RCCalibrationTest::PX4RCCalibrationTest(void) :
    _mockUASManager(NULL),
    _calWidget(NULL)
{
}

/// @brief Called one time before any test cases are run.
void PX4RCCalibrationTest::initTestCase(void)
{
	// Validate that our function to channel mapping is still correct.
	for (int function=0; function<PX4RCCalibration::rcCalFunctionMax; function++) {
		int chanIndex = _rgFunctionChannelMap[function];
		if (chanIndex != -1) {
			Q_ASSERT(_rgChannelSettingsPreValidate[chanIndex].function == function);
			Q_ASSERT(_rgChannelSettingsPostValidate[chanIndex].function == function);
		}
	}
	Q_ASSERT(_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionAux2] == -1);
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
 
    for (size_t i=0; i<PX4RCCalibration::_chanMax; i++) {
        QString radioWidgetName("radio%1Widget");
        QString radioWidgetUserName("Radio %1");
        
        RCChannelWidget* radioWidget = _calWidget->findChild<RCChannelWidget*>(radioWidgetName.arg(i+1));
        Q_ASSERT(radioWidget);
        
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
    for (int chan=0; chan<PX4RCCalibration::_chanMinimum; chan++) {
        _mockUAS->emitRemoteControlChannelRawChanged(chan, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
        
        // We use _chanCount internally so we should validate it
        QCOMPARE(_calWidget->_chanCount, chan+1);
        
        // Validate Next button state
        if (chan == PX4RCCalibration::_chanMinimum - 1) {
            // Last channel should trigger enable
            CHK_BUTTONS(nextButtonMask);
        } else {
            // Still less than the minimum channels
            CHK_BUTTONS(0);
        }

        // Only available channels should have enabled widget. A ui update cycle needs to have passed so we wait a little.
        QTest::qWait(PX4RCCalibration::_updateInterval * 2);
        for (int chanWidget=0; chanWidget<PX4RCCalibration::_chanMax; chanWidget++) {
            QCOMPARE(_rgRadioWidget[chanWidget]->isEnabled(), !!(chanWidget <= chan));
        }
    }
}

void PX4RCCalibrationTest::_beginState_worker(enum TestMode mode)
{
    bool tryCancel1 = true;
    
StartOver:
    if (mode == testModeStandalone || mode == testModePrerequisite) {
        _centerChannels();

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
    _beginState_worker(testModeStandalone);
}

void PX4RCCalibrationTest::_identifyState_worker(enum TestMode mode)
{
    bool tryCancel1 = true;
    
StartOver:
    if (mode == testModeStandalone || mode == testModePrerequisite)
    {
        _centerChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateIdentify);
        
    }
    
    // Loop over all function identifying mapped channels
    for (int function=0; function<PX4RCCalibration::rcCalFunctionMax; function++) {
		int channelIndex = _rgFunctionChannelMap[function];
		
        // If this function is not mapped we skip it
		bool skipNonMapped = channelIndex == -1;
		bool skipButtonEnabled = !PX4RCCalibration::_rgFunctionInfo[function].required;
        int skipMask = skipButtonEnabled ? skipButtonMask : 0;
        
        // We should now be waiting for movement on a channel to identify the RC function. The Next button will stay
        // disabled until the sticks are moved enough to identify the channel. For required functions the Skip button is
        // disabled.
        CHK_BUTTONS(cancelButtonMask | skipMask);

        // Skip this mapping if specified by test case data
        if (skipNonMapped) {
            QTest::mouseClick(_skipButton, Qt::LeftButton);
            continue;
        }
        
        // Move channel less than delta to make sure function is not identified
        _mockUAS->emitRemoteControlChannelRawChanged(channelIndex, (float)PX4RCCalibration::_rcCalPWMCenterPoint + (PX4RCCalibration::_rcCalMoveDelta - 2.0f));
        CHK_BUTTONS(cancelButtonMask | skipMask);

        if (function != 0) {
            // Try to assign a function index 0 channel to more than one function. This is not allowed so Next button should not enable.
            _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[0], (float)PX4RCCalibration::_rcCalPWMValidMinValue);
            _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[0], (float)PX4RCCalibration::_rcCalPWMValidMaxValue);
            CHK_BUTTONS(cancelButtonMask | skipMask);
        }

        if (tryCancel1) {
            NEXT_OR_CANCEL(1);
        }
        
        // Move channel larger than delta to identify channel. We should now be sitting in a found state.
        _mockUAS->emitRemoteControlChannelRawChanged(channelIndex, (float)PX4RCCalibration::_rcCalPWMCenterPoint + (PX4RCCalibration::_rcCalMoveDelta + 2.0f));
        CHK_BUTTONS(cancelButtonMask | tryAgainButtonMask | nextButtonMask);

        NEXT_OR_CANCEL(1);
    }
    
    // We should now be waiting for min/max values.
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateMinMax);
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);
    
    if (mode == testModeStandalone) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateMappingMask);
    }
}

void PX4RCCalibrationTest::_identifyState_test(void)
{
    _identifyState_worker(testModeStandalone);
}

void PX4RCCalibrationTest::_minMaxState_worker(enum TestMode mode)
{
    bool tryCancel1 = true;
    
StartOver:
    if (mode == testModeStandalone || mode == testModePrerequisite) {
        // The Min/Max calibration updates the radio channel ui widgets with the min/max values for all mapped channels.
        // In order for those adio channel ui widgets to be updated correctly those functions must be alread mapped to a
		// channel. So we have to run the _identifyState_test first to set up the internal state correctly.
        _identifyState_worker(testModePrerequisite);

        _centerChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateMinMax);
        
        // We should now be waiting for min/max values.
        CHK_BUTTONS(nextButtonMask | cancelButtonMask);
    }
    
    // Before we start sending rc values the widgets should all have min/max as invalid
    for (int chan=0; chan<PX4RCCalibration::_chanMax; chan++) {
        QCOMPARE(_rgRadioWidget[chan]->isMinValid(), false);
        QCOMPARE(_rgRadioWidget[chan]->isMaxValid(), false);
    }
    
    // Try setting a min/max value that is below the threshold to make sure min/max doesn't go valid
    _mockUAS->emitRemoteControlChannelRawChanged(0, (float)(PX4RCCalibration::_rcCalPWMValidMinValue + 1));
    _mockUAS->emitRemoteControlChannelRawChanged(0, (float)(PX4RCCalibration::_rcCalPWMValidMaxValue - 1));
    QCOMPARE(_rgRadioWidget[0]->isMinValid(), false);
    QCOMPARE(_rgRadioWidget[0]->isMaxValid(), false);

    // Send min/max values for all channels
    for (int chan=0; chan<_availableChannels; chan++) {
		//qDebug() << chan << _rgChannelSettingsPreValidate[chan].rcMin << _rgChannelSettingsPreValidate[chan].rcMax;
		_mockUAS->emitRemoteControlChannelRawChanged(chan, (float)_rgChannelSettingsPreValidate[chan].rcMin);
		_mockUAS->emitRemoteControlChannelRawChanged(chan, (float)_rgChannelSettingsPreValidate[chan].rcMax);
		
		// Re-Center to not screw up next state
		_mockUAS->emitRemoteControlChannelRawChanged(chan, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
    }

    _validateWidgets(validateMinMaxMask, _rgChannelSettingsPreValidate);
    
    // Make sure throttle is at min
    _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionThrottle], (float)PX4RCCalibration::_rcCalPWMValidMinValue);

    // Click the next button: We should now be waiting for center throttle in prep for inversion detection.
    // Throttle channel is at minimum so Next button should be disabled.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateCenterThrottle);
    CHK_BUTTONS(cancelButtonMask);

    if (mode == testModeStandalone) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateMinMaxMask);
    }
}

void PX4RCCalibrationTest::_minMaxState_test(void)
{
    _minMaxState_worker(testModeStandalone);
}

void PX4RCCalibrationTest::_centerThrottleState_worker(enum TestMode mode)
{
    bool tryCancel1 = true;
    
StartOver:
    if (mode == testModeStandalone || mode == testModePrerequisite) {
        // In order to perform the center throttle state test the throttle channel has to have been identified.
        // So we have to run the _identifyState_test first to set up the internal state correctly.
        _identifyState_worker(testModePrerequisite);
        
        _centerChannels();
        _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionThrottle], (float)PX4RCCalibration::_rcCalPWMValidMinValue);
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateCenterThrottle);
        
        // We should now be waiting for center throttle in prep for inversion detection.
        // Throttle channel is at minimum so Next button should be disabled.
        CHK_BUTTONS(cancelButtonMask);
    }

    // Move the throttle to just below rough center. Next should still be disabled
    _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionThrottle], PX4RCCalibration::_rcCalPWMCenterPoint - PX4RCCalibration::_rcCalRoughCenterDelta - 1);
    CHK_BUTTONS(cancelButtonMask);
    
    // Center the throttle and make sure Next button gets enabled
    _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionThrottle], PX4RCCalibration::_rcCalPWMCenterPoint);
    CHK_BUTTONS(cancelButtonMask | nextButtonMask);
    
    // Click the next button which should take us to our first channel inversion test. The Next button will stay disabled until
    // the stick for the specified channel is moved down.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateDetectInversion);
    CHK_BUTTONS(cancelButtonMask);
}

void PX4RCCalibrationTest::_centerThrottleState_test(void)
{
    _centerThrottleState_worker(testModeStandalone);
}

void PX4RCCalibrationTest::_detectInversionState_worker(enum TestMode mode)
{
    bool tryCancel1 = true;
    bool tryCancel2 = true;
    
StartOver:
    if (mode == testModeStandalone || mode == testModePrerequisite) {
        // In order to perform the detect inversion test the roll/pitch/yaw/throttle functions must be mapped to a channel.
        // So we have to run the _identifyState_test first to set up the internal state correctly.
        _identifyState_worker(testModePrerequisite);
        
        _centerChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateDetectInversion);
        
        // We should now be at the first channel inversion test. The Next button will stay disabled until the stick for the specified
        // channel is moved in the appropriate direction.
        CHK_BUTTONS(cancelButtonMask);
    }

    // Loop over Attitude Control Functions (roll/yaw/pitch/throttle) to detect inversion
    for (int chanFunction=PX4RCCalibration::rcCalFunctionFirstAttitudeFunction; chanFunction<=PX4RCCalibration::rcCalFunctionLastAttitudeFunction; chanFunction++) {
		int channelIndex = _rgFunctionChannelMap[chanFunction];
		
        if (chanFunction != 0) {
            // Click next to move to next inversion to identify
            NEXT_OR_CANCEL(1);
            CHK_BUTTONS(cancelButtonMask);
        }
        
        // Move all channels except for the one we are trying to detect to min and max value to make sure there is no effect.
        for (int chan=0; chan<_availableChannels; chan++) {
            if (channelIndex != chan) {
                _mockUAS->emitRemoteControlChannelRawChanged(chan, (float)PX4RCCalibration::_rcCalPWMCenterPoint + (PX4RCCalibration::_rcCalMoveDelta + 2.0f));
                _mockUAS->emitRemoteControlChannelRawChanged(chan, (float)PX4RCCalibration::_rcCalPWMCenterPoint - (PX4RCCalibration::_rcCalMoveDelta + 2.0f));
                CHK_BUTTONS(cancelButtonMask);

                // Make sure to re-center for next inversion detect
				//qDebug() << "Inversion recenter" << chan << PX4RCCalibration::_rcCalPWMCenterPoint;
                _mockUAS->emitRemoteControlChannelRawChanged(chan, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
            }
        }
        
        // Move the channel we are detecting inversion on to the min or max value depending onn test case data.
        // This should put us in the found state and enable the Next button.
		float inversionValue = PX4RCCalibration::_rcCalPWMCenterPoint;
		float inversionDelta = (float)(PX4RCCalibration::_rcCalMoveDelta + 2.0f);
		if (_rgChannelSettingsPreValidate[channelIndex].reversed) {
			inversionValue += inversionDelta;
		} else {
			inversionValue -= inversionDelta;
		}
		//qDebug() << "Reverse set" << chanFunction << channelIndex << inversionValue;
        _mockUAS->emitRemoteControlChannelRawChanged(channelIndex, inversionValue);
        CHK_BUTTONS(cancelButtonMask | tryAgainButtonMask | nextButtonMask);
    }
    
    // Click the next button: We should now be waiting for low throttle in prep for trim detection.
    // Throttle channel is at minimum so Next button should be disabled.
    _centerChannels();
    NEXT_OR_CANCEL(2);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateTrims);
    CHK_BUTTONS(cancelButtonMask);
    
    if (mode == testModeStandalone) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateMappingMask | validateReversedMask);
    }
}

void PX4RCCalibrationTest::_detectInversionState_test(void)
{
    _detectInversionState_worker(testModeStandalone);
}

void PX4RCCalibrationTest::_trimsState_worker(enum TestMode mode)
{
    bool tryCancel1 = true;
    
StartOver:
    if (mode == testModeStandalone || mode == testModePrerequisite) {
        // In order to perform the trim state test the functions must be mapped and the min/max values must be set.
        // So we have to run the _minMaxState_test first to set up the internal state correctly.
        _minMaxState_worker(testModePrerequisite);
        
        _centerChannels();
        
        _calWidget->_unitTestForceCalState(PX4RCCalibration::rcCalStateTrims);
        
        // We should now be waiting for low throttle.
        CHK_BUTTONS(cancelButtonMask);
    }

    // Send trim values to channels
	for (int chan=0; chan<_availableChannels; chan++) {
		//qDebug () << "Trim set" << chan << _rgChannelSettingsPreValidate[chan].rcTrim;
		_mockUAS->emitRemoteControlChannelRawChanged(chan, _rgChannelSettingsPreValidate[chan].rcTrim);
    }
    
    _validateWidgets(validateTrimsMask, _rgChannelSettingsPreValidate);
    
    // Throttle trim was set, so next should be enabled
    CHK_BUTTONS(cancelButtonMask | nextButtonMask);
    
    // Click the next button which should set Trims and take us the Save step.
    NEXT_OR_CANCEL(1);
    QCOMPARE(_calWidget->_rcCalState, PX4RCCalibration::rcCalStateSave);
    CHK_BUTTONS(cancelButtonMask | nextButtonMask);
    _validateWidgets(validateTrimsMask, _rgChannelSettingsPostValidate);

    if (mode == testModeStandalone) {
        _calWidget->_writeCalibration(false /* !trimsOnly */);
        _validateParameters(validateTrimsMask);
    }
}

void PX4RCCalibrationTest::_trimsState_test(void)
{
    _trimsState_worker(testModeStandalone);
}

void PX4RCCalibrationTest::_fullCalibration_test(void) {
    _centerChannels();
    QTest::mouseClick(_nextButton, Qt::LeftButton);

    _beginState_worker(testModeFullSequence);
    _identifyState_worker(testModeFullSequence);
    _minMaxState_worker(testModeFullSequence);
    _centerThrottleState_worker(testModeFullSequence);
    _detectInversionState_worker(testModeFullSequence);
    _trimsState_worker(testModeFullSequence);

    // One more click and the parameters should get saved
    QTest::mouseClick(_nextButton, Qt::LeftButton);
    
    _validateParameters(validateAllMask);
    _validateWidgets(validateAllMask, _rgChannelSettingsPostValidate);
}

/// @brief Sends RC center point values on minimum set of channels.
void PX4RCCalibrationTest::_centerChannels(void)
{
    // Initialize available channels them to center point. This should also set the channel count above the
    // minimum such that we can enter the idle state.
    for (int i=0; i<_availableChannels; i++) {
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
    }
}

void PX4RCCalibrationTest::_validateParameters(int validateMask)
{
    MockQGCUASParamManager* paramMgr = _mockUAS->getMockQGCUASParamManager();
    MockQGCUASParamManager::ParamMap_t mapParamsSet = paramMgr->getMockSetParameters();

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
	if (validateMask & validateMappingMask) {
		// Check mapping for all fuctions
		for (int chanFunction=0; chanFunction<PX4RCCalibration::rcCalFunctionMax; chanFunction++) {
			int chanIndex = _rgFunctionChannelMap[chanFunction];
			
			int expectedParameterValue;
			if (chanIndex == -1) {
				expectedParameterValue = 0;  // 0 signals no mapping
			} else {
				expectedParameterValue = chanIndex + 1; // 1-based parameter value
			}
			
			if (validateMask & validateMappingMask) {
				QCOMPARE(mapParamsSet.contains(PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName), true);
				QCOMPARE(mapParamsSet[PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName].toInt(), expectedParameterValue);
			}
		}
	}

    // Validate the channel settings. Note the channels are 1-based in parameter names.
    for (int chan = 0; chan<PX4RCCalibration::_chanMax; chan++) {
        int oneBasedChannel = chan + 1;
        int rcMinExpected, rcMaxExpected, rcTrimExpected;
        bool convertOk;
        
        // Required channels have min/max set on them. Remaining channels are left to default.
		rcMinExpected = _rgChannelSettingsPostValidate[chan].rcMin;
		rcMaxExpected = _rgChannelSettingsPostValidate[chan].rcMax;
		rcTrimExpected = _rgChannelSettingsPostValidate[chan].rcTrim;

        if (validateMask & validateMinMaxMask) {
            QCOMPARE(mapParamsSet.contains(minTpl.arg(oneBasedChannel)), true);
            QCOMPARE(mapParamsSet.contains(maxTpl.arg(oneBasedChannel)), true);
            QCOMPARE(mapParamsSet[minTpl.arg(oneBasedChannel)].toInt(&convertOk), rcMinExpected);
            QCOMPARE(convertOk, true);
            QCOMPARE(mapParamsSet[maxTpl.arg(oneBasedChannel)].toInt(&convertOk), rcMaxExpected);
            QCOMPARE(convertOk, true);
        }
        
        if (validateMask & validateTrimsMask) {
            QCOMPARE(mapParamsSet.contains(trimTpl.arg(oneBasedChannel)), true);
            QCOMPARE(mapParamsSet[trimTpl.arg(oneBasedChannel)].toInt(&convertOk), rcTrimExpected);
            QCOMPARE(convertOk, true);
        }

        if (validateMask & validateReversedMask) {
			float revValue = _rgChannelSettingsPostValidate[chan].reversed ? -1.0f : 1.0f;
			
			//qDebug() << "Reverse validate" << chan << mapParamsSet[revTpl.arg(oneBasedChannel)].toFloat() << revValue;
            QCOMPARE(mapParamsSet.contains(revTpl.arg(oneBasedChannel)), true);
            QCOMPARE(mapParamsSet[revTpl.arg(oneBasedChannel)].toFloat(&convertOk), revValue);
            QCOMPARE(convertOk, true);
        }
    }
    
    if (validateMask & validateMappingMask) {
        // Check mapping for all fuctions
        for (int chanFunction=0; chanFunction<PX4RCCalibration::rcCalFunctionMax; chanFunction++) {
            QCOMPARE(mapParamsSet.contains(PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName), true);

            int expectedValue;
            if (_rgFunctionChannelMap[chanFunction] == -1) {
				expectedValue = 0;  // 0 signals no mapping
            } else {
				expectedValue = _rgFunctionChannelMap[chanFunction] + 1; // 1-based
            }
			// qDebug() << chanFunction << expectedValue << mapParamsSet[PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName].toInt();
            QCOMPARE(mapParamsSet[PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName].toInt(), expectedValue);
        }
    }
}

void PX4RCCalibrationTest::_validateWidgets(int validateMask, const struct ChannelSettings* rgChannelSettings)
{
    // Radio channel widgets should be displaying the current min/max we just set. Wait a bit for ui to update before checking.
    
    QTest::qWait(PX4RCCalibration::_updateInterval * 2);
    
    for (int chan=0; chan<_availableChannels; chan++) {
        RCChannelWidget* radioWidget = _rgRadioWidget[chan];
        Q_ASSERT(radioWidget);
		
        if (validateMask & validateMinMaxMask) {
			//qDebug() << chan;
            QCOMPARE(radioWidget->isMinMaxShown(), rgChannelSettings[chan].isMinMaxShown);
            QCOMPARE(radioWidget->min(), rgChannelSettings[chan].rcMin);
            QCOMPARE(radioWidget->max(), rgChannelSettings[chan].rcMax);
            QCOMPARE(radioWidget->isMinValid(), rgChannelSettings[chan].isMinValid);
            QCOMPARE(radioWidget->isMaxValid(), rgChannelSettings[chan].isMaxValid);
        }
        
        if (validateMask & validateTrimsMask) {
			//qDebug() << "Trim validate widget" << chan;
            QCOMPARE(radioWidget->trim(), rgChannelSettings[chan].rcTrim);
        }
    }
    
    for (int chan=_availableChannels; chan<PX4RCCalibration::_chanMax; chan++) {
        QCOMPARE(_rgRadioWidget[chan]->isEnabled(), false);
    }
}