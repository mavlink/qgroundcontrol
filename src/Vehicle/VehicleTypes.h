#pragma once

/// @file VehicleTypes.h
/// Lightweight header containing types extracted from the Vehicle class.
/// Including this instead of the full Vehicle.h avoids pulling in the 1400+
/// line Vehicle class and all its transitive includes, which significantly
/// reduces moc parse time for headers that only need these definitions.
///
/// Vehicle inherits from VehicleTypes so that existing code using
/// Vehicle::MavCmdResultFailureCode_t etc. continues to work unchanged.

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
};
