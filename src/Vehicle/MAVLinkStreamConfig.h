/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QVector>

#include <functional>

/**
 * @class MAVLinkStreamConfig
 * Allows to configure a set of mavlink streams to a specific rate,
 * and restore back to default.
 * Note that only one set is active at a time.
 */
class MAVLinkStreamConfig
{
public:
    using SetMessageIntervalCb = std::function<void(int messageId, int rate)>;

    MAVLinkStreamConfig(const SetMessageIntervalCb& messageIntervalCb);

    void setHighRateRateAndAttitude();
    void setHighRateVelAndPos();
    void setHighRateAltAirspeed();

    void restoreDefaults();

    void gotSetMessageIntervalAck();
private:
    enum class State {
        Idle,
        RestoringDefaults,
        Configuring
    };

    struct DesiredStreamRate {
        int messageId;
        int rate;
    };

    void restoreNextDefault();
    void nextDesiredRate();
    void setNextState(State state);

    State _state{State::Idle};
    QVector<DesiredStreamRate> _desiredRates;
    QVector<int> _changedIds;

    State _nextState{State::Idle};
    QVector<DesiredStreamRate> _nextDesiredRates;

    const SetMessageIntervalCb _messageIntervalCb;
};
