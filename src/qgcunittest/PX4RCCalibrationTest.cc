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
#include "RadioComponentController.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"

/// @file
///     @brief QRadioComponentController Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

UT_REGISTER_TEST(RadioConfigTest)
QGC_LOGGING_CATEGORY(RadioConfigTestLog, "RadioConfigTestLog")

// This will check for the wizard buttons being enabled of disabled according to the mask you pass in.
// We use a macro instead of a method so that we get better line number reporting on failure.
#define CHK_BUTTONS(mask) \
{ \
    if (_controller->_nextButton->isEnabled() != !!((mask) & nextButtonMask) || \
        _controller->_skipButton->isEnabled() != !!((mask) & skipButtonMask) || \
        _controller->_cancelButton->isEnabled() != !!((mask) & cancelButtonMask) ) { \
        qCDebug(RadioConfigTestLog) << _controller->_statusText->property("text"); \
    } \
    QCOMPARE(_controller->_nextButton->isEnabled(), !!((mask) & nextButtonMask)); \
    QCOMPARE(_controller->_skipButton->isEnabled(), !!((mask) & skipButtonMask)); \
    QCOMPARE(_controller->_cancelButton->isEnabled(), !!((mask) & cancelButtonMask)); \
}

// This allows you to write unit tests which will click the Cancel button the first time through, followed
// by the Next button on the second iteration.
#define NEXT_OR_CANCEL(cancelNum) \
{ \
    if (mode == testModeStandalone && tryCancel ## cancelNum) { \
        QTest::mouseClick(_cancelButton, Qt::LeftButton); \
        QCOMPARE(_controller->_rcCalState, RadioComponentController::rcCalStateChannelWait); \
        tryCancel ## cancelNum = false; \
        goto StartOver; \
    } else { \
        QTest::mouseClick(_nextButton, Qt::LeftButton); \
    } \
}

const int RadioConfigTest::_stickSettleWait = RadioComponentController::_stickDetectSettleMSecs * 1.5;

const int RadioConfigTest::_testMinValue = RadioComponentController::_rcCalPWMDefaultMinValue + 10;
const int RadioConfigTest::_testMaxValue = RadioComponentController::_rcCalPWMDefaultMaxValue - 10;
const int RadioConfigTest::_testCenterValue = RadioConfigTest::_testMinValue + ((RadioConfigTest::_testMaxValue - RadioConfigTest::_testMinValue) / 2);

const struct RadioConfigTest::ChannelSettings RadioConfigTest::_rgChannelSettings[RadioConfigTest::_availableChannels] = {
	// Function										Min                 Max                 #  Reversed
	
	// Channel 0 : Not mapped to function, Simulate invalid Min/Max
	{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,	_testCenterValue,   0, false },
	
    // Channels 1-4: Mapped to attitude control function
    { RadioComponentController::rcCalFunctionRoll,			_testMinValue,      _testMaxValue,      0, true },
    { RadioComponentController::rcCalFunctionPitch,			_testMinValue,      _testMaxValue,      0, false },
    { RadioComponentController::rcCalFunctionYaw,			_testMinValue,      _testMaxValue,      0, true },
    { RadioComponentController::rcCalFunctionThrottle,		_testMinValue,      _testMaxValue,      0,  false },
    
    // Channels 5-8: Not mapped to function, Simulate invalid Min/Max, since available channel Min/Max is still shown.
    // These are here to skip over the flight mode functions
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
    
    // Channels 9-11: Remainder of non-flight mode switches
    { RadioComponentController::rcCalFunctionFlaps,			_testMinValue,      _testMaxValue,      0, false },
    { RadioComponentController::rcCalFunctionAux1,			_testMinValue,      _testMaxValue,      0, false },
    { RadioComponentController::rcCalFunctionAux2,			_testMinValue,      _testMaxValue,      0, false },
	
    // Channel 12 : Not mapped to function, Simulate invalid Min, valid Max
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,	_testMaxValue,      0,  false },
    
	// Channel 13 : Not mapped to function, Simulate valid Min, invalid Max
	{ RadioComponentController::rcCalFunctionMax,           _testMinValue,      _testCenterValue,   0,	false },
	
    // Channels 14-17: Not mapped to function, Simulate invalid Min/Max, since available channel Min/Max is still shown
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
    { RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
	{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
	{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
};

// Note the: 1500/*RadioComponentController::_rcCalPWMCenterPoint*/ entires. For some reason I couldn't get the compiler to do the
// right thing with the constant. So I just hacked inthe real value instead of fighting with it any longer.
const struct RadioConfigTest::ChannelSettings RadioConfigTest::_rgChannelSettingsValidate[RadioComponentController::_chanMax] = {
    // Function										Min Value									Max Value									Trim Value										Reversed
	
    // Channels 0: not mapped and should be set to defaults
	{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
	
    // Channels 1-4: Mapped to attitude control function
	{ RadioComponentController::rcCalFunctionRoll,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               true },
    { RadioComponentController::rcCalFunctionPitch,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               false },
    { RadioComponentController::rcCalFunctionYaw,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               true },
    { RadioComponentController::rcCalFunctionThrottle,		_testMinValue,                              _testMaxValue,                              _testMinValue,                                  false },
    
    // Channels 5-8: not mapped and should be set to defaults
    { RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
    { RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
    { RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
    { RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
    
    // Channels 9-11: Remainder of non-flight mode switches
    { RadioComponentController::rcCalFunctionFlaps,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               false },
    { RadioComponentController::rcCalFunctionAux1,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               false },
    { RadioComponentController::rcCalFunctionAux2,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               false },
	
	// Channels 12-17 are not mapped and should be set to defaults
	{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
	{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
	{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
	{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
    { RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
    { RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
};

RadioConfigTest::RadioConfigTest(void) :
    _calWidget(NULL),
    _controller(NULL)
{
    
}

void RadioConfigTest::init(void)
{
    UnitTest::init();
    
    _mockLink = new MockLink();
    Q_CHECK_PTR(_mockLink);
    LinkManager::instance()->_addLink(_mockLink);
    LinkManager::instance()->connectLink(_mockLink);
    QTest::qWait(5000); // Give enough time for UI to settle and heartbeats to go through
    
    _autopilot = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(UASManager::instance()->getActiveUAS()).data();
    Q_ASSERT(_autopilot);
    
    QSignalSpy spyPlugin(_autopilot, SIGNAL(pluginReadyChanged(bool)));
    if (!_autopilot->pluginReady()) {
        QCOMPARE(spyPlugin.wait(60000), true);
    }
    Q_ASSERT(_autopilot->pluginReady());
    
    // This will instatiate the widget with an active uas with ready parameters
    _calWidget = new QGCQmlWidgetHolder();
    _calWidget->resize(600, 600);
    Q_CHECK_PTR(_calWidget);
    _calWidget->setAutoPilot(_autopilot);
    _calWidget->setSource(QUrl::fromUserInput("qrc:/qml/RadioComponent.qml"));
    
    // Nasty hack to get to controller
    _controller = RadioComponentController::_unitTestController;
    Q_ASSERT(_controller);

    _controller->_setUnitTestMode();
    
    _rgSignals[0] = SIGNAL(nextButtonMessageBoxDisplayed());
    _multiSpyNextButtonMessageBox = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyNextButtonMessageBox);
    QCOMPARE(_multiSpyNextButtonMessageBox->init(_controller, _rgSignals, 1), true);
    
    QCOMPARE(_controller->_currentStep, -1);
}

void RadioConfigTest::cleanup(void)
{
    Q_ASSERT(_calWidget);
    delete _calWidget;
    
    // Disconnecting the link will prompt for log file save
    setExpectedFileDialog(getSaveFileName, QStringList());
    
    LinkManager::instance()->disconnectLink(_mockLink);
    
    UnitTest::cleanup();
}

/// @brief Test for correct behavior in determining minimum numbers of channels for flight.
void RadioConfigTest::_minRCChannels_test(void)
{
    // Next button won't be enabled until we see the minimum number of channels.
    for (int chan=0; chan<RadioComponentController::_chanMinimum; chan++) {
        _mockLink->emitRemoteControlChannelRawChanged(chan, (float)RadioComponentController::_rcCalPWMCenterPoint);
        
        // We use _chanCount internally so we should validate it
        QCOMPARE(_controller->_chanCount, chan+1);
        
        // Validate Next button state
        _controller->nextButtonClicked();
        bool signalFound = _multiSpyNextButtonMessageBox->waitForSignalByIndex(0, 200);
        if (chan == RadioComponentController::_chanMinimum - 1) {
            // Last channel should trigger Calibration available
            QCOMPARE(signalFound, false);
            QCOMPARE(_controller->_currentStep, 0);
        } else {
            // Still less than the minimum channels. Next button should show message box. Calibration should not start.
            QCOMPARE(signalFound, true);
            QCOMPARE(_controller->_currentStep, -1);
        }
        _multiSpyNextButtonMessageBox->clearAllSignals();

        // The following test code no longer works since view update doesn't happens until parameters are received.
        // Leaving code here because RC Cal could be restructured to handle this case at some point.
#if 0
        // Only available channels should have visible widget. A ui update cycle needs to have passed so we wait a little.
        QTest::qWait(RadioComponentController::_updateInterval * 2);
        for (int chanWidget=0; chanWidget<RadioComponentController::_chanMax; chanWidget++) {
            qCDebug(RadioConfigTestLog) << _rgValueWidget[chanWidget]->objectName() << chanWidget << chan;
            QCOMPARE(_rgValueWidget[chanWidget]->isVisible(), !!(chanWidget <= chan));
        }
#endif
    }
}

void RadioConfigTest::_beginCalibration(void)
{
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);

    // We should already have enough channels to proceed with calibration. Click next to start the process.
    
    _controller->nextButtonClicked();
    QCOMPARE(_controller->_currentStep, 1);
    CHK_BUTTONS(cancelButtonMask);
}

void RadioConfigTest::_stickMoveWaitForSettle(int channel, int value)
{
    qCDebug(RadioConfigTestLog) << "_stickMoveWaitForSettle channel:value" << channel << value;

    // Move the stick, this will initialized the settle checker
    _mockLink->emitRemoteControlChannelRawChanged(channel, value);
    
    // Emit the signal again to start the settle timer
    _mockLink->emitRemoteControlChannelRawChanged(channel, value);
    
    // Wait long enough for the settle timer to expire
    QTest::qWait(RadioComponentController::_stickDetectSettleMSecs * 1.5);
    
    // Emit the signal again so that we detect stick settle
    _mockLink->emitRemoteControlChannelRawChanged(channel, value);
}

void RadioConfigTest::_stickMoveAutoStep(const char* functionStr, enum RadioComponentController::rcCalFunctions function, enum RadioConfigTest::MoveToDirection direction, bool identifyStep)
{
    Q_UNUSED(functionStr);
    qCDebug(RadioConfigTestLog) << "_stickMoveAutoStep function:direction:reversed:identifyStep" << functionStr << function << direction << identifyStep;
    
    CHK_BUTTONS(cancelButtonMask);
    
    int channel = _rgFunctionChannelMap[function];
    int saveStep = _controller->_currentStep;
    
    bool reversed = _rgChannelSettings[channel].reversed;
    
    if (!identifyStep && direction != moveToCenter) {
        // We have already identified the function channel mapping. Move other channels around to make sure there is no impact.
        
        int otherChannel = channel + 1;
        if (otherChannel >= RadioComponentController::_chanMax) {
            otherChannel = 0;
        }
        
        _stickMoveWaitForSettle(otherChannel, _testMinValue);
        QCOMPARE(_controller->_currentStep, saveStep);
        CHK_BUTTONS(cancelButtonMask);
        
        _stickMoveWaitForSettle(otherChannel, RadioComponentController::_rcCalPWMCenterPoint);
        QCOMPARE(_controller->_currentStep, saveStep);
        CHK_BUTTONS(cancelButtonMask);
    }
    
    // Move channel to specified position to trigger next step
    
    int value;
    if (direction == moveToMin) {
        value = reversed ? _testMaxValue : _testMinValue;
    } else if (direction == moveToMax) {
        value = reversed ? _testMinValue : _testMaxValue;
    } else if (direction == moveToCenter) {
        value = RadioComponentController::_rcCalPWMCenterPoint;
    } else {
        Q_ASSERT(false);
    }
    
    _stickMoveWaitForSettle(channel, value);
    QCOMPARE(_controller->_currentStep, saveStep + 1);
}

void RadioConfigTest::_switchMinMaxStep(void)
{
    CHK_BUTTONS(nextButtonMask | cancelButtonMask);
    
    // Try setting a min/max value that is below the threshold to make sure min/max doesn't go valid
    _mockLink->emitRemoteControlChannelRawChanged(0, (float)(RadioComponentController::_rcCalPWMValidMinValue + 1));
    _mockLink->emitRemoteControlChannelRawChanged(0, (float)(RadioComponentController::_rcCalPWMValidMaxValue - 1));
    
    // Send min/max values switch channels
    for (int chan=0; chan<_availableChannels; chan++) {
        _mockLink->emitRemoteControlChannelRawChanged(chan, _rgChannelSettings[chan].rcMin);
        _mockLink->emitRemoteControlChannelRawChanged(chan, _rgChannelSettings[chan].rcMax);
    }
    
    _channelHomePosition();

    int saveStep = _controller->_currentStep;
    _controller->nextButtonClicked();
    QCOMPARE(_controller->_currentStep, saveStep + 1);
}

void RadioConfigTest::_flapsDetectStep(void)
{
    int channel = _rgFunctionChannelMap[RadioComponentController::rcCalFunctionFlaps];
    
    qCDebug(RadioConfigTestLog) << "_flapsDetectStep channel" << channel;
    
    // Test code can't handle reversed flaps channel
    Q_ASSERT(!_rgChannelSettings[channel].reversed);
    
    CHK_BUTTONS(nextButtonMask | cancelButtonMask | skipButtonMask);
    
    int saveStep = _controller->_currentStep;

    // Wiggle channel to identify
    _stickMoveWaitForSettle(channel, _testMaxValue);
    _stickMoveWaitForSettle(channel, _testMinValue);
    
    // Leave channel on full flaps down
    _stickMoveWaitForSettle(channel, _testMaxValue);
    
    // User has to hit next at this step
    QCOMPARE(_controller->_currentStep, saveStep);
    CHK_BUTTONS(nextButtonMask | cancelButtonMask | skipButtonMask);
    _controller->nextButtonClicked();
    QCOMPARE(_controller->_currentStep, saveStep + 1);
}

void RadioConfigTest::_switchSelectAutoStep(const char* functionStr, RadioComponentController::rcCalFunctions function)
{
    Q_UNUSED(functionStr);
    ////qCDebug(RadioConfigTestLog)() << "_switchSelectAutoStep" << functionStr << "function:" << function;
    
    int buttonMask = cancelButtonMask;
    if (function != RadioComponentController::rcCalFunctionModeSwitch) {
        buttonMask |= skipButtonMask;
    }
    
    CHK_BUTTONS(buttonMask);
    
    int saveStep = _controller->_currentStep;
    
    // Wiggle stick for channel
    int channel = _rgFunctionChannelMap[function];
    _mockLink->emitRemoteControlChannelRawChanged(channel, _testMinValue);
    _mockLink->emitRemoteControlChannelRawChanged(channel, _testMaxValue);
    
    QCOMPARE(_controller->_currentStep, saveStep + 1);
}

void RadioConfigTest::_fullCalibration_test(void)
{
    // IMPORTANT NOTE: We used channels 1-5 for attitude mapping in the test below.
    // MockLink.params file cannot have flight mode switches mapped to those channels.
    // If it does it will cause errors since the stick will not be detetected where

    /// _rgFunctionChannelMap maps from function index to channel index. For channels which are not part of
    /// rc cal set the mapping the the previous mapping.
    
    for (int function=0; function<RadioComponentController::rcCalFunctionMax; function++) {
        bool found = false;
        
        // If we are mapping this function during cal set it into _rgFunctionChannelMap
        for (int channel=0; channel<RadioConfigTest::_availableChannels; channel++) {
            if (_rgChannelSettings[channel].function == function) {
                
                // First make sure this function isn't being use for a switch
                
                QStringList switchList;
                switchList << "RC_MAP_MODE_SW" << "RC_MAP_LOITER_SW" << "RC_MAP_RETURN_SW" << "RC_MAP_POSCTL_SW" << "RC_MAP_ACRO_SW";
                
                foreach (QString switchParam, switchList) {
                    Q_ASSERT(_autopilot->getParameterFact(FactSystem::defaultComponentId, switchParam)->value().toInt() != channel + 1);
                }
                
                _rgFunctionChannelMap[function] = channel;
                found = true;
                
                break;
            }
        }
        
        // If we aren't mapping this function during calibration, set it to the previous setting
        if (!found) {
            _rgFunctionChannelMap[function] = _autopilot->getParameterFact(FactSystem::defaultComponentId, RadioComponentController::_rgFunctionInfo[function].parameterName)->value().toInt();
            qCDebug(RadioConfigTestLog) << "Assigning switch" << function << _rgFunctionChannelMap[function];
            if (_rgFunctionChannelMap[function] == 0) {
                _rgFunctionChannelMap[function] = -1;   // -1 signals no mapping
            } else {
                _rgFunctionChannelMap[function]--;   // parameter is 1-based, _rgFunctionChannelMap is not
            }
        }
    }
    
    _channelHomePosition();
    _controller->nextButtonClicked();
    _beginCalibration();
    _stickMoveAutoStep("Throttle", RadioComponentController::rcCalFunctionThrottle, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Throttle", RadioComponentController::rcCalFunctionThrottle, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Yaw", RadioComponentController::rcCalFunctionYaw, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Yaw", RadioComponentController::rcCalFunctionYaw, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Roll", RadioComponentController::rcCalFunctionRoll, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Roll", RadioComponentController::rcCalFunctionRoll, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Pitch", RadioComponentController::rcCalFunctionPitch, moveToMax, true /* identify step */);
    _stickMoveAutoStep("Pitch", RadioComponentController::rcCalFunctionPitch, moveToMin, false /* not identify step */);
    _stickMoveAutoStep("Pitch", RadioComponentController::rcCalFunctionPitch, moveToCenter, false /* not identify step */);
    _switchMinMaxStep();
    _flapsDetectStep();
    _stickMoveAutoStep("Flaps", RadioComponentController::rcCalFunctionFlaps, moveToMin, false /* not identify step */);
    _switchSelectAutoStep("Aux1", RadioComponentController::rcCalFunctionAux1);
    _switchSelectAutoStep("Aux2", RadioComponentController::rcCalFunctionAux2);

    // One more click and the parameters should get saved
    _controller->nextButtonClicked();
    _validateParameters();
}

/// @brief Sets rc input to Throttle down home position. Centers all other channels.
void RadioConfigTest::_channelHomePosition(void)
{
    // Initialize available channels to center point.
    for (int i=0; i<_availableChannels; i++) {
        _mockLink->emitRemoteControlChannelRawChanged(i, (float)RadioComponentController::_rcCalPWMCenterPoint);
    }
    
    // Throttle to min - 1 (throttle is not reversed). We do this so that the trim value is below the min value. This should end up
    // being validated and raised to min value. If not, something is wrong with RC Cal code.
    _mockLink->emitRemoteControlChannelRawChanged(_rgFunctionChannelMap[RadioComponentController::rcCalFunctionThrottle], _testMinValue - 1);
}

void RadioConfigTest::_validateParameters(void)
{
    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
    // Check mapping for all fuctions
    for (int chanFunction=0; chanFunction<RadioComponentController::rcCalFunctionMax; chanFunction++) {
        int chanIndex = _rgFunctionChannelMap[chanFunction];
        
        int expectedParameterValue;
        if (chanIndex == -1) {
            expectedParameterValue = 0;  // 0 signals no mapping
        } else {
            expectedParameterValue = chanIndex + 1; // 1-based parameter value
        }
        
        qCDebug(RadioConfigTestLog) << "Validate" << chanFunction << _autopilot->getParameterFact(FactSystem::defaultComponentId, RadioComponentController::_rgFunctionInfo[chanFunction].parameterName)->value().toInt();
        QCOMPARE(_autopilot->getParameterFact(FactSystem::defaultComponentId, RadioComponentController::_rgFunctionInfo[chanFunction].parameterName)->value().toInt(), expectedParameterValue);
    }

    // Validate the channel settings. Note the channels are 1-based in parameter names.
    for (int chan = 0; chan<RadioComponentController::_chanMax; chan++) {
        int oneBasedChannel = chan + 1;
        bool convertOk;
        
        // Required channels have min/max set on them. Remaining channels are left to default.
		int rcMinExpected = _rgChannelSettingsValidate[chan].rcMin;
		int rcMaxExpected = _rgChannelSettingsValidate[chan].rcMax;
		int rcTrimExpected = _rgChannelSettingsValidate[chan].rcTrim;
        bool rcReversedExpected = _rgChannelSettingsValidate[chan].reversed;

        int rcMinActual = _autopilot->getParameterFact(FactSystem::defaultComponentId, minTpl.arg(oneBasedChannel))->value().toInt(&convertOk);
        QCOMPARE(convertOk, true);
        int rcMaxActual = _autopilot->getParameterFact(FactSystem::defaultComponentId, maxTpl.arg(oneBasedChannel))->value().toInt(&convertOk);
        QCOMPARE(convertOk, true);
        int rcTrimActual = _autopilot->getParameterFact(FactSystem::defaultComponentId, trimTpl.arg(oneBasedChannel))->value().toInt(&convertOk);
        QCOMPARE(convertOk, true);
        float rcReversedFloat = _autopilot->getParameterFact(FactSystem::defaultComponentId, revTpl.arg(oneBasedChannel))->value().toFloat(&convertOk);
        QCOMPARE(convertOk, true);
        bool rcReversedActual = (rcReversedFloat == -1.0f);
        
        qCDebug(RadioConfigTestLog) << "_validateParameters expected channel:min:max:trim:rev" << chan << rcMinExpected << rcMaxExpected << rcTrimExpected << rcReversedExpected;
        qCDebug(RadioConfigTestLog) << "_validateParameters actual channel:min:max:trim:rev" << chan << rcMinActual << rcMaxActual << rcTrimActual << rcReversedActual;

        QCOMPARE(rcMinExpected, rcMinActual);
        QCOMPARE(rcMaxExpected, rcMaxActual);
        QCOMPARE(rcTrimExpected, rcTrimActual);
        QCOMPARE(rcReversedExpected, rcReversedActual);
    }
    
    // Check mapping for all fuctions
    for (int chanFunction=0; chanFunction<RadioComponentController::rcCalFunctionMax; chanFunction++) {
        int expectedValue;
        if (_rgFunctionChannelMap[chanFunction] == -1) {
            expectedValue = 0;  // 0 signals no mapping
        } else {
            expectedValue = _rgFunctionChannelMap[chanFunction] + 1; // 1-based
        }
        // qCDebug(RadioConfigTestLog) << chanFunction << expectedValue << mapParamsSet[RadioComponentController::_rgFunctionInfo[chanFunction].parameterName].toInt();
        QCOMPARE(_autopilot->getParameterFact(FactSystem::defaultComponentId, RadioComponentController::_rgFunctionInfo[chanFunction].parameterName)->value().toInt(), expectedValue);
    }
}
