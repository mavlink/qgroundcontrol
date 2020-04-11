/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    _setupTimer();
    _nameToFactMetaDataMap = FactMetaData::createMapFromJsonFile(metaDataFile, this);
}

FactGroup::FactGroup(int updateRateMsecs, QObject* parent)
    : QObject(parent)
    , _updateRateMSecs(updateRateMsecs)
{
    _setupTimer();
}

void FactGroup::_loadFromJsonArray(const QJsonArray jsonArray)
{
    QMap<QString, QString> defineMap;
    _nameToFactMetaDataMap = FactMetaData::createMapFromJsonArray(jsonArray, defineMap, this);
}

void FactGroup::_setupTimer()
{
    if (_updateRateMSecs > 0) {
        connect(&_updateTimer, &QTimer::timeout, this, &FactGroup::_updateAllValues);
        _updateTimer.setSingleShot(false);
        _updateTimer.setInterval(_updateRateMSecs);
        _updateTimer.start();
    }
}

Fact* FactGroup::getFact(const QString& name)
{
    if (name.contains(".")) {
        QStringList parts = name.split(".");
        if (parts.count() != 2) {
            qWarning() << "Only single level of hierarchy supported";
            return nullptr;
        }

        FactGroup * factGroup = getFactGroup(parts[0]);
        if (!factGroup) {
            qWarning() << "Unknown FactGroup" << parts[0];
            return nullptr;
        }

        return factGroup->getFact(parts[1]);
    }

    Fact*   fact =          nullptr;
    QString camelCaseName = _camelCase(name);

    if (_nameToFactMap.contains(camelCaseName)) {
        fact = _nameToFactMap[camelCaseName];
        QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
    } else {
        qWarning() << "Unknown Fact" << camelCaseName;
    }

    return fact;
}

FactGroup* FactGroup::getFactGroup(const QString& name)
{
    FactGroup*  factGroup = nullptr;
    QString     camelCaseName = _camelCase(name);

    if (_nameToFactGroupMap.contains(camelCaseName)) {
        factGroup = _nameToFactGroupMap[camelCaseName];
        QQmlEngine::setObjectOwnership(factGroup, QQmlEngine::CppOwnership);
    } else {
        qWarning() << "Unknown FactGroup" << camelCaseName;
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
    _factNames.append(name);
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
    for(Fact* fact: _nameToFactMap) {
        fact->sendDeferredValueChangedSignal();
    }
}

void FactGroup::setLiveUpdates(bool liveUpdates)
{
    if (_updateTimer.interval() == 0) {
        return;
    }

    if (liveUpdates) {
        _updateTimer.stop();
    } else {
        _updateTimer.start();
    }
    for(Fact* fact: _nameToFactMap) {
        fact->setSendValueChangedSignals(liveUpdates);
    }
}


QString FactGroup::_camelCase(const QString& text)
{
    return text[0].toLower() + text.right(text.length() - 1);
}
