/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// NO NEW CODE HERE
// UASInterface, UAS.h/cc are deprecated. All new functionality should go into Vehicle.h/cc
//

#pragma once

#include "UASInterface.h"
#include <MAVLinkProtocol.h>
#include <QVector3D>
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "FirmwarePluginManager.h"

Q_DECLARE_LOGGING_CATEGORY(UASLog)

class Vehicle;

/**
 * @brief A generic MAVLINK-connected MAV/UAV
 *
 * This class represents one vehicle. It can be used like the real vehicle, e.g. a call to halt()
 * will automatically send the appropriate messages to the vehicle. The vehicle state will also be
 * automatically updated by the comm architecture, so when writing code to e.g. control the vehicle
 * no knowledge of the communication infrastructure is needed.
 */
class UAS : public UASInterface
{
    Q_OBJECT
public:
    UAS(MAVLinkProtocol* protocol, Vehicle* vehicle, FirmwarePluginManager * firmwarePluginManager);

    float lipoFull;  ///< 100% charged voltage
    float lipoEmpty; ///< Discharged voltage

    /* MANAGEMENT */

    /** @brief Get the unique system id */
    int getUASID() const;

    /** @brief The time interval the robot is switched on */
    quint64 getUptime() const;

    /// Vehicle is about to go away
    void shutdownVehicle(void);

    // Setters for HIL noise variance
    void setXaccVar(float var){
        xacc_var = var;
    }

    void setYaccVar(float var){
        yacc_var = var;
    }

    void setZaccVar(float var){
        zacc_var = var;
    }

    void setRollSpeedVar(float var){
        rollspeed_var = var;
    }

    void setPitchSpeedVar(float var){
        pitchspeed_var = var;
    }

    void setYawSpeedVar(float var){
        pitchspeed_var = var;
    }

    void setXmagVar(float var){
        xmag_var = var;
    }

    void setYmagVar(float var){
        ymag_var = var;
    }

    void setZmagVar(float var){
        zmag_var = var;
    }

    void setAbsPressureVar(float var){
        abs_pressure_var = var;
    }

    void setDiffPressureVar(float var){
        diff_pressure_var = var;
    }

    void setPressureAltVar(float var){
        pressure_alt_var = var;
    }

    void setTemperatureVar(float var){
        temperature_var = var;
    }

protected:
    /// LINK ID AND STATUS
    int uasId;                    ///< Unique system ID

    QList<int> unknownPackets;    ///< Packet IDs which are unknown and have been received
    MAVLinkProtocol* mavlink;     ///< Reference to the MAVLink instance
    float receiveDropRate;        ///< Percentage of packets that were dropped on the MAV's receiving link (from GCS and other MAVs)
    float sendDropRate;           ///< Percentage of packets that were not received from the MAV by the GCS

    /// BASIC UAS TYPE, NAME AND STATE
    int status;                   ///< The current status of the MAV

    /// TIMEKEEPING
    quint64 startTime;            ///< The time the UAS was switched on
    quint64 onboardTimeOffset;

    /// MANUAL CONTROL
    bool controlRollManual;     ///< status flag, true if roll is controlled manually
    bool controlPitchManual;    ///< status flag, true if pitch is controlled manually
    bool controlYawManual;      ///< status flag, true if yaw is controlled manually
    bool controlThrustManual;   ///< status flag, true if thrust is controlled manually

    /// POSITION
    bool isGlobalPositionKnown; ///< If the global position has been received for this MAV

    /// ATTITUDE
    bool attitudeKnown;             ///< True if attitude was received, false else
    bool attitudeStamped;           ///< Should arriving data be timestamped with the last attitude? This helps with broken system time clocks on the MAV
    quint64 lastAttitude;           ///< Timestamp of last attitude measurement

    bool blockHomePositionChanges;   ///< Block changes to the home position

    /// SIMULATION NOISE
    float xacc_var;             ///< variance of x acclerometer noise for HIL sim (mg)
    float yacc_var;             ///< variance of y acclerometer noise for HIL sim (mg)
    float zacc_var;             ///< variance of z acclerometer noise for HIL sim (mg)
    float rollspeed_var;        ///< variance of x gyroscope noise for HIL sim (rad/s)
    float pitchspeed_var;       ///< variance of y gyroscope noise for HIL sim (rad/s)
    float yawspeed_var;         ///< variance of z gyroscope noise for HIL sim (rad/s)
    float xmag_var;             ///< variance of x magnatometer noise for HIL sim (???)
    float ymag_var;             ///< variance of y magnatometer noise for HIL sim (???)
    float zmag_var;             ///< variance of z magnatometer noise for HIL sim (???)
    float abs_pressure_var;     ///< variance of absolute pressure noise for HIL sim (hPa)
    float diff_pressure_var;    ///< variance of differential pressure noise for HIL sim (hPa)
    float pressure_alt_var;     ///< variance of altitude pressure noise for HIL sim (hPa)
    float temperature_var;      ///< variance of temperature noise for HIL sim (C)

public:
    /** @brief Get the human-readable status message for this code */
    void getStatusForCode(int statusCode, QString& uasState, QString& stateDescription);

public slots:
    /** @brief Order the robot to pair its receiver **/
    void pairRX(int rxType, int rxSubType);

    /** @brief Receive a message from one of the communication links. */
    virtual void receiveMessage(mavlink_message_t message);

signals:
    void rollChanged(double val,QString name);
    void pitchChanged(double val,QString name);
    void yawChanged(double val,QString name);

protected:
    /** @brief Get the UNIX timestamp in milliseconds, enter microseconds */
    quint64 getUnixTime(quint64 time=0);
    /** @brief Get the UNIX timestamp in milliseconds, enter milliseconds */
    quint64 getUnixTimeFromMs(quint64 time);
    /** @brief Get the UNIX timestamp in milliseconds, ignore attitudeStamped mode */
    quint64 getUnixReferenceTime(quint64 time);

    bool connectionLost; ///< Flag indicates a timed out connection
    quint64 connectionLossTime; ///< Time the connection was interrupted
    quint64 lastVoltageWarning; ///< Time at which the last voltage warning occurred
    quint64 lastNonNullTime;    ///< The last timestamp from the MAV that was not null
    unsigned int onboardTimeOffsetInvalidCount;     ///< Count when the offboard time offset estimation seemed wrong

private:
    Vehicle*                _vehicle;
    FirmwarePluginManager*  _firmwarePluginManager;
};


