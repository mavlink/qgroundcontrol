/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMParameterMetaData.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QXmlStreamReader>

QGC_LOGGING_CATEGORY(APMParameterMetaDataLog, "qgc.firmwareplugin.apm.apmparametermetadata")

using namespace Qt::Literals::StringLiterals;

APMParameterMetaData::APMParameterMetaData(QObject *parent)
    : ParameterMetaData(parent)
{
    qCDebug(APMParameterMetaDataLog) << this;
}

APMParameterMetaData::~APMParameterMetaData()
{
    qCDebug(APMParameterMetaDataLog) << this;
}

QString APMParameterMetaData::_mavTypeToString(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_DECAROTOR:
    case MAV_TYPE_DODECAROTOR:
    case MAV_TYPE_COAXIAL:
        return u"ArduCopter"_s;

    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_FIXEDROTOR:
    case MAV_TYPE_VTOL_TAILSITTER:
    case MAV_TYPE_VTOL_TILTWING:
    case MAV_TYPE_VTOL_RESERVED5:
        return u"ArduPlane"_s;

    case MAV_TYPE_GROUND_ROVER:
    case MAV_TYPE_SURFACE_BOAT:
        return u"Rover"_s;

    case MAV_TYPE_SUBMARINE:
        return u"ArduSub"_s;

    case MAV_TYPE_ANTENNA_TRACKER:
        return u"AntennaTracker"_s;

    case MAV_TYPE_AIRSHIP:
        return u"Blimp"_s;

    default:
        return QString();
    }
}

QString APMParameterMetaData::_groupFromParameterName(const QString &name)
{
    static const QRegularExpression re(uR"([0-9]+$)"_s);
    return name.section('_', 0, 0).remove(re);
}

bool APMParameterMetaData::_parseRange(QStringView txt, APMFactMetaDataRaw &out)
{
    static const QRegularExpression sep(uR"((\s+|to|-))"_s);
    const QList<QStringView> parts = txt.split(sep, Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        return false;
    }

    out.min = parts.at(0).trimmed().toString();
    out.max = parts.at(1).trimmed().toString();
    return true;
}

bool APMParameterMetaData::_parseBitmask(QStringView txt, APMFactMetaDataRaw &out)
{
    const QList<QStringView> items = txt.split(u',');

    for (const QStringView item : items) {
        const QList<QStringView> pair = item.split(u':');
        if (pair.size() != 2) {
            return false;
        }
        const APMFactMetaDataRaw::Bit bit = {
            pair.at(0).trimmed().toString(),
            pair.at(1).trimmed().toString()
        };
        out.bitmask.append(bit);
    }
    return true;
}

bool APMParameterMetaData::_parseParameterAttributes(QXmlStreamReader &xml, APMFactMetaDataRaw &raw)
{
    const auto parseField = [&](QStringView key, QStringView value) -> void {
        static const QHash<QStringView, std::function<void(QStringView)>> table = {
            { u"Range",          [&](QStringView txt) -> void { (void) _parseRange(txt, raw); } },
            { u"Increment",      [&](QStringView txt) -> void { raw.incrementSize = txt.toString(); } },
            { u"Units",          [&](QStringView txt) -> void { raw.units = txt.toString(); } },
            { u"UnitText",       [&](QStringView txt) -> void { raw.unitText = txt.toString(); } },
            { u"ReadOnly",       [&](QStringView txt) -> void { raw.readOnly = (txt.compare(u"true", Qt::CaseInsensitive) == 0); } },
            { u"Bitmask",        [&](QStringView txt) -> void { (void) _parseBitmask(txt, raw); } },
            { u"RebootRequired", [&](QStringView txt) -> void { raw.rebootRequired = (txt.compare(u"true", Qt::CaseInsensitive) == 0); } },
            { u"Values",         [&](QStringView txt) -> void { Q_UNUSED(txt); /* Do Nothing - Only Present in Older Versions */ } },
        };

        if (const auto it = table.constFind(key); it != table.constEnd()) {
            it.value()(value);
        } else {
            qCWarning(APMParameterMetaDataLog) << "Unknown <field name=\"" << key << "\">";
        }
    };

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType tokenType = xml.readNext();

        if (tokenType == QXmlStreamReader::StartElement) {
            const QStringView name = xml.name();
            const QXmlStreamAttributes attributes = xml.attributes();

            if (name == u"field") {
                const QStringView attr = attributes.value(u"name");
                const QStringView txt = xml.readElementText(QXmlStreamReader::SkipChildElements);
                parseField(attr, txt.trimmed());
            } else if (name == u"values") {
                // Do nothing
            } else if (name == u"value") {
                const QString code = attributes.value(u"code").toString();
                const QString desc = xml.readElementText();
                raw.values.append({ code, desc.trimmed() });
            } else if (name == u"bitmask") {
                // Do nothing
            } else if (name == u"bit") {
                // Do nothing
            } else {
                qCWarning(APMParameterMetaDataLog) << "Unknown parameter element in XML:" << name;
            }
        } else if (tokenType == QXmlStreamReader::EndElement) {
            if (xml.name() == u"param") {
                break;
            }
        }
    }

    if (xml.hasError()) {
        qCWarning(APMParameterMetaDataLog) << "XML parse error:" << xml.errorString();
        return false;
    }

    return true;
}

void APMParameterMetaData::_correctGroupMemberships(const ParamMap &paramToMeta, QHash<QString, QStringList> &groupMembers)
{
    for (auto it = groupMembers.constBegin(); it != groupMembers.constEnd(); ++it) {
        if (it->size() != 1) {
            continue;
        }

        const QString &loneParam = it->constFirst();
        const auto p = paramToMeta.constFind(loneParam);
        if (p != paramToMeta.constEnd() && p.value()) {
            p.value()->group = FactMetaData::defaultGroup();
        }
    }
    groupMembers.clear();
}

void APMParameterMetaData::loadFromFile(const QString &xmlPath)
{
    QWriteLocker guard(&_lock);

    if (_parsed) {
        qCWarning(APMParameterMetaDataLog) << "Parameter meta data loaded more than once";
        return;
    }
    _parsed = true;

    QFile file(xmlPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(APMParameterMetaDataLog) << "Cannot open parameter meta‑data file" << xmlPath;
        return;
    }

    QXmlStreamReader xml(file.readAll());
    file.close();

    XmlState state = XmlState::None;
    QString currentCategory;
    QHash<QString, QStringList> groupMembers;

    static const QRegularExpression wantedCat(uR"(ArduCopter|ArduPlane|APMrover2|Rover|ArduSub|AntennaTracker)"_s);

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType tokenType = xml.readNext();
        const QXmlStreamAttributes attributes = xml.attributes();

        if (tokenType == QXmlStreamReader::StartElement) {
            const QStringView tag = xml.name();

            if (tag == u"paramfile") {
                state = XmlState::ParamFileFound;
            } else if (tag == u"vehicles") {
                if (state != XmlState::ParamFileFound) {
                    return;
                }
                state = XmlState::FoundVehicles;
            } else if (tag == u"libraries") {
                if (state != XmlState::ParamFileFound) {
                    return;
                }
                state = XmlState::FoundLibraries;
                currentCategory = u"libraries"_s;
            } else if (tag == u"parameters") {
                if ((state != XmlState::FoundVehicles) && (state != XmlState::FoundLibraries)) {
                    return;
                }

                const QString attr = attributes.value(u"name").toString();
                if ((state == XmlState::FoundLibraries) || attr.contains(wantedCat)) {
                    state = XmlState::FoundParameters;
                    if ((state == XmlState::FoundParameters) && (currentCategory != u"libraries")) {
                        currentCategory = attr;
                    }
                } else {
                    xml.skipCurrentElement();
                }
            } else if (tag == u"param") {
                if (state != XmlState::FoundParameters) {
                    return;
                }

                state = XmlState::FoundParameter;

                QString name = attributes.value(u"name").toString();
                if (const int idx = name.indexOf(u':'); idx >= 0) {
                    (void) name.remove(0, idx + 1);
                }

                ParamMap &catMap = _params[currentCategory];
                APMFactMetaDataRaw *currentRaw = nullptr;
                if (const auto it = catMap.constFind(name); it != catMap.constEnd()) {
                    currentRaw = it.value().get();
                } else {
                    RawPtr raw = std::make_shared<APMFactMetaDataRaw>();
                    currentRaw = raw.get();
                    RawPtr *holder = &catMap[name];
                    *holder = std::move(raw);
                }

                const QString group = _groupFromParameterName(name);
                groupMembers[group] << name;

                currentRaw->name = name;
                currentRaw->group = group;
                currentRaw->category = attributes.value(u"user").toString();
                currentRaw->shortDescription = attributes.value(u"humanName").toString();
                currentRaw->longDescription = attributes.value(u"documentation").toString();

                (void) _parseParameterAttributes(xml, *currentRaw);
                state = XmlState::FoundParameters;
            }
        } else if (tokenType == QXmlStreamReader::EndElement) {
            if ((xml.name() == u"parameters") && (state == XmlState::FoundParameters)) {
                _correctGroupMemberships(_params[currentCategory], groupMembers);
                state = (currentCategory == u"libraries") ? XmlState::FoundLibraries : XmlState::FoundVehicles;
            }
        }
    }

    if (xml.hasError()) {
        qCWarning(APMParameterMetaDataLog) << "XML parse error:" << xml.errorString();
    }
}

FactMetaData *APMParameterMetaData::getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type)
{
    QReadLocker guard(&_lock);

    const QString vehicleStr = _mavTypeToString(vehicleType);
    const QString altVehicleStr = (vehicleStr == u"Rover") ? u"APMrover2"_s : QString();

    const auto findRaw = [&](const QString &cat) -> const APMFactMetaDataRaw* {
        if (const auto catIt = _params.constFind(cat);
            catIt != _params.constEnd()) {
            if (const auto pIt = catIt->constFind(paramName); pIt != catIt->constEnd()) {
                return pIt->get();
            }
        }
        return nullptr;
    };

    const APMFactMetaDataRaw *raw = findRaw(vehicleStr);
    if (!raw && !altVehicleStr.isEmpty()) {
        raw = findRaw(altVehicleStr);
    }
    if (!raw) {
        raw = findRaw(u"libraries"_s);
    }

    FactMetaData *meta = new FactMetaData(type, this);

    if (!raw) {
        meta->setCategory(u"Advanced"_s);
        meta->setGroup(_groupFromParameterName(paramName));
        return meta;
    }

    meta->setName(raw->name);
    meta->setGroup(raw->group);
    meta->setReadOnly(raw->readOnly);
    meta->setVehicleRebootRequired(raw->rebootRequired);

    if (!raw->category.isEmpty()) {
        meta->setCategory(raw->category);
    }
    if (!raw->shortDescription.isEmpty()) {
        meta->setShortDescription(raw->shortDescription);
    }
    if (!raw->longDescription.isEmpty()) {
        meta->setLongDescription(raw->longDescription);
    }
    if (!raw->units.isEmpty()) {
        meta->setRawUnits(raw->units);
    }

    const auto tryConvert = [&](const QString &txt, auto setter) -> void {
        if (txt.isEmpty()) {
            return;
        }

        QVariant v;
        QString err;
        if (meta->convertAndValidateRaw(txt, false, v, err)) {
            (meta->*setter)(v);
        } else {
            qCWarning(APMParameterMetaDataLog) << "Invalid numeric value for" << meta->name() << ":" << txt;
        }
    };

    tryConvert(raw->min, &FactMetaData::setRawMin);
    tryConvert(raw->max, &FactMetaData::setRawMax);

    if (!raw->values.isEmpty()) {
        QStringList strs;
        QVariantList vals;
        for (const APMFactMetaDataRaw::Value &value : raw->values) {
            QVariant v;
            QString err;
            if (meta->convertAndValidateRaw(value.first, false, v, err)) {
                vals << v;
                strs << value.second;
            }
        }

        if (!strs.isEmpty()) {
            meta->setEnumInfo(strs, vals);
        }
    }

    if (!raw->bitmask.isEmpty()) {
        QStringList strs;
        QVariantList vals;

        const auto toTyped = [&](unsigned idx) -> QVariant {
            const quint64 bit = 1ull << idx;
            switch (type) {
            case FactMetaData::valueTypeInt8:
                return QVariant(static_cast<qint8>(bit));
            case FactMetaData::valueTypeInt16:
                return QVariant(static_cast<qint16>(bit));
            case FactMetaData::valueTypeInt32:
            case FactMetaData::valueTypeInt64:
                return QVariant(static_cast<qint32>(bit));
            default:
                return QVariant(bit);
            }
        };

        for (const APMFactMetaDataRaw::Bit &bit : raw->bitmask) {
            bool ok = false;
            const unsigned idx = bit.first.toUInt(&ok);
            if (!ok) {
                continue;
            }
            vals << toTyped(idx);
            strs << bit.second;
        }
        if (!strs.isEmpty()) {
            meta->setBitmaskInfo(strs, vals);
        }
    }

    bool ok = false;
    const double inc = raw->incrementSize.toDouble(&ok);
    if (ok) {
        meta->setRawIncrement(inc);
    }

    if ((paramName.endsWith(u"_P") || paramName.endsWith(u"_I") || paramName.endsWith(u"_D")) && ((type == FactMetaData::valueTypeFloat) || (type == FactMetaData::valueTypeDouble))) {
        meta->setDecimalPlaces(6);
    }

    return meta;
}

QVersionNumber APMParameterMetaData::versionFromFilename(QStringView path)
{
    static const QRegularExpression rx(uR"(.*\.(\d+)\.(\d+)\.xml$)"_s);

    const QRegularExpressionMatch match = rx.matchView(path);
    if (!match.hasMatch()) {
        qCWarning(APMParameterMetaDataLog) << "Unable to parse version from parameter meta data file name:" << path;
        return QVersionNumber(-1, -1);
    }

    return QVersionNumber(match.capturedView(1).toInt(), match.capturedView(2).toInt());
}
