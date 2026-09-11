#pragma once

#include <QtCore/QMetaType>
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
    int uncertainBytes = 0;
    QString detail = {};
};

Q_DECLARE_METATYPE(GPSOpenResult)
Q_DECLARE_METATYPE(GPSReadResult)
Q_DECLARE_METATYPE(GPSWriteResult)
