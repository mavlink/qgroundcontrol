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

UT_REGISTER_TEST(PX4RCCalibrationTest)

// This will check for the wizard buttons being enabled of disabled according to the mask you pass in.
// We use a macro instead of a method so that we get better line number reporting on failure.
#define CHK_BUTTONS(mask) \
{ \
    if (_nextButton->isEnabled() != !!((mask) & nextButtonMask) || \
        _skipButton->isEnabled() != !!((mask) & skipButtonMask) || \
        _cancelButton->isEnabled() != !!((mask) & cancelButtonMask) ) { \
        qDebug() << _statusLabel->text(); \
    } \
    QCOMPARE(_nextButton->isEnabled(), !!((mask) & nextButtonMask)); \
    QCOMPARE(_skipButton->isEnabled(), !!((mask) & skipButtonMask)); \
    QCOMPARE(_cancelButton->isEnabled(), !!((mask) & cancelButtonMask)); \
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

const int PX4RCCalibrationTest::_stickSettleWait = PX4RCCalibration::_stickDetectSettleMSecs * 1.5;

const int PX4RCCalibrationTest::_testMinValue = PX4RCCalibration::_rcCalPWMDefaultMinValue + 10;
const int PX4RCCalibrationTest::_testMaxValue = PX4RCCalibration::_rcCalPWMDefaultMaxValue - 10;
const int PX4RCCalibrationTest::_testCenterValue = PX4RCCalibrationTest::_testMinValue + ((PX4RCCalibrationTest::_testMaxValue - PX4RCCalibrationTest::_testMinValue) / 2);

/// @brief Maps from function index to channel index. -1 signals no mapping. Channel indices are offset 1 from function index
/// to catch bugs where function index is incorrectly used as channel index.
const int PX4RCCalibrationTest::_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionMax]= { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

const struct PX4RCCalibrationTest::ChannelSettings PX4RCCalibrationTest::_rgChannelSettings[PX4RCCalibrationTest::_availableChannels] = {
	// Function										Min                 Max                 #  Reversed
	
	// Channel 0 : Not mapped to function, Simulate invalid Min/Max
	{ PX4RCCalibration::rcCalFunctionMax,			_testCenterValue,	_testCenterValue,   0, false },
	
    // Channels 1-11 are mapped to all available modes
    { PX4RCCalibration::rcCalFunctionRoll,			_testMinValue,      _testMaxValue,      0, true },
    { PX4RCCalibration::rcCalFunctionPitch,			_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionYaw,			_testMinValue,      _testMaxValue,      0, true },
    { PX4RCCalibration::rcCalFunctionThrottle,		_testMinValue,      _testMaxValue,      0,  false },
    { PX4RCCalibration::rcCalFunctionModeSwitch,	_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionPosCtlSwitch,	_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionLoiterSwitch,	_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionReturnSwitch,	_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionFlaps,			_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionAux1,			_testMinValue,      _testMaxValue,      0, false },
    { PX4RCCalibration::rcCalFunctionAux2,			_testMinValue,      _testMaxValue,      0, false },
	
    // Channel 12 : Not mapped to function, Simulate invalid Min, valid Max
    { PX4RCCalibration::rcCalFunctionMax,			_testCenterValue,	_testMaxValue,      0,  false },
    
	// Channel 13 : Not mapped to function, Simulate valid Min, invalid Max
	{ PX4RCCalibration::rcCalFunctionMax,           _testMinValue,      _testCenterValue,   0,	false },
	
    // Channels 14-17: Not mapped to function, Simulate invalid Min/Max, since available channel Min/Max is still shown
    { PX4RCCalibration::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
    { PX4RCCalibration::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
	{ PX4RCCalibration::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
	{ PX4RCCalibration::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
};

const struct PX4RCCalibrationTest::ChannelSettings PX4RCCalibrationTest::_rgChannelSettingsValidate[PX4RCCalibration::_chanMax] = {
    // Function										Min Value									Max Value									Trim Value										Reversed
	
	// Channel 0 is not mapped and should be defaulted
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
	
	// Channels 1-11 are mapped to all available modes
	{ PX4RCCalibration::rcCalFunctionRoll,			_testMinValue,		_testMaxValue,        _testCenterValue, true },
    { PX4RCCalibration::rcCalFunctionPitch,			_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionYaw,			_testMinValue,		_testMaxValue,        _testCenterValue, true },
    { PX4RCCalibration::rcCalFunctionThrottle,		_testMinValue,		_testMaxValue,        _testMinValue,    false },
    { PX4RCCalibration::rcCalFunctionModeSwitch,	_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionPosCtlSwitch,	_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionLoiterSwitch,	_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionReturnSwitch,	_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionFlaps,			_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionAux1,			_testMinValue,		_testMaxValue,        _testCenterValue, false },
    { PX4RCCalibration::rcCalFunctionAux2,			_testMinValue,		_testMaxValue,        _testCenterValue, false },
	
	// Channels 12-17 are not mapped and should be set to defaults
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
	{ PX4RCCalibration::rcCalFunctionMax,			PX4RCCalibration::_rcCalPWMDefaultMinValue,	PX4RCCalibration::_rcCalPWMDefaultMaxValue,	PX4RCCalibration::_rcCalPWMCenterPoint,         false },
};

PX4RCCalibrationTest::PX4RCCalibrationTest(void) :
    _mockUASManager(NULL),
    _calWidget(NULL)
{
}

/// @brief Called one time before any test cases are run.
void PX4RCCalibrationTest::initTestCase(void)
{
    UnitTest::initTestCase();
    
	// Validate that our function to channel mapping is still correct.
	for (int function=0; function<PX4RCCalibration::rcCalFunctionMax; function++) {
		int chanIndex = _rgFunctionChannelMap[function];
		if (chanIndex != -1) {
			Q_ASSERT(_rgChannelSettings[chanIndex].function == function);
			Q_ASSERT(_rgChannelSettingsValidate[chanIndex].function == function);
		}
	}
}

void PX4RCCalibrationTest::init(void)
{
    UnitTest::init();
    
    _mockUASManager = new MockUASManager();
    Q_ASSERT(_mockUASManager);
    
    UASManager::setMockUASManager(_mockUASManager);
    
    _mockUAS = new MockUAS();
    Q_CHECK_PTR(_mockUAS);
    
    // This will instatiate the widget with no active UAS set
    _calWidget = new PX4RCCalibration();
    Q_CHECK_PTR(_calWidget);
    _calWidget->_setUnitTestMode();
    _calWidget->setVisible(true);
    
    _mockUASManager->setMockActiveUAS(_mockUAS);

    // Get pointers to the push buttons
    _cancelButton = _calWidget->findChild<QPushButton*>("rcCalCancel");
    _nextButton = _calWidget->findChild<QPushButton*>("rcCalNext");
    _skipButton = _calWidget->findChild<QPushButton*>("rcCalSkip");
    
    Q_ASSERT(_cancelButton);
    Q_ASSERT(_nextButton);
    Q_ASSERT(_skipButton);
    
    _statusLabel = _calWidget->findChild<QLabel*>("rcCalStatus");
    Q_ASSERT(_statusLabel);
 
    for (size_t i=0; i<PX4RCCalibration::_chanMax; i++) {
        QString radioWidgetName("channel%1Value");

        RCValueWidget* radioWidget = _calWidget->findChild<RCValueWidget*>(radioWidgetName.arg(i+1));
        Q_ASSERT(radioWidget);
        
        _rgValueWidget[i] = radioWidget;
    }
    
    _rgSignals[0] = SIGNAL(nextButtonMessageBoxDisplayed());
    _multiSpyNextButtonMessageBox = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyNextButtonMessageBox);
    QCOMPARE(_multiSpyNextButtonMessageBox->init(_calWidget, _rgSignals, 1), true);
    
    QCOMPARE(_calWidget->_currentStep, -1);
}

void PX4RCCalibrationTest::cleanup(void)
{
    UnitTest::cleanup();
    
    Q_ASSERT(_calWidget);
    delete _calWidget;
    
    Q_ASSERT(_mockUAS);
    delete _mockUAS;
    
    UASManager::setMockUASManager(NULL);
    
    Q_ASSERT(_mockUASManager);
    delete _mockUASManager;
    
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

/// @brief Test for correct behavior in determining minimum numbers of channels for flight.
void PX4RCCalibrationTest::_minRCChannels_test(void)
{
    // Next button won't be enabled until we see the minimum number of channels.
    for (int chan=0; chan<PX4RCCalibration::_chanMinimum; chan++) {
        _mockUAS->emitRemoteControlChannelRawChanged(chan, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
        
        // We use _chanCount internally so we should validate it
        QCOMPARE(_calWidget->_chanCount, chan+1);
        
        // Validate Next button state
        QTest::mouseClick(_nextButton, Qt::LeftButton);
        bool signalFound = _multiSpyNextButtonMessageBox->waitForSignalByIndex(0, 200);
        if (chan == PX4RCCalibration::_chanMinimum - 1) {
            // Last channel should trigger Calibration available
            QCOMPARE(signalFound, false);
            QCOMPARE(_calWidget->_currentStep, 0);
        } else {
            // Still less than the minimum channels. Next button should show message box. Calibration should not start.
            QCOMPARE(signalFound, true);
            QCOMPARE(_calWidget->_currentStep, -1);
        }
        _multiSpyNextButtonMessageBox->clearAllSignals();

        // The following test code no longer works since view update doesn't happens until parameters are received.
        // Leaving code here because RC Cal could be restructured to handle this case at some point.
#if 0
        // Only available channels should have visible widget. A ui update cycle needs to have passed so we wait a little.
        QTest::qWait(PX4RCCalibration::_updateInterval * 2);
        for (int chanWidget=0; chanWidget<PX4RCCalibration::_chanMax; chanWidget++) {
            //qDebug() << _rgValueWidget[chanWidget]->objectName() << chanWidget << chan;
            QCOMPARE(_rgValueWidget[chanWidget]->isVisible(), !!(chanWidget <= chan));
        }
#endif
    }
}

void PX4RCCalibrationTest::_beginCalibration(void)
{
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);

    // We should already have enough channels to proceed with calibration. Click next to start the process.
    
    QTest::mouseClick(_nextButton, Qt::LeftButton);
    QCOMPARE(_calWidget->_currentStep, 1);
    CHK_BUTTONS(cancelButtonMask);
}

void PX4RCCalibrationTest::_stickMoveWaitForSettle(int channel, int value)
{
    //qDebug() << "_stickMoveWaitForSettle channel:value" << channel << value;

    // Move the stick, this will initialized the settle checker
    _mockUAS->emitRemoteControlChannelRawChanged(channel, value);
    
    // Emit the signal again to start the settle timer
    _mockUAS->emitRemoteControlChannelRawChanged(channel, value);
    
    // Wait long enough for the settle timer to expire
    QTest::qWait(PX4RCCalibration::_stickDetectSettleMSecs * 1.5);
    
    // Emit the signal again so that we detect stick settle
    _mockUAS->emitRemoteControlChannelRawChanged(channel, value);
}

void PX4RCCalibrationTest::_stickMoveAutoStep(const char* functionStr, enum PX4RCCalibration::rcCalFunctions function, enum PX4RCCalibrationTest::MoveToDirection direction, bool identifyStep)
{
    Q_UNUSED(functionStr);
    //qDebug() << "_stickMoveAutoStep function:direction:reversed:identifyStep" << functionStr << function << direction << identifyStep;
    
    CHK_BUTTONS(cancelButtonMask);
    
    int channel = _rgFunctionChannelMap[function];
    int saveStep = _calWidget->_currentStep;
    
    bool reversed = _rgChannelSettings[channel].reversed;
    
    if (!identifyStep && direction != moveToCenter) {
        // We have already identified the function channel mapping. Move other channels around to make sure there is no impact.
        
        int otherChannel = channel + 1;
        if (otherChannel >= PX4RCCalibration::_chanMax) {
            otherChannel = 0;
        }
        
        _stickMoveWaitForSettle(otherChannel, _testMinValue);
        QCOMPARE(_calWidget->_currentStep, saveStep);
        CHK_BUTTONS(cancelButtonMask);
        
        _stickMoveWaitForSettle(otherChannel, PX4RCCalibration::_rcCalPWMCenterPoint);
        QCOMPARE(_calWidget->_currentStep, saveStep);
        CHK_BUTTONS(cancelButtonMask);
    }
    
    // Move channel to specified position to trigger next step
    
    int value;
    if (direction == moveToMin) {
        value = reversed ? _testMaxValue : _testMinValue;
    } else if (direction == moveToMax) {
        value = reversed ? _testMinValue : _testMaxValue;
    } else if (direction == moveToCenter) {
        value = PX4RCCalibration::_rcCalPWMCenterPoint;
    } else {
        Q_ASSERT(false);
    }
    
    _stickMoveWaitForSettle(channel, value);
    QCOMPARE(_calWidget->_currentStep, saveStep + 1);
}

void PX4RCCalibrationTest::_switchMinMaxStep(void)
{
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);
    
    // Try setting a min/max value that is below the threshold to make sure min/max doesn't go valid
    _mockUAS->emitRemoteControlChannelRawChanged(0, (float)(PX4RCCalibration::_rcCalPWMValidMinValue + 1));
    _mockUAS->emitRemoteControlChannelRawChanged(0, (float)(PX4RCCalibration::_rcCalPWMValidMaxValue - 1));
    QCOMPARE(_rgValueWidget[0]->isMinValid(), false);
    QCOMPARE(_rgValueWidget[0]->isMaxValid(), false);
    
    // Send min/max values switch channels
    for (int chan=0; chan<_availableChannels; chan++) {
        //qDebug() << chan << _rgChannelSettingsPreValidate[chan].rcMin << _rgChannelSettingsPreValidate[chan].rcMax;
        _mockUAS->emitRemoteControlChannelRawChanged(chan, _rgChannelSettings[chan].rcMin);
        _mockUAS->emitRemoteControlChannelRawChanged(chan, _rgChannelSettings[chan].rcMax);
    }
    
    _channelHomePosition();

    int saveStep = _calWidget->_currentStep;
    QTest::mouseClick(_nextButton, Qt::LeftButton);
    QCOMPARE(_calWidget->_currentStep, saveStep + 1);
}

void PX4RCCalibrationTest::_flapsDetectStep(void)
{
    int channel = _rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionFlaps];
    
    //qDebug() << "_flapsDetectStep channel" << channel;
    
    // Test code can't handle reversed flaps channel
    Q_ASSERT(!_rgChannelSettings[channel].reversed);
    
    CHK_BUTTONS(nextButtonMask | cancelButtonMask | skipButtonMask);
    
    int saveStep = _calWidget->_currentStep;

    // Wiggle channel to identify
    _stickMoveWaitForSettle(channel, _testMaxValue);
    _stickMoveWaitForSettle(channel, _testMinValue);
    
    // Leave channel on full flaps down
    _stickMoveWaitForSettle(channel, _testMaxValue);
    
    // User has to hit next at this step
    QCOMPARE(_calWidget->_currentStep, saveStep);
    CHK_BUTTONS(nextButtonMask | cancelButtonMask | skipButtonMask);
    QTest::mouseClick(_nextButton, Qt::LeftButton);
    QCOMPARE(_calWidget->_currentStep, saveStep + 1);
}

void PX4RCCalibrationTest::_switchSelectAutoStep(const char* functionStr, PX4RCCalibration::rcCalFunctions function)
{
    Q_UNUSED(functionStr);
    //qDebug() << "_switchSelectAutoStep" << functionStr << "function:" << function;
    
    int buttonMask = cancelButtonMask;
    if (function != PX4RCCalibration::rcCalFunctionModeSwitch) {
        buttonMask |= skipButtonMask;
    }
    
    CHK_BUTTONS(buttonMask);
    
    int saveStep = _calWidget->_currentStep;
    
    // Wiggle stick for channel
    int channel = _rgFunctionChannelMap[function];
    _mockUAS->emitRemoteControlChannelRawChanged(channel, _testMinValue);
    _mockUAS->emitRemoteControlChannelRawChanged(channel, _testMaxValue);
    
    QCOMPARE(_calWidget->_currentStep, saveStep + 1);
}

void PX4RCCalibrationTest::_fullCalibration_test(void)
{
    
    _channelHomePosition();
    QTest::mouseClick(_nextButton, Qt::LeftButton);    
    _beginCalibration();
    _stickMoveAutoStep("Throttle", PX4RCCalibration::rcCalFunctionThrottle, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Throttle", PX4RCCalibration::rcCalFunctionThrottle, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Yaw", PX4RCCalibration::rcCalFunctionYaw, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Yaw", PX4RCCalibration::rcCalFunctionYaw, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Roll", PX4RCCalibration::rcCalFunctionRoll, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Roll", PX4RCCalibration::rcCalFunctionRoll, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Pitch", PX4RCCalibration::rcCalFunctionPitch, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Pitch", PX4RCCalibration::rcCalFunctionPitch, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Pitch", PX4RCCalibration::rcCalFunctionPitch, moveToCenter, false /* not identify step */);
    _switchMinMaxStep();
    _flapsDetectStep();
    _stickMoveAutoStep("Flaps", PX4RCCalibration::rcCalFunctionFlaps, moveToMin, false /* not identify step */);
    _switchSelectAutoStep("Mode", PX4RCCalibration::rcCalFunctionModeSwitch);
    _switchSelectAutoStep("PostCtl", PX4RCCalibration::rcCalFunctionPosCtlSwitch);
    _switchSelectAutoStep("Loiter", PX4RCCalibration::rcCalFunctionLoiterSwitch);
    _switchSelectAutoStep("Return", PX4RCCalibration::rcCalFunctionReturnSwitch);
    _switchSelectAutoStep("Aux1", PX4RCCalibration::rcCalFunctionAux1);
    _switchSelectAutoStep("Aux2", PX4RCCalibration::rcCalFunctionAux2);

    // One more click and the parameters should get saved
    QTest::mouseClick(_nextButton, Qt::LeftButton);
    _validateParameters();
}

/// @brief Sets rc input to Throttle down home position. Centers all other channels.
void PX4RCCalibrationTest::_channelHomePosition(void)
{
    // Initialize available channels to center point.
    for (int i=0; i<_availableChannels; i++) {
        _mockUAS->emitRemoteControlChannelRawChanged(i, (float)PX4RCCalibration::_rcCalPWMCenterPoint);
    }
    
    // Throttle to low position (throttle is not reversed)/
    _mockUAS->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[PX4RCCalibration::rcCalFunctionThrottle], _testMinValue);
}

void PX4RCCalibrationTest::_validateParameters(void)
{
    MockQGCUASParamManager* paramMgr = _mockUAS->getMockQGCUASParamManager();
    MockQGCUASParamManager::ParamMap_t mapParamsSet = paramMgr->getMockSetParameters();

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
    // Check mapping for all fuctions
    for (int chanFunction=0; chanFunction<PX4RCCalibration::rcCalFunctionMax; chanFunction++) {
        int chanIndex = _rgFunctionChannelMap[chanFunction];
        
        int expectedParameterValue;
        if (chanIndex == -1) {
            expectedParameterValue = 0;  // 0 signals no mapping
        } else {
            expectedParameterValue = chanIndex + 1; // 1-based parameter value
        }
        
        QCOMPARE(mapParamsSet.contains(PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName), true);
        QCOMPARE(mapParamsSet[PX4RCCalibration::_rgFunctionInfo[chanFunction].parameterName].toInt(), expectedParameterValue);
    }

    // Validate the channel settings. Note the channels are 1-based in parameter names.
    for (int chan = 0; chan<PX4RCCalibration::_chanMax; chan++) {
        int oneBasedChannel = chan + 1;
        bool convertOk;
        
        // Required channels have min/max set on them. Remaining channels are left to default.
		int rcMinExpected = _rgChannelSettingsValidate[chan].rcMin;
		int rcMaxExpected = _rgChannelSettingsValidate[chan].rcMax;
		int rcTrimExpected = _rgChannelSettingsValidate[chan].rcTrim;
        bool rcReversedExpected = _rgChannelSettingsValidate[chan].reversed;

        QCOMPARE(mapParamsSet.contains(minTpl.arg(oneBasedChannel)), true);
        QCOMPARE(mapParamsSet.contains(maxTpl.arg(oneBasedChannel)), true);
        QCOMPARE(mapParamsSet.contains(trimTpl.arg(oneBasedChannel)), true);
        QCOMPARE(mapParamsSet.contains(revTpl.arg(oneBasedChannel)), true);
        
        int rcMinActual = mapParamsSet[minTpl.arg(oneBasedChannel)].toInt(&convertOk);
        QCOMPARE(convertOk, true);
        int rcMaxActual = mapParamsSet[maxTpl.arg(oneBasedChannel)].toInt(&convertOk);
        QCOMPARE(convertOk, true);
        int rcTrimActual = mapParamsSet[trimTpl.arg(oneBasedChannel)].toInt(&convertOk);
        QCOMPARE(convertOk, true);
        float rcReversedFloat = mapParamsSet[revTpl.arg(oneBasedChannel)].toFloat(&convertOk);
        QCOMPARE(convertOk, true);
        bool rcReversedActual = (rcReversedFloat == -1.0f);
        
        //qDebug() << "_validateParemeters expected channel:min:max:trim:rev" << chan << rcMinExpected << rcMaxExpected << rcTrimExpected << rcReversedExpected;
        //qDebug() << "_validateParemeters actual channel:min:max:trim:rev" << chan << rcMinActual << rcMaxActual << rcTrimActual << rcReversedActual;

        QCOMPARE(rcMinExpected, rcMinActual);
        QCOMPARE(rcMaxExpected, rcMaxActual);
        QCOMPARE(rcTrimExpected, rcTrimActual);
        QCOMPARE(rcReversedExpected, rcReversedActual);
    }
    
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
