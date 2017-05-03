/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FactGroup.h"
#include "JsonHelper.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QQmlEngine>

QGC_LOGGING_CATEGORY(FactGroupLog, "FactGroupLog")

FactGroup::FactGroup(int updateRateMsecs, const QString& metaDataFile, QObject* parent)
    : QObject(parent)
    , _updateRateMSecs(updateRateMsecs)
{
    if (_updateRateMSecs > 0) {
        connect(&_updateTimer, &QTimer::timeout, this, &FactGroup::_updateAllValues);
        _updateTimer.setSingleShot(false);
        _updateTimer.start(_updateRateMSecs);
    }

    _loadMetaData(metaDataFile);
}

Fact* FactGroup::getFact(const QString& name)
{
    Fact* fact = NULL;

    if (name.contains(".")) {
        QStringList parts = name.split(".");
        if (parts.count() != 2) {
            qWarning() << "Only single level of hierarchy supported";
            return NULL;
        }

        FactGroup * factGroup = getFactGroup(parts[0]);
        if (!factGroup) {
            qWarning() << "Unknown FactGroup" << parts[0];
            return NULL;
        }

        return factGroup->getFact(parts[1]);
    }

    if (_nameToFactMap.contains(name)) {
        fact = _nameToFactMap[name];
        QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
    } else {
        qWarning() << "Unknown Fact" << name;
    }

    return fact;
}

FactGroup* FactGroup::getFactGroup(const QString& name)
{
    FactGroup* factGroup = NULL;

    if (_nameToFactGroupMap.contains(name)) {
        factGroup = _nameToFactGroupMap[name];
        QQmlEngine::setObjectOwnership(factGroup, QQmlEngine::CppOwnership);
    } else {
        qWarning() << "Unknown FactGroup" << name;
    }

    return factGroup;
}

void FactGroup::_addFact(Fact* fact, const QString& name)
{
    if (_nameToFactMap.contains(name)) {
        qWarning() << "Duplicate Fact" << name;
        return;
    }

    fact->setSendValueChangedSignals(_updateRateMSecs == 0);
    if (_nameToFactMetaDataMap.contains(name)) {
        fact->setMetaData(_nameToFactMetaDataMap[name]);
    }
    _nameToFactMap[name] = fact;
}

void FactGroup::_addFactGroup(FactGroup* factGroup, const QString& name)
{
    if (_nameToFactGroupMap.contains(name)) {
        qWarning() << "Duplicate FactGroup" << name;
        return;
    }

    _nameToFactGroupMap[name] = factGroup;
}

void FactGroup::_updateAllValues(void)
{
    foreach(Fact* fact, _nameToFactMap) {
        fact->sendDeferredValueChangedSignal();
    }
}

void FactGroup::_loadMetaData(const QString& jsonFilename)
{
    _nameToFactMetaDataMap = FactMetaData::createMapFromJsonFile(jsonFilename, this);
}
