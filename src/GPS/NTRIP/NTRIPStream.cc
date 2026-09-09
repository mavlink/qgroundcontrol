#include "NTRIPStream.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPStreamLog, "GPS.NTRIP.NTRIPStream")

NTRIPStream::NTRIPStream(QObject* parent)
    : QObject(parent)
{
    qCDebug(NTRIPStreamLog) << this;
}

NTRIPStream::~NTRIPStream()
{
    qCDebug(NTRIPStreamLog) << this;
}
