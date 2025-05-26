/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Autotune.h"
#include "QGCApplication.h"

//-----------------------------------------------------------------------------
Autotune::Autotune(Vehicle *vehicle) :
    QObject(vehicle)
    , _vehicle(vehicle)
{
    _pollTimer.setInterval(1000); // 1s for the polling interval
    _pollTimer.setSingleShot(false);
    connect(&_pollTimer, &QTimer::timeout, this, &Autotune::sendMavlinkRequest);
}


//-----------------------------------------------------------------------------
void Autotune::autotuneRequest()
{
    sendMavlinkRequest();

    startTimers();
    _autotuneInProgress  = true;
    _autotuneStatus = tr("Autotune: In progress");

    emit autotuneChanged();
}


//-----------------------------------------------------------------------------
void Autotune::ackHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    Q_UNUSED(compId);
    Q_UNUSED(failureCode);

    auto * autotune = static_cast<Autotune *>(resultHandlerData);

    if (autotune->_autotuneInProgress) {
        if (failureCode == Vehicle::MavCmdResultCommandResultOnly) {
            if ((ack.result == MAV_RESULT_IN_PROGRESS) || (ack.result == MAV_RESULT_ACCEPTED)) {
                autotune->handleAckStatus(ack.progress);
            }
            else if (ack.result == MAV_RESULT_FAILED) {
                autotune->handleAckFailure();
            }
            else {
                autotune->handleAckError(ack.result);
            }
        } else {
            autotune->handleAckFailure();
        }
        emit autotune->autotuneChanged();
    } else {
        qWarning() << "Ack received for a command different from MAV_CMD_DO_AUTOTUNE_ENABLE ot wrong UI state.";
    }
}

void Autotune::progressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack)
{
    Q_UNUSED(compId);

    auto * autotune = static_cast<Autotune *>(progressHandlerData);

    if (autotune->_autotuneInProgress) {
        autotune->handleAckStatus(ack.progress);
        emit autotune->autotuneChanged();
    } else {
        qWarning() << "Ack received for a command different from MAV_CMD_DO_AUTOTUNE_ENABLE ot wrong UI state.";
    }
}

//-----------------------------------------------------------------------------
void Autotune::handleAckStatus(uint8_t ackProgress)
{
    _autotuneProgress = ackProgress/100.f;

    if (ackProgress < 20) {
        _autotuneStatus = tr("Autotune: initializing");
    }
    else if (ackProgress < 40) {
        _autotuneStatus = tr("Autotune: roll");
    }
    else if (ackProgress < 60) {
        _autotuneStatus = tr("Autotune: pitch");
    }
    else if (ackProgress < 80) {
        _autotuneStatus = tr("Autotune: yaw");
    }
    else if (ackProgress == 95) {
        _autotuneStatus = tr("Wait for disarm");

        if(!_disarmMessageDisplayed) {
            qgcApp()->showAppMessage(tr("Land and disarm the vehicle in order to apply the parameters."));
            _disarmMessageDisplayed = true;
        }
    }
    else if (ackProgress < 100) {
        _autotuneStatus = tr("Autotune: in progress");
    }
    else { // success or unknown error
        stopTimers();
        _autotuneInProgress = false;

        if (ackProgress == 100) {
            _autotuneStatus = tr("Autotune: Success");

            qgcApp()->showAppMessage(tr("Autotune successful."));
        }
        else {
            _autotuneStatus = tr("Autotune: Unknown error");
        }
    }
}


//-----------------------------------------------------------------------------
void Autotune::handleAckFailure()
{
    stopTimers();
    _autotuneInProgress = false;
    _disarmMessageDisplayed = false;
    _autotuneStatus = tr("Autotune: Failed");
}


//-----------------------------------------------------------------------------
void Autotune::handleAckError(uint8_t ackError)
{
    stopTimers();
    _autotuneInProgress = false;
    _disarmMessageDisplayed = false;
    _autotuneStatus = tr("Autotune: Ack error %1").arg(ackError);
}


//-----------------------------------------------------------------------------
void Autotune::startTimers()
{
    _pollTimer.start();
}


//-----------------------------------------------------------------------------
void Autotune::stopTimers()
{
    _pollTimer.stop();
}


//-----------------------------------------------------------------------------
void Autotune::sendMavlinkRequest()
{
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = ackHandler;
    handlerInfo.resultHandlerData   = this;
    handlerInfo.progressHandler     = progressHandler;
    handlerInfo.progressHandlerData = this;

    _vehicle->sendMavCommandWithHandler(
            &handlerInfo,
            MAV_COMP_ID_AUTOPILOT1,           // the ID of the autopilot
            MAV_CMD_DO_AUTOTUNE_ENABLE,       // the mavlink command
            1,                                // request autotune
            0,                                // unused parameter
            0,                                // unused parameter
            0,                                // unused parameter
            0,                                // unused parameter
            0,                                // unused parameter
            0);
}
