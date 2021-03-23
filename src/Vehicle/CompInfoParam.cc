/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoParam.h"
#include "JsonHelper.h"
#include "FactMetaData.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(CompInfoParamLog, "CompInfoParamLog")

const char* CompInfoParam::_jsonParametersKey           = "parameters";
const char* CompInfoParam::_cachedMetaDataFilePrefix    = "ParameterFactMetaData";
const char* CompInfoParam::_indexedNameTag              = "{n}";

CompInfoParam::CompInfoParam(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_PARAMETER, compId, vehicle, parent)
{

}

void CompInfoParam::setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName)
{
    qCDebug(CompInfoParamLog) << "setJson: metadataJsonFileName:translationJsonFileName" << metadataJsonFileName << translationJsonFileName;

    if (metadataJsonFileName.isEmpty()) {
        // This will fall back to using the old FirmwarePlugin mechanism for parameter meta data.
        // In this case paramter metadata is loaded through the _parameterMajorVersionKnown call which happens after parameter are downloaded
        return;
    }

    QString         errorString;
    QJsonDocument   jsonDoc;

    _noJsonMetadata = false;

    if (!JsonHelper::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
        qCWarning(CompInfoParamLog) << "Metadata json file open failed: compid:" << compId << errorString;
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,   QJsonValue::Double, true },
        { _jsonParametersKey,           QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(jsonObj, keyInfoList, errorString)) {
        qCWarning(CompInfoParamLog) << "Metadata json validation failed: compid:" << compId << errorString;
        return;
    }

    int version = jsonObj[JsonHelper::jsonVersionKey].toInt();
    if (version != 1) {
        qCWarning(CompInfoParamLog) << "Metadata json unsupported version" << version;
        return;
    }

    QJsonArray rgParameters = jsonObj[_jsonParametersKey].toArray();
    for (QJsonValue parameterValue: rgParameters) {
        QMap<QString, QString> emptyDefineMap;

        if (!parameterValue.isObject()) {
            qCWarning(CompInfoParamLog) << "Metadata json read failed: compid:" << compId << "parameters array contains non-object";
            return;
        }

        FactMetaData* newMetaData = FactMetaData::createFromJsonObject(parameterValue.toObject(), emptyDefineMap, this);

        if (newMetaData->name().contains(_indexedNameTag)) {
            _indexedNameMetaDataList.append(RegexFactMetaDataPair_t(newMetaData->name(), newMetaData));
        } else {
            _nameToMetaDataMap[newMetaData->name()] = newMetaData;
        }
    }
}

FactMetaData* CompInfoParam::factMetaDataForName(const QString& name, FactMetaData::ValueType_t type)
{
    FactMetaData* factMetaData = nullptr;

    if (_noJsonMetadata) {
        QObject* opaqueMetaData = _getOpaqueParameterMetaData();
        if (opaqueMetaData) {
            factMetaData = vehicle->firmwarePlugin()->_getMetaDataForFact(opaqueMetaData, name, type, vehicle->vehicleType());
        }
    }

    if (!factMetaData) {
        if (_nameToMetaDataMap.contains(name)) {
            factMetaData = _nameToMetaDataMap[name];
        } else {
            // We didn't get any direct matches. Try an indexed name.
            for (int i=0; i<_indexedNameMetaDataList.count(); i++) {
                const RegexFactMetaDataPair_t& pair = _indexedNameMetaDataList[i];

                QString indexedName = pair.first;
                QString indexedRegex("(\\d+)");
                indexedName.replace(_indexedNameTag, indexedRegex);

                QRegularExpression      regex(indexedName);
                QRegularExpressionMatch match = regex.match(name);

                QStringList captured = match.capturedTexts();
                if (captured.count() == 2) {
                    factMetaData = new FactMetaData(*pair.second, this);
                    factMetaData->setName(name);

                    QString shortDescription = factMetaData->shortDescription();
                    shortDescription.replace(_indexedNameTag, captured[1]);
                    factMetaData->setShortDescription(shortDescription);
                    QString longDescription = factMetaData->shortDescription();
                    longDescription.replace(_indexedNameTag, captured[1]);
                    factMetaData->setLongDescription(longDescription);
                }
            }

            if (!factMetaData) {
                factMetaData = new FactMetaData(type, this);
                int i = name.indexOf("_");
                if (i > 0) {
                    factMetaData->setGroup(name.left(i));
                }
                if (compId != MAV_COMP_ID_AUTOPILOT1) {
                    factMetaData->setCategory(tr("Component %1").arg(compId));
                }
            }
            _nameToMetaDataMap[name] = factMetaData;
        }
    }

    return factMetaData;
}

FirmwarePlugin* CompInfoParam::_anyVehicleTypeFirmwarePlugin(MAV_AUTOPILOT firmwareType)
{
    FirmwarePluginManager*  pluginMgr               = qgcApp()->toolbox()->firmwarePluginManager();
    MAV_TYPE                anySupportedVehicleType = QGCMAVLink::vehicleClassToMavType(pluginMgr->supportedVehicleClasses(QGCMAVLink::firmwareClass(firmwareType))[0]);

    return pluginMgr->firmwarePluginForAutopilot(firmwareType, anySupportedVehicleType);
}

QString CompInfoParam::_parameterMetaDataFile(Vehicle* vehicle, MAV_AUTOPILOT firmwareType, int& majorVersion, int& minorVersion)
{
    bool            cacheHit            = false;
    int             wantedMajorVersion  = 1;
    FirmwarePlugin* fwPlugin            = _anyVehicleTypeFirmwarePlugin(firmwareType);

    if (firmwareType != MAV_AUTOPILOT_PX4) {
        return fwPlugin->_internalParameterMetaDataFile(vehicle);
    } else {
        // Only PX4 support the old style cached metadata
        QSettings   settings;
        QDir        cacheDir = QFileInfo(settings.fileName()).dir();

        // First look for a direct cache hit
        int cacheMinorVersion, cacheMajorVersion;
        QFile cacheFile(cacheDir.filePath(QString("%1.%2.%3.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType).arg(wantedMajorVersion)));
        if (cacheFile.exists()) {
            fwPlugin->_getParameterMetaDataVersionInfo(cacheFile.fileName(), cacheMajorVersion, cacheMinorVersion);
            if (wantedMajorVersion != cacheMajorVersion) {
                qWarning() << "Parameter meta data cache corruption:" << cacheFile.fileName() << "major version does not match file name" << "actual:excepted" << cacheMajorVersion << wantedMajorVersion;
            } else {
                qCDebug(CompInfoParamLog) << "Direct cache hit on file:major:minor" << cacheFile.fileName() << cacheMajorVersion << cacheMinorVersion;
                cacheHit = true;
            }
        }

        if (!cacheHit) {
            // No direct hit, look for lower param set version
            QString wildcard = QString("%1.%2.*.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType);
            QStringList cacheHits = cacheDir.entryList(QStringList(wildcard), QDir::Files, QDir::Name);

            // Find the highest major version number which is below the vehicles major version number
            int cacheHitIndex = -1;
            cacheMajorVersion = -1;
            QRegExp regExp(QString("%1\\.%2\\.(\\d*)\\.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType));
            for (int i=0; i< cacheHits.count(); i++) {
                if (regExp.exactMatch(cacheHits[i]) && regExp.captureCount() == 1) {
                    int majorVersion = regExp.capturedTexts()[0].toInt();
                    if (majorVersion > cacheMajorVersion && majorVersion < wantedMajorVersion) {
                        cacheMajorVersion = majorVersion;
                        cacheHitIndex = i;
                    }
                }
            }

            if (cacheHitIndex != -1) {
                // We have a cache hit on a lower major version, read minor version as well
                int majorVersion;
                cacheFile.setFileName(cacheDir.filePath(cacheHits[cacheHitIndex]));
                fwPlugin->_getParameterMetaDataVersionInfo(cacheFile.fileName(), majorVersion, cacheMinorVersion);
                if (majorVersion != cacheMajorVersion) {
                    qWarning() << "Parameter meta data cache corruption:" << cacheFile.fileName() << "major version does not match file name" << "actual:excepted" << majorVersion << cacheMajorVersion;
                    cacheHit = false;
                } else {
                    qCDebug(CompInfoParamLog) << "Indirect cache hit on file:major:minor:want" << cacheFile.fileName() << cacheMajorVersion << cacheMinorVersion << wantedMajorVersion;
                    cacheHit = true;
                }
            }
        }

        int internalMinorVersion, internalMajorVersion;
        QString internalMetaDataFile = fwPlugin->_internalParameterMetaDataFile(vehicle);
        fwPlugin->_getParameterMetaDataVersionInfo(internalMetaDataFile, internalMajorVersion, internalMinorVersion);
        qCDebug(CompInfoParamLog) << "Internal metadata file:major:minor" << internalMetaDataFile << internalMajorVersion << internalMinorVersion;
        if (cacheHit) {
            // Cache hit is available, we need to check if internal meta data is a better match, if so use internal version
            if (internalMajorVersion == wantedMajorVersion) {
                if (cacheMajorVersion == wantedMajorVersion) {
                    // Both internal and cache are direct hit on major version, Use higher minor version number
                    cacheHit = cacheMinorVersion > internalMinorVersion;
                } else {
                    // Direct internal hit, but not direct hit in cache, use internal
                    cacheHit = false;
                }
            } else {
                if (cacheMajorVersion == wantedMajorVersion) {
                    // Direct hit on cache, no direct hit on internal, use cache
                    cacheHit = true;
                } else {
                    // No direct hit anywhere, use internal
                    cacheHit = false;
                }
            }
        }

        QString metaDataFile;
        if (cacheHit && !qgcApp()->runningUnitTests()) {
            majorVersion = cacheMajorVersion;
            minorVersion = cacheMinorVersion;
            metaDataFile = cacheFile.fileName();
        } else {
            majorVersion = internalMajorVersion;
            minorVersion = internalMinorVersion;
            metaDataFile = internalMetaDataFile;
        }
        qCDebug(CompInfoParamLog) << "_parameterMetaDataFile returning file:major:minor" << metaDataFile << majorVersion << minorVersion;

        return metaDataFile;
    }
}

void CompInfoParam::_cachePX4MetaDataFile(const QString& metaDataFile)
{
    FirmwarePlugin* plugin = _anyVehicleTypeFirmwarePlugin(MAV_AUTOPILOT_PX4);

    int newMajorVersion, newMinorVersion;
    plugin->_getParameterMetaDataVersionInfo(metaDataFile, newMajorVersion, newMinorVersion);
    if (newMajorVersion != 1) {
        newMajorVersion = 1;
        qgcApp()->showAppMessage(tr("Internal Error: Parameter MetaData major must be 1"));
    }
    qCDebug(CompInfoParamLog) << "ParameterManager::cacheMetaDataFile file:major;minor" << metaDataFile << newMajorVersion << newMinorVersion;

    // Find the cache hit closest to this new file
    int cacheMajorVersion, cacheMinorVersion;
    QString cacheHit = _parameterMetaDataFile(nullptr, MAV_AUTOPILOT_PX4, cacheMajorVersion, cacheMinorVersion);
    qCDebug(CompInfoParamLog) << "ParameterManager::cacheMetaDataFile cacheHit file:firmware:major;minor" << cacheHit << cacheMajorVersion << cacheMinorVersion;

    bool cacheNewFile = false;
    if (cacheHit.isEmpty()) {
        // No cache hits, store the new file
        cacheNewFile = true;
    } else if (cacheMajorVersion == newMajorVersion) {
        // Direct hit on major version in cache:
        //      Cache new file if newer/equal minor version. We cache if equal to allow flashing test builds with new parameter metadata.
        //      Also delete older cache file.
        if (newMinorVersion >= cacheMinorVersion) {
            cacheNewFile = true;
            QFile::remove(cacheHit);
        }
    } else {
        // Indirect hit in cache, store new file
        cacheNewFile = true;
    }

    if (cacheNewFile) {
        // Cached files are stored in settings location. Copy from current file to cache naming.

        QSettings settings;
        QDir cacheDir = QFileInfo(settings.fileName()).dir();
        QFile cacheFile(cacheDir.filePath(QString("%1.%2.%3.xml").arg(_cachedMetaDataFilePrefix).arg(MAV_AUTOPILOT_PX4).arg(newMajorVersion)));
        qCDebug(CompInfoParamLog) << "ParameterManager::cacheMetaDataFile caching file:" << cacheFile.fileName();
        QFile newFile(metaDataFile);
        newFile.copy(cacheFile.fileName());
    }
}

QObject* CompInfoParam::_getOpaqueParameterMetaData(void)
{
    if (!_noJsonMetadata) {
        qWarning() << "CompInfoParam::_getOpaqueParameterMetaData _noJsonMetadata == false";
    }

    if (!_opaqueParameterMetaData && compId == MAV_COMP_ID_AUTOPILOT1) {
        // Load best parameter meta data set
        int majorVersion, minorVersion;
        QString metaDataFile = _parameterMetaDataFile(vehicle, vehicle->firmwareType(), majorVersion, minorVersion);
        qCDebug(CompInfoParamLog) << "Loading meta data the old way file" << metaDataFile;
        _opaqueParameterMetaData = vehicle->firmwarePlugin()->_loadParameterMetaData(metaDataFile);
    }

    return _opaqueParameterMetaData;
}
