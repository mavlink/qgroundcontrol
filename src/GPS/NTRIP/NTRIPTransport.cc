#include "NTRIPTransport.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPTransportLog, "GPS.NTRIP.NTRIPTransport")

NTRIPTransport::NTRIPTransport(QObject* parent) : QObject(parent)
{
    qCDebug(NTRIPTransportLog) << this;
}

NTRIPTransport::~NTRIPTransport()
{
    qCDebug(NTRIPTransportLog) << this;
}
