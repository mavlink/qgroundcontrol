#pragma once

enum class GPSNativeOpenStatus
{
    Opened,
    TimedOut,
    Cancelled,
    Error,
    Unsupported
};

enum class GPSNativeReadStatus
{
    Data,
    TimedOut,
    Cancelled,
    Closed,
    Error,
    Overflow,
    InvalidData
};

enum class GPSNativeWriteStatus
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
