#pragma once

#include <QtCore/QString>

#include "GPSIOStatus.h"

struct GPSOpenResult
{
    GPSOpenStatus status = GPSOpenStatus::Unsupported;
    QString detail = {};
};

struct GPSReadResult
{
    GPSReadStatus status = GPSReadStatus::TimedOut;
    int bytesRead = 0;
    QString detail = {};
};

struct GPSWriteResult
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
