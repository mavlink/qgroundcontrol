/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMSensorsComponentController.h"
#include "QGCMAVLink.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "APMAutoPilotPlugin.h"

#include <QVariant>
#include <QQmlProperty>

QGC_LOGGING_CATEGORY(APMSensorsComponentControllerLog, "APMSensorsComponentControllerLog")

APMSensorsComponentController::APMSensorsComponentController(void) :
    _statusLog(NULL),
    _progressBar(NULL),
    _compassButton(NULL),
    _accelButton(NULL),
    _nextButton(NULL),
    _cancelButton(NULL),
    _showOrientationCalArea(false),
    _magCalInProgress(false),
    _accelCalInProgress(false),
    _orientationCalDownSideDone(false),
    _orientationCalUpsideDownSideDone(false),
    _orientationCalLeftSideDone(false),
    _orientationCalRightSideDone(false),
    _orientationCalNoseDownSideDone(false),
    _orientationCalTailDownSideDone(false),
    _orientationCalDownSideVisible(false),
    _orientationCalUpsideDownSideVisible(false),
    _orientationCalLeftSideVisible(false),
    _orientationCalRightSideVisible(false),
    _orientationCalNoseDownSideVisible(false),
    _orientationCalTailDownSideVisible(false),
    _orientationCalDownSideInProgress(false),
    _orientationCalUpsideDownSideInProgress(false),
    _orientationCalLeftSideInProgress(false),
    _orientationCalRightSideInProgress(false),
    _orientationCalNoseDownSideInProgress(false),
    _orientationCalTailDownSideInProgress(false),
    _orientationCalDownSideRotate(false),
    _orientationCalUpsideDownSideRotate(false),
    _orientationCalLeftSideRotate(false),
    _orientationCalRightSideRotate(false),
    _orientationCalNoseDownSideRotate(false),
    _orientationCalTailDownSideRotate(false),
    _waitingForCancel(false)
{
    _compassCal.setVehicle(_vehicle);
    connect(&_compassCal, &APMCompassCal::vehicleTextMessage, this, &APMSensorsComponentController::_handleUASTextMessage);

    APMAutoPilotPlugin * apmPlugin = qobject_cast<APMAutoPilotPlugin*>(_vehicle->autopilotPlugin());

    _sensorsComponent = apmPlugin->sensorsComponent();
    connect(_sensorsComponent, &VehicleComponent::setupCompleteChanged, this, &APMSensorsComponentController::setupNeededChanged);
}

/// Appends the specified text to the status log area in the ui
void APMSensorsComponentController::_appendStatusLog(const QString& text)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void APMSensorsComponentController::_startLogCalibration(void)
{
    _hideAllCalAreas();
    
    connect(_uas, &UASInterface::textMessageReceived, this, &APMSensorsComponentController::_handleUASTextMessage);
    
    _compassButton->setEnabled(false);
    _accelButton->setEnabled(false);
    if (_accelCalInProgress) {
        _nextButton->setEnabled(true);
    }
    _cancelButton->setEnabled(false);
}

void APMSensorsComponentController::_startVisualCalibration(void)
{
    _compassButton->setEnabled(false);
    _accelButton->setEnabled(false);
    _cancelButton->setEnabled(true);

    _resetInternalState();
    
    _progressBar->setProperty("value", 0);
}

void APMSensorsComponentController::_resetInternalState(void)
{
    _orientationCalDownSideDone = true;
    _orientationCalUpsideDownSideDone = true;
    _orientationCalLeftSideDone = true;
    _orientationCalRightSideDone = true;
    _orientationCalTailDownSideDone = true;
    _orientationCalNoseDownSideDone = true;
    _orientationCalDownSideInProgress = false;
    _orientationCalUpsideDownSideInProgress = false;
    _orientationCalLeftSideInProgress = false;
    _orientationCalRightSideInProgress = false;
    _orientationCalNoseDownSideInProgress = false;
    _orientationCalTailDownSideInProgress = false;
    _orientationCalDownSideRotate = false;
    _orientationCalUpsideDownSideRotate = false;
    _orientationCalLeftSideRotate = false;
    _orientationCalRightSideRotate = false;
    _orientationCalNoseDownSideRotate = false;
    _orientationCalTailDownSideRotate = false;

    emit orientationCalSidesRotateChanged();
    emit orientationCalSidesDoneChanged();
    emit orientationCalSidesInProgressChanged();
}

void APMSensorsComponentController::_stopCalibration(APMSensorsComponentController::StopCalibrationCode code)
{
    if (_accelCalInProgress) {
        _vehicle->setConnectionLostEnabled(true);
    }

    disconnect(_uas, &UASInterface::textMessageReceived, this, &APMSensorsComponentController::_handleUASTextMessage);
    
    _compassButton->setEnabled(true);
    _accelButton->setEnabled(true);
    _nextButton->setEnabled(false);
    _cancelButton->setEnabled(false);

    if (code == StopCalibrationSuccess) {
        _resetInternalState();
        _progressBar->setProperty("value", 1);
    } else {
        _progressBar->setProperty("value", 0);
    }
    
    _waitingForCancel = false;
    emit waitingForCancelChanged();

    _refreshParams();
    
    switch (code) {
    case StopCalibrationSuccess:
        _orientationCalAreaHelpText->setProperty("text", "Calibration complete");
        emit resetStatusTextArea();
        emit calibrationComplete();
        break;

    case StopCalibrationCancelled:
        emit resetStatusTextArea();
        _hideAllCalAreas();
        break;

    default:
        // Assume failed
        _hideAllCalAreas();
        qgcApp()->showMessage(QStringLiteral("Calibration failed. Calibration log will be displayed."));
        break;
    }
    
    _magCalInProgress = false;
    _accelCalInProgress = false;
}

void APMSensorsComponentController::calibrateCompass(void)
{
    _startLogCalibration();
    _compassCal.startCalibration();
}

void APMSensorsComponentController::calibrateAccel(void)
{
    _vehicle->setConnectionLostEnabled(false);
    _startLogCalibration();
    _accelCalInProgress = true;
    _uas->startCalibration(UASInterface::StartCalibrationAccel);
}

void APMSensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    UASInterface* uas = _autopilot->vehicle()->uas();
    Q_ASSERT(uas);
    if (uasId != uas->getUASID()) {
        return;
    }

    if (text.startsWith(QLatin1Literal("PreArm:")) || text.startsWith(QLatin1Literal("EKF"))
            || text.startsWith(QLatin1Literal("Arm")) || text.startsWith(QLatin1Literal("Initialising"))) {
        return;
    }

    if (text.contains(QLatin1Literal("progress <"))) {
        QString percent = text.split("<").last().split(">").first();
        bool ok;
        int p = percent.toInt(&ok);
        if (ok) {
            Q_ASSERT(_progressBar);
            _progressBar->setProperty("value", (float)(p / 100.0));
        }
        return;
    }

    QString anyKey(QStringLiteral("and press any"));
    if (text.contains(anyKey)) {
        text = text.left(text.indexOf(anyKey)) + QStringLiteral("and click Next to continue.");
        _nextButton->setEnabled(true);
    }

    _appendStatusLog(text);
    qCDebug(APMSensorsComponentControllerLog) << text << severity;

    if (text.contains(QLatin1String("Calibration successful"))) {
        _stopCalibration(StopCalibrationSuccess);
        return;
    }

    if (text.contains(QLatin1String("FAILED"))) {
        _stopCalibration(StopCalibrationFailed);
        return;
    }

    // All calibration messages start with [cal]
    QString calPrefix(QStringLiteral("[cal] "));
    if (!text.startsWith(calPrefix)) {
        return;
    }
    text = text.right(text.length() - calPrefix.length());

    QString calStartPrefix(QStringLiteral("calibration started: "));
    if (text.startsWith(calStartPrefix)) {
        text = text.right(text.length() - calStartPrefix.length());
        
        _startVisualCalibration();
        
        if (text == QLatin1Literal("accel") || text == QLatin1Literal("mag") || text == QLatin1Literal("gyro")) {
            // Reset all progress indication
            _orientationCalDownSideDone = false;
            _orientationCalUpsideDownSideDone = false;
            _orientationCalLeftSideDone = false;
            _orientationCalRightSideDone = false;
            _orientationCalTailDownSideDone = false;
            _orientationCalNoseDownSideDone = false;
            _orientationCalDownSideInProgress = false;
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalLeftSideInProgress = false;
            _orientationCalRightSideInProgress = false;
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalTailDownSideInProgress = false;
            
            // Reset all visibility
            _orientationCalDownSideVisible = false;
            _orientationCalUpsideDownSideVisible = false;
            _orientationCalLeftSideVisible = false;
            _orientationCalRightSideVisible = false;
            _orientationCalTailDownSideVisible = false;
            _orientationCalNoseDownSideVisible = false;
            
            _orientationCalAreaHelpText->setProperty("text", "Place your vehicle into one of the Incomplete orientations shown below and hold it still");
            
            if (text == "accel") {
                _accelCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "mag") {
                _magCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else {
                Q_ASSERT(false);
            }
            emit orientationCalSidesDoneChanged();
            emit orientationCalSidesVisibleChanged();
            emit orientationCalSidesInProgressChanged();
            _updateAndEmitShowOrientationCalArea(true);
        }
        return;
    }
    
    if (text.endsWith(QLatin1Literal("orientation detected"))) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side started" << side;
        
        if (side == QLatin1Literal("down")) {
            _orientationCalDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalDownSideRotate = true;
            }
        } else if (side == QLatin1Literal("up")) {
            _orientationCalUpsideDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalUpsideDownSideRotate = true;
            }
        } else if (side == QLatin1Literal("left")) {
            _orientationCalLeftSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalLeftSideRotate = true;
            }
        } else if (side == QLatin1Literal("right")) {
            _orientationCalRightSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalRightSideRotate = true;
            }
        } else if (side == QLatin1Literal("front")) {
            _orientationCalNoseDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalNoseDownSideRotate = true;
            }
        } else if (side == QLatin1Literal("back")) {
            _orientationCalTailDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalTailDownSideRotate = true;
            }
        }
        
        if (_magCalInProgress) {
            _orientationCalAreaHelpText->setProperty("text", "Rotate the vehicle continuously as shown in the diagram until marked as Completed");
        } else {
            _orientationCalAreaHelpText->setProperty("text", "Hold still in the current orientation");
        }
        
        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    if (text.endsWith(QLatin1Literal("side done, rotate to a different side"))) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side finished" << side;
        
        if (side == QLatin1Literal("down")) {
            _orientationCalDownSideInProgress = false;
            _orientationCalDownSideDone = true;
            _orientationCalDownSideRotate = false;
        } else if (side == QLatin1Literal("up")) {
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalUpsideDownSideDone = true;
            _orientationCalUpsideDownSideRotate = false;
        } else if (side == QLatin1Literal("left")) {
            _orientationCalLeftSideInProgress = false;
            _orientationCalLeftSideDone = true;
            _orientationCalLeftSideRotate = false;
        } else if (side == QLatin1Literal("right")) {
            _orientationCalRightSideInProgress = false;
            _orientationCalRightSideDone = true;
            _orientationCalRightSideRotate = false;
        } else if (side == QLatin1Literal("front")) {
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalNoseDownSideDone = true;
            _orientationCalNoseDownSideRotate = false;
        } else if (side == QLatin1Literal("back")) {
            _orientationCalTailDownSideInProgress = false;
            _orientationCalTailDownSideDone = true;
            _orientationCalTailDownSideRotate = false;
        }
        
        _orientationCalAreaHelpText->setProperty("text", "Place you vehicle into one of the orientations shown below and hold it still");

        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesDoneChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    if (text.startsWith(QLatin1Literal("calibration done:"))) {
        _stopCalibration(StopCalibrationSuccess);
        return;
    }

    if (text.startsWith(QLatin1Literal("calibration cancelled"))) {
        _stopCalibration(_waitingForCancel ? StopCalibrationCancelled : StopCalibrationFailed);
        return;
    }

    if (text.startsWith(QLatin1Literal("calibration failed"))) {
        _stopCalibration(StopCalibrationFailed);
        return;
    }
}

void APMSensorsComponentController::_refreshParams(void)
{
    QStringList fastRefreshList;
    
    fastRefreshList << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_X")
                    << QStringLiteral("INS_ACCOFFS_X") << QStringLiteral("INS_ACCOFFS_Y") << QStringLiteral("INS_ACCOFFS_Z");
    foreach (const QString &paramName, fastRefreshList) {
        _autopilot->refreshParameter(FactSystem::defaultComponentId, paramName);
    }
    
    // Now ask for all to refresh
    _autopilot->refreshParametersPrefix(FactSystem::defaultComponentId, QStringLiteral("COMPASS_"));
    _autopilot->refreshParametersPrefix(FactSystem::defaultComponentId, QStringLiteral("INS_"));
}

void APMSensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void APMSensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowOrientationCalArea(false);
}

void APMSensorsComponentController::cancelCalibration(void)
{
    _waitingForCancel = true;
    emit waitingForCancelChanged();
    _cancelButton->setEnabled(false);

    if (_magCalInProgress) {
        _compassCal.cancelCalibration();
    } else {
        // The firmware doesn't always allow us to cancel calibration. The best we can do is wait
        // for it to timeout.
        _uas->stopCalibration();
    }
}

void APMSensorsComponentController::nextClicked(void)
{
    mavlink_message_t       msg;
    mavlink_command_ack_t   ack;

    ack.command = 0;
    ack.result = 1;
    mavlink_msg_command_ack_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &msg, &ack);

    _vehicle->sendMessage(msg);
}

bool APMSensorsComponentController::compassSetupNeeded(void) const
{
    return _sensorsComponent->compassSetupNeeded();
}

bool APMSensorsComponentController::accelSetupNeeded(void) const
{
    return _sensorsComponent->accelSetupNeeded();
}
