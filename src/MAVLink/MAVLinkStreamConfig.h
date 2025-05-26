/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkStreamConfigLog)

/// Allows to configure a set of mavlink streams to a specific rate,
/// and restore back to default.
/// Note that only one set is active at a time.
class MAVLinkStreamConfig
{
public:
    using SetMessageIntervalCb = std::function<void(int messageId, int rate)>;

    MAVLinkStreamConfig(const SetMessageIntervalCb &messageIntervalCb);
    ~MAVLinkStreamConfig();

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
        const int messageId = 0;
        const int rate = 0;
    };

    void restoreNextDefault();
    void nextDesiredRate();
    void setNextState(State state);

    State _state{State::Idle};
    QList<DesiredStreamRate> _desiredRates;
    QList<int> _changedIds;

    State _nextState{State::Idle};
    QList<DesiredStreamRate> _nextDesiredRates;

    const SetMessageIntervalCb _messageIntervalCb;
};
