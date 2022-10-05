/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QTimer>
#include <QObject>

/**
 * @brief The MavlinkMessagesTimer class
 *
 * Track the mavlink messages for a single vehicle on one link.
 * As long as regular messages are received the status is active. On the timer timeout
 * status is set to inactive. On any status change the activeChanged signal is emitted.
 * If high_latency is true then active is always true.
 */
class MavlinkMessagesTimer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief MavlinkMessagesTimer
     *
     * Constructor
     *
     * @param vehicle_id: The vehicle ID for which the heartbeat will be tracked.
     * @param high_latency: Indicates if the link is a high latency one.
     */
    MavlinkMessagesTimer(int vehicle_id, bool high_latency);

    /**
     * @brief init
     *
     * Starts the timer and emits the signal that the link is active for this vehicle ID
     */
    void init();

    ~MavlinkMessagesTimer();

    /**
     * @brief getActive
     * @return The current value of active
     */
    bool getActive() const { return _active; }

    /**
     * @brief getVehicleID
     * @return The vehicle ID
     */
    int getVehicleID() const { return _vehicleID; }

    /**
     * @brief restartTimer
     *
     * Restarts the timer and emits the signal if the timer was previously inactive
     */
    void restartTimer();

signals:
    /**
     * @brief heartbeatTimeout
     *
     * Emitted if no mavlink message is received over the specified time.
     *
     * @param vehicle_id: The vehicle ID for which the heartbeat timed out.
     */
    void heartbeatTimeout(int vehicle_id);

    /**
     * @brief activeChanged
     *
     * Emitted if the active status changes.
     *
     * @param active: The new value of the active state.
     * @param vehicle_id: The vehicle ID for which the active state changed.
     */
    void activeChanged(bool active, int vehicle_id);
private slots:
    /**
     * @brief timerTimeout
     *
     * Handle the timer timeout.
     *
     * Emit the signals according to the current state for regular links.
     * Do nothing for a high latency link.
     */
    void timerTimeout();

private:
    bool _active = false; // The state of active. Is true if the timer has not timed out.
    QTimer* _timer = nullptr; // Single shot timer
    int _vehicleID = -1; // Vehicle ID for which the heartbeat is tracked.
    bool _high_latency = false; // Indicates if the link is a high latency link or not.

    static const int    _messageReceivedTimeoutMSecs = 3500;  // Signal connection lost after 3.5 seconds of no messages
};

