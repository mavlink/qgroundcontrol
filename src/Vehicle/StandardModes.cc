/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Vehicle.h"
#include "StandardModes.h"

QGC_LOGGING_CATEGORY(StandardModesLog, "StandardModesLog")


static void requestMessageResultHandler(void* resultHandlerData, MAV_RESULT result,
                                        Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t &message)
{
    StandardModes* standardModes = static_cast<StandardModes*>(resultHandlerData);
    standardModes->gotMessage(result, message);
}

StandardModes::StandardModes(QObject *parent, Vehicle *vehicle)
        : QObject(parent), _vehicle(vehicle)
{
}

void StandardModes::gotMessage(MAV_RESULT result, const mavlink_message_t &message)
{
    _requestActive = false;
    if (_wantReset) {
        _wantReset = false;
        request();
        return;
    }

    if (result == MAV_RESULT_ACCEPTED) {
        mavlink_available_modes_t availableModes;
        mavlink_msg_available_modes_decode(&message, &availableModes);
        bool cannotBeSet = availableModes.properties & MAV_MODE_PROPERTY_NOT_USER_SELECTABLE;
        bool advanced = availableModes.properties & MAV_MODE_PROPERTY_ADVANCED;
        availableModes.mode_name[sizeof(availableModes.mode_name)-1] = '\0';
        QString name = availableModes.mode_name;
        switch (availableModes.standard_mode) {
            case MAV_STANDARD_MODE_POSITION_HOLD:
                name = "Position";
                break;
            case MAV_STANDARD_MODE_ORBIT:
                name = "Orbit";
                break;
            case MAV_STANDARD_MODE_CRUISE:
                name = "Cruise";
                break;
            case MAV_STANDARD_MODE_ALTITUDE_HOLD:
                name = "Altitude";
                break;
            case MAV_STANDARD_MODE_RETURN_HOME:
                name = "Return";
                break;
            case MAV_STANDARD_MODE_SAFE_RECOVERY:
                name = "Safe Recovery";
                break;
            case MAV_STANDARD_MODE_MISSION:
                name = "Mission";
                break;
            case MAV_STANDARD_MODE_LAND:
                name = "Land";
                break;
            case MAV_STANDARD_MODE_TAKEOFF:
                name = "Takeoff";
                break;
        }

        if (name == "Takeoff" || name == "VTOL Takeoff" || name == "Orbit" || name == "Land" || name == "Return") { // These are exposed in the UI as separate buttons
            cannotBeSet = true;
        }

        qCDebug(StandardModesLog) << "Got mode:" << name << ", idx:" << availableModes.mode_index << ", custom_mode" << availableModes.custom_mode;

        _nextModes[availableModes.custom_mode] = Mode{name, availableModes.standard_mode, advanced, cannotBeSet};

        if (availableModes.mode_index >= availableModes.number_modes) { // We are done
            qCDebug(StandardModesLog) << "Completed, num modes:" << _nextModes.size();
            _modes = _nextModes;
            ensureUniqueModeNames();
            _hasModes = true;
            emit modesUpdated();
            emit requestCompleted();

        } else {
            requestMode(availableModes.mode_index + 1);
        }

    } else {
        qCDebug(StandardModesLog) << "Failed to retrieve available modes" << result;
        emit requestCompleted();
    }
}

void StandardModes::ensureUniqueModeNames()
{
    // Ensure mode names are unique. This should generally already be the case, but e.g. during development when
    // restarting dynamic modes, it might not be.
    for (auto iter = _modes.begin(); iter != _modes.end(); ++iter) {
        int duplicateIdx = 0;
        for (auto iter2 = iter + 1; iter2 != _modes.end(); ++iter2) {
            if (iter.value().name == iter2.value().name) {
                iter2.value().name += QStringLiteral(" (%1)").arg(duplicateIdx + 1);
                ++duplicateIdx;
            }
        }
    }
}

void StandardModes::request()
{
#ifdef DAILY_BUILD // Disable use of development/WIP MAVLink messages for release builds
    if (_requestActive) {
        // If we are in the middle of waiting for a request, wait for the response first
        _wantReset = true;
        return;
    }

    _nextModes.clear();

    qCDebug(StandardModesLog) << "Requesting available modes";
    // Request one at a time. This could be improved by requesting all, but we can't use Vehicle::requestMessage for that
    StandardModes::requestMode(1);
#else
    emit requestCompleted();
#endif // DAILY_BUILD
}

void StandardModes::requestMode(int modeIndex)
{
    _requestActive = true;
    _vehicle->requestMessage(
            requestMessageResultHandler,
            this,
            MAV_COMP_ID_AUTOPILOT1,
            MAVLINK_MSG_ID_AVAILABLE_MODES, modeIndex);
}

void StandardModes::availableModesMonitorReceived(uint8_t seq)
{
    if (_lastSeq != seq) {
        qCDebug(StandardModesLog) << "Available modes changed, re-requesting";
        _lastSeq = seq;
        request();
    }
}

QStringList StandardModes::flightModes()
{
    QStringList ret;
    for (const auto& mode : _modes) {
        if (mode.cannotBeSet) {
            continue;
        }
        ret += mode.name;
    }
    return ret;
}

QString StandardModes::flightMode(uint32_t custom_mode) const
{
    auto iter = _modes.find(custom_mode);
    if (iter != _modes.end()) {
        return iter->name;
    }
    return tr("Unknown %2").arg(custom_mode);
}

bool StandardModes::setFlightMode(const QString &flightMode, uint32_t *custom_mode)
{
    for (auto iter = _modes.constBegin(); iter != _modes.constEnd(); ++iter) {
        if (iter->name == flightMode) {
            *custom_mode = iter.key();
            return true;
        }
    }
    return false;
}
