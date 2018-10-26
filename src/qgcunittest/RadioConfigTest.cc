/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "RadioConfigTest.h"
#include "RadioComponentController.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "PX4/PX4AutoPilotPlugin.h"
#include "APM/APMAutoPilotPlugin.h"
#include "APM/APMRadioComponent.h"
#include "PX4RadioComponent.h"

/// @file
///     @brief QRadioComponentController Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

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

const struct RadioConfigTest::ChannelSettings RadioConfigTest::_rgChannelSettingsPX4[RadioComponentController::_chanMaxPX4] = {
    // Function										Min                 Max                 #  Reversed

    // Channel 0 : Not mapped to function, Simulate invalid Min/Max
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,	_testCenterValue,   0, false },

// Channels 1-4: Mapped to attitude control function
{ RadioComponentController::rcCalFunctionRoll,			_testMinValue,      _testMaxValue,      0, true },
{ RadioComponentController::rcCalFunctionPitch,			_testMinValue,      _testMaxValue,      0, false },
{ RadioComponentController::rcCalFunctionYaw,			_testMinValue,      _testMaxValue,      0, true },
{ RadioComponentController::rcCalFunctionThrottle,		_testMinValue,      _testMaxValue,      0,  false },

// Channels 5-11: Not mapped to function, Simulate invalid Min/Max, since available channel Min/Max is still shown.
// These are here to skip over the flight mode functions
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },

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

// Note the: 1500/*RadioComponentController::_rcCalPWMCenterPoint*/ entries. For some reason I couldn't get the compiler to do the
// right thing with the constant. So I just hacked inthe real value instead of fighting with it any longer.
const struct RadioConfigTest::ChannelSettings RadioConfigTest::_rgChannelSettingsValidatePX4[RadioComponentController::_chanMaxPX4] = {
    // Function										Min Value									Max Value									Trim Value										Reversed

    // Channels 0: not mapped and should be set to defaults
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },

// Channels 1-4: Mapped to attitude control function
{ RadioComponentController::rcCalFunctionRoll,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               true },
{ RadioComponentController::rcCalFunctionPitch,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               false },
{ RadioComponentController::rcCalFunctionYaw,			_testMinValue,                              _testMaxValue,                              _testCenterValue,                               true },
{ RadioComponentController::rcCalFunctionThrottle,		_testMinValue,                              _testMaxValue,                              _testMinValue,                                  false },

// Channels 5-11: not mapped and should be set to defaults
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },

// Channels 12-17 are not mapped and should be set to defaults
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
};

const struct RadioConfigTest::ChannelSettings RadioConfigTest::_rgChannelSettingsAPM[RadioComponentController::_chanMaxAPM] = {
    // Function										Min                 Max                 #  Reversed

    // Channel 0 : Not mapped to function, Simulate invalid Min/Max
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,	_testCenterValue,   0, false },

// Channels 1-4: Mapped to attitude control function
{ RadioComponentController::rcCalFunctionRoll,			_testMinValue,      _testMaxValue,      0, true },
{ RadioComponentController::rcCalFunctionPitch,			_testMinValue,      _testMaxValue,      0, false },
{ RadioComponentController::rcCalFunctionYaw,			_testMinValue,      _testMaxValue,      0, true },
{ RadioComponentController::rcCalFunctionThrottle,		_testMinValue,      _testMaxValue,      0,  false },

// Channels 5-11: Not mapped to function, Simulate invalid Min/Max, since available channel Min/Max is still shown.
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,   _testCenterValue,   0,	false },

// Channel 12 : Not mapped to function, Simulate invalid Min, valid Max
{ RadioComponentController::rcCalFunctionMax,			_testCenterValue,	_testMaxValue,      0,  false },

// Channel 13 : Not mapped to function, Simulate valid Min, invalid Max
{ RadioComponentController::rcCalFunctionMax,           _testMinValue,      _testCenterValue,   0,	false },
};

// Note the: 1500/*RadioComponentController::_rcCalPWMCenterPoint*/ entries. For some reason I couldn't get the compiler to do the
// right thing with the constant. So I just hacked inthe real value instead of fighting with it any longer.
const struct RadioConfigTest::ChannelSettings RadioConfigTest::_rgChannelSettingsValidateAPM[RadioComponentController::_chanMaxAPM] = {
    // Function										Min Value									Max Value									Trim Value										Reversed

    // Channels 0: not mapped and should be set to defaults
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },

// Channels 1-4: Mapped to attitude control function
{ RadioComponentController::rcCalFunctionRoll,			_testMinValue, _testMaxValue, _testCenterValue, true },
{ RadioComponentController::rcCalFunctionPitch,			_testMinValue, _testMaxValue, _testCenterValue, true },
{ RadioComponentController::rcCalFunctionYaw,			_testMinValue, _testMaxValue, _testCenterValue, true },
{ RadioComponentController::rcCalFunctionThrottle,		_testMinValue, _testMaxValue, _testMinValue,    false },

// Channels 5-11: not mapped and should be set to defaults
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },

// Channels 12-13 are not mapped and should be set to defaults
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
{ RadioComponentController::rcCalFunctionMax,			RadioComponentController::_rcCalPWMDefaultMinValue,	RadioComponentController::_rcCalPWMDefaultMaxValue,	1500/*RadioComponentController::_rcCalPWMCenterPoint*/,         false },
};

RadioConfigTest::RadioConfigTest(void) :
    _calWidget(NULL),
    _controller(NULL)
{
    
}

void RadioConfigTest::_init(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);
    
    _autopilot = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->autopilotPlugin();
    Q_ASSERT(_autopilot);

    // This test is so quick that it tends to finish before the mission item protocol completes. This causes an error to pop up.
    // So we wait a little to let mission items sync.
    QTest::qWait(500);
    
    // This will instatiate the widget with an active uas with ready parameters
    _calWidget = new QGCQmlWidgetHolder(QString(), NULL);
    _calWidget->resize(600, 600);
    Q_CHECK_PTR(_calWidget);
    _calWidget->setAutoPilot(_autopilot);

    // Find the radio component
    QObject* vehicleComponent = NULL;
    for (const QVariant& varVehicleComponent: _autopilot->vehicleComponents()) {
        if (firmwareType == MAV_AUTOPILOT_PX4) {
            PX4RadioComponent* radioComponent = qobject_cast<PX4RadioComponent*>(varVehicleComponent.value<VehicleComponent*>());
            if (radioComponent) {
                vehicleComponent = radioComponent;
                break;
            }
        } else {
            APMRadioComponent* radioComponent = qobject_cast<APMRadioComponent*>(varVehicleComponent.value<VehicleComponent*>());
            if (radioComponent) {
                vehicleComponent = radioComponent;
                break;
            }
        }
    }
    Q_CHECK_PTR(vehicleComponent);

    _calWidget->setContextPropertyObject("vehicleComponent", vehicleComponent);
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
    
    UnitTest::cleanup();
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
    
    bool reversed = _channelSettings()[channel].reversed;
    
    if (!identifyStep && direction != moveToCenter) {
        // We have already identified the function channel mapping. Move other channels around to make sure there is no impact.
        
        int otherChannel = channel + 1;
        if (otherChannel >= _chanMax()) {
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
    for (int chan=0; chan<_chanMax(); chan++) {
        _mockLink->emitRemoteControlChannelRawChanged(chan, _channelSettings()[chan].rcMin);
        _mockLink->emitRemoteControlChannelRawChanged(chan, _channelSettings()[chan].rcMax);
    }
    
    _channelHomePosition();

    int saveStep = _controller->_currentStep;
    _controller->nextButtonClicked();
    QCOMPARE(_controller->_currentStep, saveStep + 1);
}

void RadioConfigTest::_fullCalibrationWorker(MAV_AUTOPILOT firmwareType)
{
    _init(firmwareType);

    // IMPORTANT NOTE: We used channels 1-5 for attitude mapping in the test below.
    // MockLink.params file cannot have flight mode switches mapped to those channels.
    // If it does it will cause errors since the stick will not be detetected where

    /// _rgFunctionChannelMap maps from function index to channel index. For channels which are not part of
    /// rc cal set the mapping the the previous mapping.
    
    for (int function=0; function<RadioComponentController::rcCalFunctionMax; function++) {
        bool found = false;
        
        // If we are mapping this function during cal set it into _rgFunctionChannelMap
        for (int channel=0; channel<_chanMax(); channel++) {
            if (_channelSettings()[channel].function == function) {
                
                if (_px4Vehicle()) {
                    // Make sure this function isn't being use for a switch
                    QStringList switchList;
                    switchList << "RC_MAP_MODE_SW" << "RC_MAP_LOITER_SW" << "RC_MAP_RETURN_SW" << "RC_MAP_POSCTL_SW" << "RC_MAP_ACRO_SW";

                    for (const QString &switchParam: switchList) {
                        Q_ASSERT(_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, switchParam)->rawValue().toInt() != channel + 1);
                    }
                }
                
                _rgFunctionChannelMap[function] = channel;
                found = true;
                
                break;
            }
        }
        
        // If we aren't mapping this function during calibration, set it to the previous setting
        if (!found) {
            const char* paramName = _functionInfo()[function].parameterName;
            if (paramName) {
                _rgFunctionChannelMap[function] = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName)->rawValue().toInt();
                qCDebug(RadioConfigTestLog) << "Assigning switch" << function << _rgFunctionChannelMap[function];
                if (_rgFunctionChannelMap[function] == 0) {
                    _rgFunctionChannelMap[function] = -1;   // -1 signals no mapping
                } else {
                    _rgFunctionChannelMap[function]--;   // parameter is 1-based, _rgFunctionChannelMap is not
                }
            } else {
                _rgFunctionChannelMap[function] = -1;   // -1 signals no mapping
            }
        }
    }
    
    _channelHomePosition();
    _controller->nextButtonClicked();
    _beginCalibration();
    _stickMoveAutoStep("Throttle",  RadioComponentController::rcCalFunctionThrottle,    moveToMax,      true /* identify step */);
    _stickMoveAutoStep("Throttle",  RadioComponentController::rcCalFunctionThrottle,    moveToMin,      false /* not identify step */);
    _stickMoveAutoStep("Yaw",       RadioComponentController::rcCalFunctionYaw,         moveToMax,      true /* identify step */);
    _stickMoveAutoStep("Yaw",       RadioComponentController::rcCalFunctionYaw,         moveToMin,      false /* not identify step */);
    _stickMoveAutoStep("Roll",      RadioComponentController::rcCalFunctionRoll,        moveToMax,      true /* identify step */);
    _stickMoveAutoStep("Roll",      RadioComponentController::rcCalFunctionRoll,        moveToMin,      false /* not identify step */);
    _stickMoveAutoStep("Pitch",     RadioComponentController::rcCalFunctionPitch,       moveToMax,      true /* identify step */);
    _stickMoveAutoStep("Pitch",     RadioComponentController::rcCalFunctionPitch,       moveToMin,      false /* not identify step */);
    _stickMoveAutoStep("Pitch",     RadioComponentController::rcCalFunctionPitch,       moveToCenter,   false /* not identify step */);
    _switchMinMaxStep();

    // One more click and the parameters should get saved
    _controller->nextButtonClicked();
    _validateParameters();
}

void RadioConfigTest::_fullCalibration_px4_test(void)
{
    _fullCalibrationWorker(MAV_AUTOPILOT_PX4);
}


void RadioConfigTest::_fullCalibration_apm_test(void)
{
    _fullCalibrationWorker(MAV_AUTOPILOT_ARDUPILOTMEGA);
}

/// @brief Sets rc input to Throttle down home position. Centers all other channels.
void RadioConfigTest::_channelHomePosition(void)
{
    // Initialize available channels to center point.
    for (int i=0; i<_chanMax(); i++) {
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

    QString revTplPX4("RC%1_REV");
    QString revTplAPM("RC%1_REVERSED");

    // Check mapping for all fuctions
    for (int chanFunction=0; chanFunction<RadioComponentController::rcCalFunctionMax; chanFunction++) {
        int chanIndex = _rgFunctionChannelMap[chanFunction];
        
        int expectedParameterValue;
        if (chanIndex == -1) {
            expectedParameterValue = 0;  // 0 signals no mapping
        } else {
            expectedParameterValue = chanIndex + 1; // 1-based parameter value
        }
        
        const char* paramName = _functionInfo()[chanFunction].parameterName;
        if (paramName) {
            qCDebug(RadioConfigTestLog) << "Validate" << chanFunction << _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName)->rawValue().toInt();
            QCOMPARE(_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName)->rawValue().toInt(), expectedParameterValue);
        }
    }

    // Validate the channel settings. Note the channels are 1-based in parameter names.
    for (int chan = 0; chan<_chanMax(); chan++) {
        int oneBasedChannel = chan + 1;
        bool convertOk;
        
        // Required channels have min/max set on them. Remaining channels are left to default.
        int rcMinExpected = _channelSettingsValidate()[chan].rcMin;
        int rcMaxExpected = _channelSettingsValidate()[chan].rcMax;
        int rcTrimExpected = _channelSettingsValidate()[chan].rcTrim;
        bool rcReversedExpected = _channelSettingsValidate()[chan].reversed;

        int rcMinActual = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, minTpl.arg(oneBasedChannel))->rawValue().toInt(&convertOk);
        QCOMPARE(convertOk, true);
        int rcMaxActual = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, maxTpl.arg(oneBasedChannel))->rawValue().toInt(&convertOk);
        QCOMPARE(convertOk, true);
        int rcTrimActual = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, trimTpl.arg(oneBasedChannel))->rawValue().toInt(&convertOk);
        QCOMPARE(convertOk, true);

        bool rcReversedActual;
        if (_vehicle->px4Firmware()) {
            float rcReversedFloat = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, revTplPX4.arg(oneBasedChannel))->rawValue().toFloat(&convertOk);
            QCOMPARE(convertOk, true);
            rcReversedActual = (rcReversedFloat == -1.0f);
        } else {
            rcReversedActual = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, revTplAPM.arg(oneBasedChannel))->rawValue().toBool();
        }
        
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
        const char* paramName = _functionInfo()[chanFunction].parameterName;
        if (paramName) {
            // qCDebug(RadioConfigTestLog) << chanFunction << expectedValue << mapParamsSet[paramName].toInt();
            QCOMPARE(_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName)->rawValue().toInt(), expectedValue);
        }
    }
}

bool RadioConfigTest::_px4Vehicle(void) const
{
    return _vehicle->firmwareType() == MAV_AUTOPILOT_PX4;
}

const struct RadioConfigTest::ChannelSettings* RadioConfigTest::_channelSettings(void) const
{
    return _px4Vehicle() ? _rgChannelSettingsPX4 : _rgChannelSettingsAPM;
}

const struct RadioConfigTest::ChannelSettings* RadioConfigTest::_channelSettingsValidate(void) const
{
    return _px4Vehicle() ? _rgChannelSettingsValidatePX4 : _rgChannelSettingsValidateAPM;
}

const struct RadioComponentController::FunctionInfo* RadioConfigTest::_functionInfo(void) const
{
    return _px4Vehicle() ? RadioComponentController::_rgFunctionInfoPX4 : RadioComponentController::_rgFunctionInfoAPM;
}

int RadioConfigTest::_chanMax(void) const
{
    return _px4Vehicle() ? RadioComponentController::_chanMaxPX4 : RadioComponentController::_chanMaxAPM;
}
