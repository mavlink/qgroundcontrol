#pragma once

#include <QtCore/QString>

enum class GPSOpenStatus
{
    Opened,
    TimedOut,
    Cancelled,
    Error,
    Unsupported
};

enum class GPSReadStatus
{
    Data,
    TimedOut,
    Cancelled,
    Closed,
    Error,
    Overflow,
    InvalidData
};

enum class GPSWriteStatus
{
    Completed,
    TimedOut,
    Cancelled,
    Error,
    Unsupported,
    InvalidData
};

enum class [[nodiscard]] GPSBaudStatus
{
    Configured,
    Unsupported,
    Cancelled,
    Error
};

struct [[nodiscard]] GPSOpenResult
{
    GPSOpenStatus status = GPSOpenStatus::Unsupported;
    QString detail = {};
};

struct [[nodiscard]] GPSReadResult
{
    GPSReadStatus status = GPSReadStatus::TimedOut;
    int bytesRead = 0;
    QString detail = {};
};

struct [[nodiscard]] GPSWriteResult
{
    GPSWriteStatus status = GPSWriteStatus::Unsupported;
    int acceptedBytes = 0;
    int writtenBytes = 0;
    QString detail = {};

    /// Invalid counts return -1 rather than masking inconsistent evidence.
    [[nodiscard]] int uncertainBytes() const
    {
        return acceptedBytes >= 0 && writtenBytes >= 0 && writtenBytes <= acceptedBytes ? acceptedBytes - writtenBytes
                                                                                        : -1;
    }
};
