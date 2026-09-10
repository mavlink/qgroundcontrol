#pragma once

#include "GPSProtocol.h"

// Existing receiver scripts retain their event vocabulary; only this fixture adapter
// translates it into the production typed protocol services.
struct GPSReadRequest
{
    uint8_t* buffer;
    int capacity;
    int timeoutMs;
};

enum class GPSCallbackType
{
    /**
     * Read data from device. This is a blocking operation with a timeout.
     * data1: points to a GPSReadRequest; timeout and storage are independent.
     * data2: buffer length in bytes. Less bytes than this can be read.
     * return: num read bytes, 0 on timeout (the method can actually also return 0 before
     *         the timeout happens).
     *         GPSProtocol::ReadCancelled on intentional shutdown, other negative values on error.
     */
    readDeviceData = 0,

    /**
     * Write data to device
     * data1: data to be written
     * data2: number of bytes to write
     * return: num written bytes
     */
    writeDeviceData,

    /**
     * set Baudrate
     * data1: ignored
     * data2: baudrate
     * return: 0 on success
     */
    setBaudrate,

    /**
     * Got an RTCM message from the device.
     * data1: pointer to the message
     * data2: message length
     * return: ignored
     */
    gotRTCMMessage,

    /**
     * Got a relative position message from the device.
     * data1: pointer to the message
     * data2: message length
     * return: ignored
     */
    gotRelativePositionMessage,

    /**
     * message about current survey-in status
     * data1: points to a SurveyInStatus struct
     * data2: ignored
     * return: ignored
     */
    surveyInStatus,

    /**
     * can be used to set the current clock accurately
     * data1: pointer to a timespec struct
     * data2: ignored
     * return: ignored
     */
    setClock,
};

typedef int (*GPSCallbackPtr)(GPSCallbackType type, void* data1, int data2, void* user);
using SurveyInStatus = GPSSurveyReport;

inline GPSProtocolIO makeGPSProtocolTestIO(GPSCallbackPtr callback, void* user)
{
    GPSProtocolIO io;
    io.nowUs = [] { return gps_absolute_time(); };
    io.wait = [](std::chrono::microseconds duration) {
        gps_usleep(duration.count());
        return true;
    };
    if (!callback)
        return io;
    io.read = [callback, user](std::span<uint8_t> bytes, GPSProtocolDeadline deadline) {
        GPSReadRequest request{bytes.data(), static_cast<int>(bytes.size()),
                               deadline.remainingMilliseconds(gps_absolute_time())};
        const int result = callback(GPSCallbackType::readDeviceData, &request, request.capacity, user);
        return GPSProtocolReadResult{result > 0                             ? GPSReadStatus::Data
                                     : result == 0                          ? GPSReadStatus::TimedOut
                                     : result == GPSProtocol::ReadCancelled ? GPSReadStatus::Cancelled
                                                                            : GPSReadStatus::Error,
                                     result > 0 ? result : 0};
    };
    io.write = [callback, user](std::span<const uint8_t> bytes, GPSProtocolDeadline) {
        const int result =
            callback(GPSCallbackType::writeDeviceData, const_cast<uint8_t*>(bytes.data()), bytes.size(), user);
        return GPSProtocolWriteResult{result >= 0                            ? GPSWriteStatus::Completed
                                      : result == -1                         ? GPSWriteStatus::Unsupported
                                      : result == GPSProtocol::ReadCancelled ? GPSWriteStatus::Cancelled
                                                                             : GPSWriteStatus::Error,
                                      std::max(result, 0), std::max(result, 0), 0};
    };
    io.setBaudrate = [callback, user](unsigned baudrate) {
        const int result = callback(GPSCallbackType::setBaudrate, nullptr, baudrate, user);
        return result >= 0                            ? GPSBaudStatus::Configured
               : result == -1                         ? GPSBaudStatus::Unsupported
               : result == GPSProtocol::ReadCancelled ? GPSBaudStatus::Cancelled
                                                      : GPSBaudStatus::Error;
    };
    io.rtcm = [callback, user](std::span<const uint8_t> bytes) {
        callback(GPSCallbackType::gotRTCMMessage, const_cast<uint8_t*>(bytes.data()), bytes.size(), user);
    };
    io.relativePosition = [callback, user](const GPSRelativeReport& report) {
        callback(GPSCallbackType::gotRelativePositionMessage, const_cast<GPSRelativeReport*>(&report), sizeof(report),
                 user);
    };
    io.survey = [callback, user](const GPSSurveyReport& report) {
        callback(GPSCallbackType::surveyInStatus, const_cast<GPSSurveyReport*>(&report), 0, user);
    };
    return io;
}

inline int callGPSProtocolTestIO(GPSProtocolIO io, GPSCallbackType type, void* data, int size)
{
    switch (type) {
        case GPSCallbackType::readDeviceData: {
            int timeout = 0;
            std::memcpy(&timeout, data, sizeof(timeout));
            const auto result = io.read({static_cast<uint8_t*>(data), static_cast<size_t>(size)},
                                        {io.nowUs() + uint64_t(timeout) * 1000});
            return result.status == GPSReadStatus::Data        ? result.bytesRead
                   : result.status == GPSReadStatus::TimedOut  ? 0
                   : result.status == GPSReadStatus::Cancelled ? GPSProtocol::ReadCancelled
                                                               : -1;
        }
        case GPSCallbackType::writeDeviceData: {
            const auto result = io.write({static_cast<const uint8_t*>(data), static_cast<size_t>(size)}, {});
            return result.status == GPSWriteStatus::Completed   ? result.writtenBytes
                   : result.status == GPSWriteStatus::Cancelled ? GPSProtocol::ReadCancelled
                                                                : -1;
        }
        case GPSCallbackType::setBaudrate: {
            const auto result = io.setBaudrate(size);
            return result == GPSBaudStatus::Configured  ? 0
                   : result == GPSBaudStatus::Cancelled ? GPSProtocol::ReadCancelled
                                                        : -1;
        }
        case GPSCallbackType::gotRTCMMessage:
            io.rtcm({static_cast<uint8_t*>(data), static_cast<size_t>(size)});
            break;
        case GPSCallbackType::gotRelativePositionMessage:
            if (data && size == sizeof(GPSRelativeReport))
                io.relativePosition(*static_cast<GPSRelativeReport*>(data));
            break;
        case GPSCallbackType::surveyInStatus:
            if (data)
                io.survey(*static_cast<GPSSurveyReport*>(data));
            break;
        default:
            break;
    }
    return 0;
}
