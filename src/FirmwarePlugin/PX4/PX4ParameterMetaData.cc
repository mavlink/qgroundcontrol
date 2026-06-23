#include "PX4ParameterMetaData.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace Qt::StringLiterals;

QGC_LOGGING_CATEGORY(PX4ParameterMetaDataLog, "FirmwarePlugin.PX4ParameterMetaData")

PX4ParameterMetaData::PX4ParameterMetaData(QObject* parent)
    : ParameterMetaData(parent)
{
    qCDebug(PX4ParameterMetaDataLog) << this;
}

PX4ParameterMetaData::~PX4ParameterMetaData()
{
    qCDebug(PX4ParameterMetaDataLog) << this;
}

void PX4ParameterMetaData::parseParameterJson(const QJsonObject &json)
{
    const int version = json.value(u"version").toInt();
    if (version < 1) {
        qCWarning(PX4ParameterMetaDataLog) << "Parameter JSON version too old:" << version;
        return;
    }

    const QJsonArray parameters = json.value(u"parameters").toArray();

    for (const QJsonValue &paramVal : parameters) {
        if (!paramVal.isObject()) {
            continue;
        }

        const QJsonObject param = paramVal.toObject();
        const QString name = param.value(u"name").toString();
        if (name.isEmpty()) {
            continue;
        }

        if (_cachedMetaData.contains(name)) {
            qCWarning(PX4ParameterMetaDataLog) << "Duplicate parameter:" << name;
            _cachedMetaData.take(name)->deleteLater();
        }

        FactMetaData *metaData = FactMetaData::createFromJsonObject(param, kEmptyDefines, this);
        if (metaData->name().isEmpty()) {
            qCWarning(PX4ParameterMetaDataLog) << "Skipping invalid parameter metadata:" << name;
            metaData->deleteLater();
            continue;
        }
        _postProcessMetaData(name, metaData); // applied here; getMetaDataForFact() hits cache so won't re-apply
        _cachedMetaData[name] = metaData;
    }
}

void PX4ParameterMetaData::_postProcessMetaData(const QString &name, FactMetaData *metaData)
{
    Q_UNUSED(name)

    if (metaData->category().isEmpty() || metaData->category() == FactMetaData::defaultCategory()) {
        metaData->setCategory(u"Standard"_s);
    }

    if (metaData->volatileValue()) {
        metaData->setReadOnly(true);
    }

    QString shortDesc = metaData->shortDescription();
    if (shortDesc.contains(u'\n')) {
        metaData->setShortDescription(shortDesc.replace(u'\n', u' '));
    }
    QString longDesc = metaData->longDescription();
    if (longDesc.contains(u'\n')) {
        metaData->setLongDescription(longDesc.replace(u'\n', u' '));
    }
}
