/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SensorsComponentController.h"
#include "QGCMAVLink.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "ParameterManager.h"

#include <QVariant>
#include <QQmlProperty>

QGC_LOGGING_CATEGORY(SensorsComponentControllerLog, "SensorsComponentControllerLog")

SensorsComponentController::SensorsComponentController(void)
    : _statusLog                                (nullptr)
    , _progressBar                              (nullptr)
    , _compassButton                            (nullptr)
    , _gyroButton                               (nullptr)
    , _accelButton                              (nullptr)
    , _airspeedButton                           (nullptr)
    , _levelButton                              (nullptr)
    , _cancelButton                             (nullptr)
    , _setOrientationsButton                    (nullptr)
    , _showOrientationCalArea                   (false)
    , _gyroCalInProgress                        (false)
    , _magCalInProgress                         (false)
    , _accelCalInProgress                       (false)
    , _airspeedCalInProgress                    (false)
    , _levelCalInProgress                       (false)
    , _orientationCalDownSideDone               (false)
    , _orientationCalUpsideDownSideDone         (false)
    , _orientationCalLeftSideDone               (false)
    , _orientationCalRightSideDone              (false)
    , _orientationCalNoseDownSideDone           (false)
    , _orientationCalTailDownSideDone           (false)
    , _orientationCalDownSideVisible            (false)
    , _orientationCalUpsideDownSideVisible      (false)
    , _orientationCalLeftSideVisible            (false)
    , _orientationCalRightSideVisible           (false)
    , _orientationCalNoseDownSideVisible        (false)
    , _orientationCalTailDownSideVisible        (false)
    , _orientationCalDownSideInProgress         (false)
    , _orientationCalUpsideDownSideInProgress   (false)
    , _orientationCalLeftSideInProgress         (false)
    , _orientationCalRightSideInProgress        (false)
    , _orientationCalNoseDownSideInProgress     (false)
    , _orientationCalTailDownSideInProgress     (false)
    , _orientationCalDownSideRotate             (false)
    , _orientationCalUpsideDownSideRotate       (false)
    , _orientationCalLeftSideRotate             (false)
    , _orientationCalRightSideRotate            (false)
    , _orientationCalNoseDownSideRotate         (false)
    , _orientationCalTailDownSideRotate         (false)
    , _unknownFirmwareVersion                   (false)
    , _waitingForCancel                         (false)
{
}

bool SensorsComponentController::usingUDPLink(void)
{
    return _vehicle->priorityLink()->getLinkConfiguration()->type() == LinkConfiguration::TypeUdp;
}

/// Appends the specified text to the status log area in the ui
void SensorsComponentController::_appendStatusLog(const QString& text)
{
    if (!_statusLog) {
        qWarning() << "Internal error";
        return;
    }
    
    QVariant returnedValue;
    QVariant varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void SensorsComponentController::_startLogCalibration(void)
{
    _unknownFirmwareVersion = false;
    _hideAllCalAreas();
    
    connect(_vehicle, &Vehicle::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
    
    _cancelButton->setEnabled(false);
}

void SensorsComponentController::_startVisualCalibration(void)
{
    _compassButton->setEnabled(false);
    _gyroButton->setEnabled(false);
    _accelButton->setEnabled(false);
    _airspeedButton->setEnabled(false);
    _levelButton->setEnabled(false);
    _setOrientationsButton->setEnabled(false);
    _cancelButton->setEnabled(true);

    _resetInternalState();
    
    _progressBar->setProperty("value", 0);
}

void SensorsComponentController::_resetInternalState(void)
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

void SensorsComponentController::_stopCalibration(SensorsComponentController::StopCalibrationCode code)
{
    disconnect(_vehicle, &Vehicle::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
    
    _compassButton->setEnabled(true);
    _gyroButton->setEnabled(true);
    _accelButton->setEnabled(true);
    _airspeedButton->setEnabled(true);
    _levelButton->setEnabled(true);
    _setOrientationsButton->setEnabled(true);
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
            _orientationCalAreaHelpText->setProperty("text", tr("Calibration complete"));
            if (!_airspeedCalInProgress && !_levelCalInProgress) {
                emit resetStatusTextArea();
            }
            if (_magCalInProgress) {
                emit magCalComplete();
            }
            break;
            
        case StopCalibrationCancelled:
            emit resetStatusTextArea();
            _hideAllCalAreas();
            break;
            
        default:
            // Assume failed
            _hideAllCalAreas();
            qgcApp()->showMessage(tr("Calibration failed. Calibration log will be displayed."));
            break;
    }
    
    _magCalInProgress = false;
    _accelCalInProgress = false;
    _gyroCalInProgress = false;
    _airspeedCalInProgress = false;
}

void SensorsComponentController::calibrateGyro(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationGyro);
}

void SensorsComponentController::calibrateCompass(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationMag);
}

void SensorsComponentController::calibrateAccel(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationAccel);
}

void SensorsComponentController::calibrateLevel(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationLevel);
}

void SensorsComponentController::calibrateAirspeed(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationAirspeed);
}

void SensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    if (uasId != _vehicle->id()) {
        return;
    }
    
    if (text.contains("progress <")) {
        QString percent = text.split("<").last().split(">").first();
        bool ok;
        int p = percent.toInt(&ok);
        if (ok) {
            if (_progressBar) {
                _progressBar->setProperty("value", (float)(p / 100.0));
            } else {
                qWarning() << "Internal error";
            }
        }
        return;
    }

    _appendStatusLog(text);
    qCDebug(SensorsComponentControllerLog) << text;
    
    if (_unknownFirmwareVersion) {
        // We don't know how to do visual cal with the version of firwmare
        return;
    }
    
    // All calibration messages start with [cal]
    QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        return;
    }
    text = text.right(text.length() - calPrefix.length());

    QString calStartPrefix("calibration started: ");
    if (text.startsWith(calStartPrefix)) {
        text = text.right(text.length() - calStartPrefix.length());
        
        // Split version number and cal type
        QStringList parts = text.split(" ");
        if (parts.count() != 2 && parts[0].toInt() != _supportedFirmwareCalVersion) {
            _unknownFirmwareVersion = true;
            QString msg = tr("Unsupported calibration firmware version, using log");
            _appendStatusLog(msg);
            qDebug() << msg;
            return;
        }
        
        _startVisualCalibration();
        
        text = parts[1];
        if (text == "accel" || text == "mag" || text == "gyro") {
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
            
            _orientationCalAreaHelpText->setProperty("text", tr("Place your vehicle into one of the Incomplete orientations shown below and hold it still"));
            
            if (text == "accel") {
                _accelCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "mag") {

                // Work out what the autopilot is configured to
                int sides = 0;

                if (_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "CAL_MAG_SIDES")) {
                    // Read the requested calibration directions off the system
                    sides = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "CAL_MAG_SIDES")->rawValue().toFloat();
                } else {
                    // There is no valid setting, default to all six sides
                    sides = (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
                }

                _magCalInProgress = true;
                _orientationCalTailDownSideVisible =   ((sides & (1 << 0)) > 0);
                _orientationCalNoseDownSideVisible =   ((sides & (1 << 1)) > 0);
                _orientationCalLeftSideVisible =       ((sides & (1 << 2)) > 0);
                _orientationCalRightSideVisible =      ((sides & (1 << 3)) > 0);
                _orientationCalUpsideDownSideVisible = ((sides & (1 << 4)) > 0);
                _orientationCalDownSideVisible =       ((sides & (1 << 5)) > 0);
            } else if (text == "gyro") {
                _gyroCalInProgress = true;
                _orientationCalDownSideVisible = true;
            } else {
                qWarning() << "Unknown calibration message type" << text;
            }
            emit orientationCalSidesDoneChanged();
            emit orientationCalSidesVisibleChanged();
            emit orientationCalSidesInProgressChanged();
            _updateAndEmitShowOrientationCalArea(true);
        } else if (text == "airspeed") {
            _airspeedCalInProgress = true;
        } else if (text == "level") {
            _levelCalInProgress = true;
        }
        return;
    }
    
    if (text.endsWith("orientation detected")) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side started" << side;
        
        if (side == "down") {
            _orientationCalDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalDownSideRotate = true;
            }
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalUpsideDownSideRotate = true;
            }
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalLeftSideRotate = true;
            }
        } else if (side == "right") {
            _orientationCalRightSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalRightSideRotate = true;
            }
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalNoseDownSideRotate = true;
            }
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalTailDownSideRotate = true;
            }
        }
        
        if (_magCalInProgress) {
            _orientationCalAreaHelpText->setProperty("text", tr("Rotate the vehicle continuously as shown in the diagram until marked as Completed"));
        } else {
            _orientationCalAreaHelpText->setProperty("text", tr("Hold still in the current orientation"));
        }
        
        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    if (text.endsWith("side done, rotate to a different side")) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side finished" << side;
        
        if (side == "down") {
            _orientationCalDownSideInProgress = false;
            _orientationCalDownSideDone = true;
            _orientationCalDownSideRotate = false;
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalUpsideDownSideDone = true;
            _orientationCalUpsideDownSideRotate = false;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = false;
            _orientationCalLeftSideDone = true;
            _orientationCalLeftSideRotate = false;
        } else if (side == "right") {
            _orientationCalRightSideInProgress = false;
            _orientationCalRightSideDone = true;
            _orientationCalRightSideRotate = false;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalNoseDownSideDone = true;
            _orientationCalNoseDownSideRotate = false;
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = false;
            _orientationCalTailDownSideDone = true;
            _orientationCalTailDownSideRotate = false;
        }
        
        _orientationCalAreaHelpText->setProperty("text", tr("Place you vehicle into one of the orientations shown below and hold it still"));

        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesDoneChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }

    if (text.endsWith("side already completed")) {
        _orientationCalAreaHelpText->setProperty("text", tr("Orientation already completed, place you vehicle into one of the incomplete orientations shown below and hold it still"));
        return;
    }
    
    QString calCompletePrefix("calibration done:");
    if (text.startsWith(calCompletePrefix)) {
        _stopCalibration(StopCalibrationSuccess);
        return;
    }
    
    if (text.startsWith("calibration cancelled")) {
        _stopCalibration(_waitingForCancel ? StopCalibrationCancelled : StopCalibrationFailed);
        return;
    }
    
    if (text.startsWith("calibration failed")) {
        _stopCalibration(StopCalibrationFailed);
        return;
    }
}

void SensorsComponentController::_refreshParams(void)
{
    QStringList fastRefreshList;
    
    // We ask for a refresh on these first so that the rotation combo show up as fast as possible
    fastRefreshList << "CAL_MAG0_ID" << "CAL_MAG1_ID" << "CAL_MAG2_ID" << "CAL_MAG0_ROT" << "CAL_MAG1_ROT" << "CAL_MAG2_ROT";
    foreach (const QString &paramName, fastRefreshList) {
        _vehicle->parameterManager()->refreshParameter(FactSystem::defaultComponentId, paramName);
    }
    
    // Now ask for all to refresh
    _vehicle->parameterManager()->refreshParametersPrefix(FactSystem::defaultComponentId, "CAL_");
    _vehicle->parameterManager()->refreshParametersPrefix(FactSystem::defaultComponentId, "SENS_");
}

void SensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void SensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowOrientationCalArea(false);
}

void SensorsComponentController::cancelCalibration(void)
{
    // The firmware doesn't allow us to cancel calibration. The best we can do is wait
    // for it to timeout.
    _waitingForCancel = true;
    emit waitingForCancelChanged();
    _cancelButton->setEnabled(false);
    _uas->stopCalibration();
}
