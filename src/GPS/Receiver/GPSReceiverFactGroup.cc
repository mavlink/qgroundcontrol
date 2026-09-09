#include "GPSReceiverFactGroup.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverFactGroupLog, "GPS.Receiver.GPSReceiverFactGroup")

GPSReceiverFactGroup::GPSReceiverFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSReceiverFact.json"), parent)
{
    qCDebug(GPSReceiverFactGroupLog) << this;

    _addFact(&_connectedFact);
    _addFact(&_numSatellitesFact);
    _addFact(&_numSatellitesUsedFact);
    _addFact(&_lastErrorFact);
}

GPSReceiverFactGroup::~GPSReceiverFactGroup()
{
    qCDebug(GPSReceiverFactGroupLog) << this;
}
