#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

enum class GPSOpenStatus
{
    Opened,
    TimedOut,
    Cancelled,
    Error,
    Unsupported
};

struct GPSOpenResult
{
    GPSOpenStatus status = GPSOpenStatus::Unsupported;
    QString detail = {};
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

struct GPSReadResult
{
    GPSReadStatus status = GPSReadStatus::TimedOut;
    int bytesRead = 0;
    QString detail = {};
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

struct GPSWriteResult
{
    GPSWriteStatus status = GPSWriteStatus::Unsupported;
    int acceptedBytes = 0;
    int writtenBytes = 0;
    int uncertainBytes = 0;
    QString detail = {};
};

Q_DECLARE_METATYPE(GPSOpenResult)
Q_DECLARE_METATYPE(GPSReadResult)
Q_DECLARE_METATYPE(GPSWriteResult)
