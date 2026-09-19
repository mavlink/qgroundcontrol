#pragma once

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

enum class GPSBaudStatus
{
    Configured,
    Unsupported,
    Cancelled,
    Error
};
