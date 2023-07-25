/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkStreamConfig.h"
#include "QGCMAVLink.h"

MAVLinkStreamConfig::MAVLinkStreamConfig(const SetMessageIntervalCb &messageIntervalCb)
    : _messageIntervalCb(messageIntervalCb)
{
}

void MAVLinkStreamConfig::setHighRateRateAndAttitude()
{
    int requestedRate = (int)(1000000.0 / 100.0); // 100 Hz in usecs (better set this a bit higher than actually needed,
    // to give it more priority in case of exceeding link bandwidth)

    _nextDesiredRates = QVector<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_ATTITUDE_QUATERNION, requestedRate},
        {MAVLINK_MSG_ID_ATTITUDE_TARGET, requestedRate},
    }};
    // we could check if we're already configured as requested

    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setHighRateVelAndPos()
{
    int requestedRate = (int)(1000000.0 / 100.0);
    _nextDesiredRates = QVector<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_LOCAL_POSITION_NED, requestedRate},
        {MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED, requestedRate},
    }};
    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setHighRateAltAirspeed()
{
    int requestedRate = (int)(1000000.0 / 100.0);
    _nextDesiredRates = QVector<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT, requestedRate},
        {MAVLINK_MSG_ID_VFR_HUD, requestedRate},
    }};
    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::gotSetMessageIntervalAck()
{
    switch (_state) {
        case State::Configuring:
            nextDesiredRate();
            break;
        case State::RestoringDefaults:
            restoreNextDefault();
            break;
        default:
            break;
    }
}

void MAVLinkStreamConfig::setNextState(State state)
{
    _nextState = state;
    if (_state == State::Idle) {
        // first restore defaults no matter what the next state is
        _state = State::RestoringDefaults;
        restoreNextDefault();
    }
}

void MAVLinkStreamConfig::nextDesiredRate()
{
    if (_nextState != State::Idle) { // allow state to be interrupted by a new request
        _desiredRates.clear();
        _state = State::RestoringDefaults;
        restoreNextDefault();
        return;
    }

    if (_desiredRates.empty()) {
        _state = State::Idle;
        return;
    }
    const DesiredStreamRate& rate = _desiredRates.last();
    _changedIds.push_back(rate.messageId);
    _messageIntervalCb(rate.messageId, rate.rate);
    _desiredRates.pop_back();
}

void MAVLinkStreamConfig::restoreDefaults()
{
    setNextState(State::RestoringDefaults);
}

void MAVLinkStreamConfig::restoreNextDefault()
{
    if (_changedIds.empty()) {
        // do we have a pending request?
        switch (_nextState) {
            case State::Configuring:
                _state = _nextState;
                _desiredRates = _nextDesiredRates;
                _nextDesiredRates.clear();
                _nextState = State::Idle;
                nextDesiredRate();
                break;
            case State::RestoringDefaults:
            case State::Idle:
                _nextState = State::Idle; // nothing to do, we just finished restoring
                _state = State::Idle;
                break;
        }
        return;
    }

    int id = _changedIds.last();
    _messageIntervalCb(id, 0); // restore the default rate
    _changedIds.pop_back();
}
