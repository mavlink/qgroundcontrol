#pragma once

/// @file VehicleTypes.h
/// Lightweight header containing types extracted from the Vehicle class.
/// Including this instead of the full Vehicle.h avoids pulling in the 1400+
/// line Vehicle class and all its transitive includes, which significantly
/// reduces moc parse time for headers that only need these definitions.
///
/// Vehicle inherits from VehicleTypes so that existing code using
/// Vehicle::MavCmdResultFailureCode_t etc. continues to work unchanged.

#include "MAVLinkEnums.h"
#include "QGCMAVLinkTypes.h"

struct VehicleTypes
{
    typedef enum {
        MavCmdResultCommandResultOnly,          ///< commandResult specifies full success/fail info
        MavCmdResultFailureNoResponseToCommand, ///< No response from vehicle to command
        MavCmdResultFailureDuplicateCommand,    ///< Unable to send command since duplicate is already being waited on for response
    } MavCmdResultFailureCode_t;

    typedef enum {
        RequestMessageNoFailure,
        RequestMessageFailureCommandError,
        RequestMessageFailureCommandNotAcked,
        RequestMessageFailureMessageNotReceived,
        RequestMessageFailureDuplicate,           ///< Exact duplicate request already active or queued for this component/message id
    } RequestMessageResultHandlerFailureCode_t;

    static const int versionNotSetValue = -1;

    /// Callback for sendMavCommandWithHandler which handles MAV_RESULT_IN_PROGRESS acks.
    typedef void (*MavCmdProgressHandler)(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);

    /// Callback for sendMavCommandWithHandler which handles all non-IN_PROGRESS acks.
    typedef void (*MavCmdResultHandler)(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);

    /// Callback info bundle for sendMavCommandWithHandler.
    typedef struct MavCmdAckHandlerInfo_s {
        MavCmdResultHandler     resultHandler;          ///< nullptr for no handler
        void*                   resultHandlerData;
        MavCmdProgressHandler   progressHandler;
        void*                   progressHandlerData;    ///< nullptr for no handler
    } MavCmdAckHandlerInfo_t;

    /// Callback for requestMessage — delivered when the ack/message pair resolves or a failure occurs.
    typedef void (*RequestMessageResultHandler)(void* resultHandlerData, MAV_RESULT commandResult, RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);
};
