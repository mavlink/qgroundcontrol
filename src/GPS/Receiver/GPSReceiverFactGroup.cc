#include "GPSReceiverFactGroup.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverFactGroupLog, "GPS.Receiver.GPSReceiverFactGroup")

GPSReceiverFactGroup::GPSReceiverFactGroup(QObject* parent, GPSRuntimeScheduler* scheduler)
    : GPSPositionFactGroup(parent, scheduler)
    , _rtk(this)
{
    qCDebug(GPSReceiverFactGroupLog) << this;

    _nameToFactMetaDataMap.insert(
        FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Vehicle/GPSReceiverFact.json"), this));
    _addFactGroup(&_rtk, QStringLiteral("rtk"));
    _addFact(&_connectedFact);
    _addFact(&_numSatellitesUsedFact);
    _addFact(&_lastErrorFact);
    numSatellites()->setRawValue(-1);
    numSatellitesUsed()->setRawValue(-1);
}

GPSReceiverFactGroup::~GPSReceiverFactGroup()
{
    qCDebug(GPSReceiverFactGroupLog) << this;
}
