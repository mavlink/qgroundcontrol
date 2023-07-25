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

#include <QList>
#include <QTimer>
#include <QSettings>
#include <iostream>
#include <QDebug>

#include <cmath>
#include <qmath.h>

#include <limits>
#include <cstdlib>

#include "UAS.h"
#include "LinkInterface.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "LinkManager.h"
#ifndef NO_SERIAL_LINK
#include "SerialLink.h"
#endif
#include "FirmwarePluginManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "Joystick.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(UASLog, "UASLog")

// THIS CLASS IS DEPRECATED. ALL NEW FUNCTIONALITY SHOULD GO INTO Vehicle class
UAS::UAS(MAVLinkProtocol* protocol, Vehicle* vehicle, FirmwarePluginManager * firmwarePluginManager) : UASInterface(),
    lipoFull(4.2f),
    lipoEmpty(3.5f),
    uasId(vehicle->id()),
    unknownPackets(),
    mavlink(protocol),
    receiveDropRate(0),
    sendDropRate(0),

    status(-1),

    startTime(QGC::groundTimeMilliseconds()),
    onboardTimeOffset(0),

    controlRollManual(true),
    controlPitchManual(true),
    controlYawManual(true),
    controlThrustManual(true),

    attitudeKnown(false),
    attitudeStamped(false),
    lastAttitude(0),

    blockHomePositionChanges(false),

    // Note variances calculated from flight case from this log: http://dash.oznet.ch/view/MRjW8NUNYQSuSZkbn8dEjY
    // TODO: calibrate stand-still pixhawk variances
    xacc_var(0.6457f),
    yacc_var(0.7048f),
    zacc_var(0.97885f),
    rollspeed_var(0.8126f),
    pitchspeed_var(0.6145f),
    yawspeed_var(0.5852f),
    xmag_var(0.2393f),
    ymag_var(0.2283f),
    zmag_var(0.1665f),
    abs_pressure_var(0.5802f),
    diff_pressure_var(0.5802f),
    pressure_alt_var(0.5802f),
    temperature_var(0.7145f),
    /*
    xacc_var(0.0f),
    yacc_var(0.0f),
    zacc_var(0.0f),
    rollspeed_var(0.0f),
    pitchspeed_var(0.0f),
    yawspeed_var(0.0f),
    xmag_var(0.0f),
    ymag_var(0.0f),
    zmag_var(0.0f),
    abs_pressure_var(0.0f),
    diff_pressure_var(0.0f),
    pressure_alt_var(0.0f),
    temperature_var(0.0f),
    */

    // The protected members.
    connectionLost(false),
    lastVoltageWarning(0),
    lastNonNullTime(0),
    onboardTimeOffsetInvalidCount(0),
    _vehicle(vehicle),
    _firmwarePluginManager(firmwarePluginManager)
{

}

/**
* @ return the id of the uas
*/
int UAS::getUASID() const
{
    return uasId;
}

// Ignore warnings from mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__

#if __GNUC__ > 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#endif

#else
#pragma warning(push, 0)
#endif

void UAS::receiveMessage(mavlink_message_t message)
{
    // Only accept messages from this system (condition 1)
    // and only then if a) attitudeStamped is disabled OR b) attitudeStamped is enabled
    // and we already got one attitude packet
    if (message.sysid == uasId && (!attitudeStamped || lastAttitude != 0 || message.msgid == MAVLINK_MSG_ID_ATTITUDE))
    {
        switch (message.msgid)
        {

        case MAVLINK_MSG_ID_LOG_ENTRY:
        {
            mavlink_log_entry_t log;
            mavlink_msg_log_entry_decode(&message, &log);
            emit logEntry(this, log.time_utc, log.size, log.id, log.num_logs, log.last_log_num);
        }
            break;

        case MAVLINK_MSG_ID_LOG_DATA:
        {
            mavlink_log_data_t log;
            mavlink_msg_log_data_decode(&message, &log);
            emit logData(this, log.ofs, log.id, log.count, log.data);
        }
            break;

        default:
            break;
        }
    }
}

// Pop warnings ignoring for mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #else
        #pragma GCC diagnostic pop
    #endif
#else
#pragma warning(pop, 0)
#endif

/**
* Check if time is smaller than 40 years, assuming no system without Unix
* timestamp runs longer than 40 years continuously without reboot. In worst case
* this will add/subtract the communication delay between GCS and MAV, it will
* never alter the timestamp in a safety critical way.
*/
quint64 UAS::getUnixReferenceTime(quint64 time)
{
    // Same as getUnixTime, but does not react to attitudeStamped mode
    if (time == 0)
    {
        //        qDebug() << "XNEW time:" <<QGC::groundTimeMilliseconds();
        return QGC::groundTimeMilliseconds();
    }
    // Check if time is smaller than 40 years,
    // assuming no system without Unix timestamp
    // runs longer than 40 years continuously without
    // reboot. In worst case this will add/subtract the
    // communication delay between GCS and MAV,
    // it will never alter the timestamp in a safety
    // critical way.
    //
    // Calculation:
    // 40 years
    // 365 days
    // 24 hours
    // 60 minutes
    // 60 seconds
    // 1000 milliseconds
    // 1000 microseconds
#ifndef _MSC_VER
    else if (time < 1261440000000000LLU)
#else
    else if (time < 1261440000000000)
#endif
    {
        //        qDebug() << "GEN time:" << time/1000 + onboardTimeOffset;
        if (onboardTimeOffset == 0)
        {
            onboardTimeOffset = QGC::groundTimeMilliseconds() - time/1000;
        }
        return time/1000 + onboardTimeOffset;
    }
    else
    {
        // Time is not zero and larger than 40 years -> has to be
        // a Unix epoch timestamp. Do nothing.
        return time/1000;
    }
}

/**
* @warning If attitudeStamped is enabled, this function will not actually return
* the precise time stamp of this measurement augmented to UNIX time, but will
* MOVE the timestamp IN TIME to match the last measured attitude. There is no
* reason why one would want this, except for system setups where the onboard
* clock is not present or broken and datasets should be collected that are still
* roughly synchronized. PLEASE NOTE THAT ENABLING ATTITUDE STAMPED RUINS THE
* SCIENTIFIC NATURE OF THE CORRECT LOGGING FUNCTIONS OF QGROUNDCONTROL!
*/
quint64 UAS::getUnixTimeFromMs(quint64 time)
{
    return getUnixTime(time*1000);
}

/**
* @warning If attitudeStamped is enabled, this function will not actually return
* the precise time stam of this measurement augmented to UNIX time, but will
* MOVE the timestamp IN TIME to match the last measured attitude. There is no
* reason why one would want this, except for system setups where the onboard
* clock is not present or broken and datasets should be collected that are
* still roughly synchronized. PLEASE NOTE THAT ENABLING ATTITUDE STAMPED
* RUINS THE SCIENTIFIC NATURE OF THE CORRECT LOGGING FUNCTIONS OF QGROUNDCONTROL!
*/
quint64 UAS::getUnixTime(quint64 time)
{
    quint64 ret = 0;
    if (attitudeStamped)
    {
        ret = lastAttitude;
    }

    if (time == 0)
    {
        ret = QGC::groundTimeMilliseconds();
    }
    // Check if time is smaller than 40 years,
    // assuming no system without Unix timestamp
    // runs longer than 40 years continuously without
    // reboot. In worst case this will add/subtract the
    // communication delay between GCS and MAV,
    // it will never alter the timestamp in a safety
    // critical way.
    //
    // Calculation:
    // 40 years
    // 365 days
    // 24 hours
    // 60 minutes
    // 60 seconds
    // 1000 milliseconds
    // 1000 microseconds
#ifndef _MSC_VER
    else if (time < 1261440000000000LLU)
#else
    else if (time < 1261440000000000)
#endif
    {
        //        qDebug() << "GEN time:" << time/1000 + onboardTimeOffset;
        if (onboardTimeOffset == 0 || time < (lastNonNullTime - 100))
        {
            lastNonNullTime = time;
            onboardTimeOffset = QGC::groundTimeMilliseconds() - time/1000;
        }
        if (time > lastNonNullTime) lastNonNullTime = time;

        ret = time/1000 + onboardTimeOffset;
    }
    else
    {
        // Time is not zero and larger than 40 years -> has to be
        // a Unix epoch timestamp. Do nothing.
        ret = time/1000;
    }

    return ret;
}

/**
* Get the status of the code and a description of the status.
* Status can be unitialized, booting up, calibrating sensors, active
* standby, cirtical, emergency, shutdown or unknown.
*/
void UAS::getStatusForCode(int statusCode, QString& uasState, QString& stateDescription)
{
    switch (statusCode)
    {
    case MAV_STATE_UNINIT:
        uasState = tr("UNINIT");
        stateDescription = tr("Unitialized, booting up.");
        break;
    case MAV_STATE_BOOT:
        uasState = tr("BOOT");
        stateDescription = tr("Booting system, please wait.");
        break;
    case MAV_STATE_CALIBRATING:
        uasState = tr("CALIBRATING");
        stateDescription = tr("Calibrating sensors, please wait.");
        break;
    case MAV_STATE_ACTIVE:
        uasState = tr("ACTIVE");
        stateDescription = tr("Active, normal operation.");
        break;
    case MAV_STATE_STANDBY:
        uasState = tr("STANDBY");
        stateDescription = tr("Standby mode, ready for launch.");
        break;
    case MAV_STATE_CRITICAL:
        uasState = tr("CRITICAL");
        stateDescription = tr("FAILURE: Continuing operation.");
        break;
    case MAV_STATE_EMERGENCY:
        uasState = tr("EMERGENCY");
        stateDescription = tr("EMERGENCY: Land Immediately!");
        break;
        //case MAV_STATE_HILSIM:
        //uasState = tr("HIL SIM");
        //stateDescription = tr("HIL Simulation, Sensors read from SIM");
        //break;

    case MAV_STATE_POWEROFF:
        uasState = tr("SHUTDOWN");
        stateDescription = tr("Powering off system.");
        break;

    default:
        uasState = tr("UNKNOWN");
        stateDescription = tr("Unknown system state");
        break;
    }
}

/* MANAGEMENT */

/**
 *
 * @return The uptime in milliseconds
 *
 */
quint64 UAS::getUptime() const
{
    if(startTime == 0)
    {
        return 0;
    }
    else
    {
        return QGC::groundTimeMilliseconds() - startTime;
    }
}

/**
* Order the robot to start receiver pairing
*/
void UAS::pairRX(int rxType, int rxSubType)
{
    if (_vehicle) {
        _vehicle->sendMavCommand(_vehicle->defaultComponentId(),    // target component
                                 MAV_CMD_START_RX_PAIR,             // command id
                                 true,                              // showError
                                 rxType,
                                 rxSubType);
    }
}

void UAS::shutdownVehicle(void)
{
    _vehicle = nullptr;
}
