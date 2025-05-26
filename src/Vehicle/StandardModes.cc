/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StandardModes.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(StandardModesLog, "StandardModesLog")

static void requestMessageResultHandler(void *resultHandlerData, MAV_RESULT result,
                                        [[maybe_unused]] Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                        const mavlink_message_t &message)
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

        qCDebug(StandardModesLog) << "Available mode received - name:" << name <<
            "index:" << availableModes.mode_index <<
            "standard_mode:" << availableModes.standard_mode <<
            "advanced:" << advanced <<
            "cannotBeSet:" << cannotBeSet <<
            "custom_mode:" << availableModes.custom_mode;

        _modeList += FirmwareFlightMode{
            name,
            availableModes.standard_mode,
            availableModes.custom_mode,
            !cannotBeSet,
            advanced,
            true,  // fixed wing - Since we don't know at this point we assume fixed wing support
            true   // multi-rotor - Since we don't know at this point we assume multi-rotor support as well
        };

        if (availableModes.mode_index >= availableModes.number_modes) { // We are done
            qCDebug(StandardModesLog) << "Completed, num modes:" << availableModes.number_modes;
            ensureUniqueModeNames();
            _vehicle->firmwarePlugin()->updateAvailableFlightModes(_modeList);
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
    for (auto iter = _modeList.begin(); iter != _modeList.end(); ++iter) {
        int duplicateIdx = 0;
        for (auto iter2 = std::next(iter); iter2 != _modeList.end(); ++iter2) {
            if ((*iter).mode_name == (*iter2).mode_name) {
                (*iter2).mode_name += QStringLiteral(" (%1)").arg(duplicateIdx + 1);
                ++duplicateIdx;
            }
        }
    }
}

void StandardModes::request()
{
    if (_requestActive) {
        // If we are in the middle of waiting for a request, wait for the response first
        _wantReset = true;
        return;
    }

    qCDebug(StandardModesLog) << "Requesting available modes";
    // Request one at a time. This could be improved by requesting all, but we can't use Vehicle::requestMessage for that
    StandardModes::requestMode(1);
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
