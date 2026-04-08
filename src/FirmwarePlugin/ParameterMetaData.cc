#include "ParameterMetaData.h"
#include "JsonParsing.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(ParameterMetaDataLog, "FirmwarePlugin.ParameterMetaData")

const FactMetaData::DefineMap_t ParameterMetaData::kEmptyDefines;

ParameterMetaData::ParameterMetaData(QObject *parent)
    : QObject(parent)
{
    qCDebug(ParameterMetaDataLog) << this;
}

ParameterMetaData::~ParameterMetaData()
{
    qCDebug(ParameterMetaDataLog) << this;
}

void ParameterMetaData::loadParameterFactMetaDataFile(const QString &metaDataFile)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (Q_UNLIKELY(QThread::currentThread() != thread())) {
        qCCritical(ParameterMetaDataLog) << "loadParameterFactMetaDataFile called from wrong thread";
        return;
    }

    if (_parameterMetaDataLoaded) {
        return;
    }

    qCDebug(ParameterMetaDataLog) << "Loading parameter meta data:" << metaDataFile;

    QJsonDocument doc;
    QString errorString;
    if (!JsonParsing::isJsonFile(metaDataFile, doc, errorString)) {
        qCWarning(ParameterMetaDataLog) << "Unable to open parameter meta data file:" << metaDataFile << errorString;
        return;
    }
    if (!doc.isObject()) {
        qCWarning(ParameterMetaDataLog) << "JSON root is not an object:" << metaDataFile;
        return;
    }

    _parameterMetaDataLoaded = true;
    parseParameterJson(doc.object());
}

FactMetaData *ParameterMetaData::getMetaDataForFact(const QString &name, FactMetaData::ValueType_t type)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (Q_UNLIKELY(QThread::currentThread() != thread())) {
        qCCritical(ParameterMetaDataLog) << "getMetaDataForFact called from wrong thread";
        return nullptr;
    }

    if (FactMetaData *cached = _cachedMetaData.value(name)) {
        return cached;
    }

    FactMetaData *metaData = _lookupMetaData(name, type);
    if (!metaData) {
        metaData = _createDefaultMetaData(name, type);
    }

    _postProcessMetaData(name, metaData);
    _cachedMetaData[name] = metaData;
    return metaData;
}

QVersionNumber ParameterMetaData::versionFromJsonData(const QByteArray &jsonData)
{
    return versionFromJsonData(jsonData, nullptr);
}

QVersionNumber ParameterMetaData::versionFromJsonData(const QByteArray &jsonData, bool *validJson)
{
    QJsonDocument doc;
    QString errorString;
    const bool parsed = JsonParsing::isJsonFile(jsonData, doc, errorString);

    if (validJson) {
        *validJson = parsed;
    }

    if (!parsed) {
        qCDebug(ParameterMetaDataLog) << "JSON parse error extracting version:" << errorString;
        return {};
    }

    const QJsonObject root = doc.object();

    // Only honour explicit parameter-catalog version stamps.
    // The top-level "version" key is a schema version (e.g. PX4 JSON
    // always has "version":1) and must NOT be conflated with the
    // parameter-set version used for cache selection.
    const int major = root.value(u"parameter_version_major").toInt(-1);
    const int minor = root.value(u"parameter_version_minor").toInt(0);
    if (major >= 0) {
        return QVersionNumber(major, minor);
    }

    return {};
}

QVersionNumber ParameterMetaData::versionFromMetaDataFile(const QString &metaDataFile)
{
    QString errorString;
    const QByteArray data = QGCFileHelper::readFile(metaDataFile, &errorString);
    if (data.isEmpty()) {
        qCWarning(ParameterMetaDataLog) << "Failed to read parameter meta data file:" << metaDataFile << errorString;
        return {};
    }

    return versionFromJsonData(data);
}

QVersionNumber ParameterMetaData::versionFromFileName(const QString &fileName)
{
    static const QRegularExpression regex(QStringLiteral("\\.(\\d+)\\.(\\d+)\\.json$"));
    const QRegularExpressionMatch match = regex.match(fileName);
    if (match.hasMatch()) {
        return QVersionNumber(match.captured(1).toInt(), match.captured(2).toInt());
    }
    return {};
}

FactMetaData *ParameterMetaData::_lookupMetaData(const QString &name, FactMetaData::ValueType_t type)
{
    Q_UNUSED(name)
    Q_UNUSED(type)
    return nullptr;
}

FactMetaData *ParameterMetaData::_createDefaultMetaData(const QString &name, FactMetaData::ValueType_t type)
{
    Q_UNUSED(name)
    return new FactMetaData(type, this);
}

void ParameterMetaData::_postProcessMetaData(const QString &name, FactMetaData *metaData)
{
    Q_UNUSED(name)
    Q_UNUSED(metaData)
}

bool ParameterMetaData::setRawConvertedValue(FactMetaData *metaData, const QString &rawText, void (FactMetaData::*setter)(const QVariant &))
{
    QVariant converted;
    QString errorString;
    if (metaData->convertAndValidateRaw(rawText, false, converted, errorString)) {
        (metaData->*setter)(converted);
        return true;
    }
    qCDebug(ParameterMetaDataLog) << "Invalid value for" << metaData->name() << "raw:" << rawText << "error:" << errorString;
    return false;
}

void ParameterMetaData::setEnumFromPairs(FactMetaData *metaData, const QList<ValueDescPair> &pairs)
{
    QStringList enumStrings;
    QVariantList enumValues;

    for (const auto &[code, description] : pairs) {
        QVariant enumValue;
        QString errorString;
        if (metaData->convertAndValidateRaw(code, false, enumValue, errorString)) {
            enumValues << enumValue;
            enumStrings << description;
        } else {
            qCWarning(ParameterMetaDataLog) << "Skipping invalid enum value for" << metaData->name() << "code:" << code << "error:" << errorString;
        }
    }

    if (!enumStrings.isEmpty()) {
        metaData->setEnumInfo(enumStrings, enumValues);
    }
}

void ParameterMetaData::setBitmaskFromPairs(FactMetaData *metaData, const QList<ValueDescPair> &pairs)
{
    QStringList bitmaskStrings;
    QVariantList bitmaskValues;

    for (const auto &[bitIndexStr, description] : pairs) {
        bool ok = false;
        const uint bitIndex = bitIndexStr.toUInt(&ok);
        if (!ok) {
            qCWarning(ParameterMetaDataLog) << "Skipping invalid bitmask index for" << metaData->name() << "value:" << bitIndexStr;
            continue;
        }
        if (bitIndex >= 64) {
            qCWarning(ParameterMetaDataLog) << "Skipping out-of-range bitmask index for" << metaData->name() << "bit:" << bitIndex;
            continue;
        }

        const QVariant rawValue = QVariant::fromValue(static_cast<quint64>(1ull << bitIndex));
        QVariant bitmaskValue;
        QString errorString;
        if (metaData->convertAndValidateRaw(rawValue, false, bitmaskValue, errorString)) {
            bitmaskValues << bitmaskValue;
            bitmaskStrings << description;
        } else {
            qCWarning(ParameterMetaDataLog) << "Skipping invalid bitmask value for" << metaData->name() << "bit:" << bitIndex << "error:" << errorString;
        }
    }

    if (!bitmaskStrings.isEmpty()) {
        metaData->setBitmaskInfo(bitmaskStrings, bitmaskValues);
    }
}
