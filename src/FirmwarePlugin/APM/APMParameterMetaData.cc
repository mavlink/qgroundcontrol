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
#include <QtCore/QStack>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>

QGC_LOGGING_CATEGORY(APMParameterMetaDataLog, "qgc.firmwareplugin.apm.apmparametermetadata")
QGC_LOGGING_CATEGORY(APMParameterMetaDataVerboseLog, "qgc.firmwareplugin.apm.apmparametermetadata:verbose")

APMParameterMetaData::APMParameterMetaData(QObject *parent)
    : QObject(parent)
{
    // qCDebug(APMParameterMetaDataLog) << Q_FUNC_INFO << this;
}

APMParameterMetaData::~APMParameterMetaData()
{
    // qCDebug(APMParameterMetaDataLog) << Q_FUNC_INFO << this;
}

QVariant APMParameterMetaData::_stringToTypedVariant(const QString &string, FactMetaData::ValueType_t type, bool *convertOk)
{
    QVariant var(string);

    QMetaType::Type convertTo = QMetaType::Int;
    switch (type) {
    case FactMetaData::valueTypeUint8:
    case FactMetaData::valueTypeUint16:
    case FactMetaData::valueTypeUint32:
    case FactMetaData::valueTypeUint64:
        convertTo = QMetaType::UInt;
        break;
    case FactMetaData::valueTypeInt8:
    case FactMetaData::valueTypeInt16:
    case FactMetaData::valueTypeInt32:
    case FactMetaData::valueTypeInt64:
        convertTo = QMetaType::Int;
        break;
    case FactMetaData::valueTypeFloat:
        convertTo = QMetaType::Float;
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    case FactMetaData::valueTypeDouble:
        convertTo = QMetaType::Double;
        break;
    case FactMetaData::valueTypeString:
        qWarning() << kInvalidConverstion;
        convertTo = QMetaType::QString;
        break;
    case FactMetaData::valueTypeBool:
        qWarning() << kInvalidConverstion;
        convertTo = QMetaType::Bool;
        break;
    case FactMetaData::valueTypeCustom:
        qWarning() << kInvalidConverstion;
        convertTo = QMetaType::QByteArray;
        break;
    }

    *convertOk = var.convert(QMetaType(convertTo));

    return var;
}

QString APMParameterMetaData::_mavTypeToString(MAV_TYPE vehicleTypeEnum)
{
    QString vehicleName;

    switch(vehicleTypeEnum) {
        case MAV_TYPE_FIXED_WING:
        case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
        case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_FIXEDROTOR:
        case MAV_TYPE_VTOL_TAILSITTER:
        case MAV_TYPE_VTOL_TILTWING:
        case MAV_TYPE_VTOL_RESERVED5:
            vehicleName = "ArduPlane";
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            vehicleName = "ArduCopter";
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            vehicleName = "Antenna Tracker";
            break;
        case MAV_TYPE_GENERIC:
        case MAV_TYPE_GCS:
        case MAV_TYPE_AIRSHIP:
        case MAV_TYPE_FREE_BALLOON:
        case MAV_TYPE_ROCKET:
            break;
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
            vehicleName = "Rover";
            break;
        case MAV_TYPE_SUBMARINE:
            vehicleName = "ArduSub";
            break;
        case MAV_TYPE_FLAPPING_WING:
        case MAV_TYPE_KITE:
        case MAV_TYPE_ONBOARD_CONTROLLER:
        case MAV_TYPE_GIMBAL:
        case MAV_TYPE_ENUM_END:
        default:
            break;
    }
    return vehicleName;
}

QString APMParameterMetaData::_groupFromParameterName(const QString &name)
{
    static const QRegularExpression regex = QRegularExpression("[0-9]*$");
    QString group = name.split('_').first();
    return group.remove(regex); // remove any numbers from the end
}

void APMParameterMetaData::loadParameterFactMetaDataFile(const QString &metaDataFile)
{
    if (_parameterMetaDataLoaded) {
        return;
    }
    _parameterMetaDataLoaded = true;

    QString currentCategory;

    qCDebug(APMParameterMetaDataLog) << "Loading parameter meta data:" << metaDataFile;

    QFile xmlFile(metaDataFile);
    Q_ASSERT(xmlFile.exists());

    bool success = xmlFile.open(QIODevice::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qCWarning(APMParameterMetaDataLog) << "Badly formed XML, reading failed: " << xml.errorString();
        return;
    }

    bool badMetaData = true;
    QStack<int> xmlState;
    APMFactMetaDataRaw *rawMetaData = nullptr;

    xmlState.push(XmlStateNone);

    QMap<QString,QStringList> groupMembers; //used to remove groups with single item

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            const QString elementName = xml.name().toString();

            if (elementName.isEmpty()) {
                // skip empty elements
            } else if (elementName == "paramfile") {
                if (xmlState.top() != XmlStateNone) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, paramfile matched";
                }
                xmlState.push(XmlstateParamFileFound);
                // we don't really do anything with this element
            } else if (elementName == "vehicles") {
                if (xmlState.top() != XmlstateParamFileFound) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, vehicles matched";
                    return;
                }
                xmlState.push(XmlStateFoundVehicles);
            } else if (elementName == "libraries") {
                if (xmlState.top() != XmlstateParamFileFound) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, libraries matched";
                    return;
                }
                currentCategory = "libraries";
                xmlState.push(XmlStateFoundLibraries);
            } else if (elementName == "parameters") {
                if (xmlState.top() != XmlStateFoundVehicles && xmlState.top() != XmlStateFoundLibraries) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, parameters matched"
                                                       << "but we don't have proper vehicle or libraries yet";
                    return;
                }

                if (xml.attributes().hasAttribute("name")) {
                    // we will handle metadata only for specific MAV_TYPEs and libraries
                    const QString nameValue = xml.attributes().value("name").toString();
                    static const QRegularExpression parameterCategories = QRegularExpression("ArduCopter|ArduPlane|APMrover2|Rover|ArduSub|AntennaTracker");
                    if (nameValue.contains(parameterCategories)) {
                        xmlState.push(XmlStateFoundParameters);
                        currentCategory = nameValue;
                    } else if(xmlState.top() == XmlStateFoundLibraries) {
                        // we handle all libraries section under the same category libraries
                        // so not setting currentCategory
                        xmlState.push(XmlStateFoundParameters);
                    } else {
                        qCDebug(APMParameterMetaDataVerboseLog) << "not interested in this block of parameters, skipping:" << nameValue;
                        if (_skipXMLBlock(xml, "parameters")) {
                            qCWarning(APMParameterMetaDataLog) << "something wrong with the xml, skip of the xml failed";
                            return;
                        }
                        (void) xml.readNext();
                        continue;
                    }
                }
            } else if (elementName == "param") {
                if (xmlState.top() != XmlStateFoundParameters) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, element param matched"
                                                       << "while we are not yet in parameters";
                    return;
                }
                xmlState.push(XmlStateFoundParameter);

                if (!xml.attributes().hasAttribute("name")) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, parameter attribute name missing";
                    return;
                }

                QString name = xml.attributes().value("name").toString();
                if (name.contains(':')) {
                    name = name.split(':').last();
                }
                const QString group = _groupFromParameterName(name);

                const QString category = xml.attributes().value("user").toString();

                const QString shortDescription = xml.attributes().value("humanName").toString();
                const QString longDescription = xml.attributes().value("documentation").toString();

                qCDebug(APMParameterMetaDataVerboseLog) << "Found parameter name:" << name
                          << "short Desc:" << shortDescription
                          << "longDescription:" << longDescription
                          << "category: " << category
                          << "group: " << group;

                Q_ASSERT(!rawMetaData);
                if (_vehicleTypeToParametersMap[currentCategory].contains(name)) {
                    qCDebug(APMParameterMetaDataLog) << "Duplicate parameter found:" << name;
                    rawMetaData = _vehicleTypeToParametersMap[currentCategory][name];
                } else {
                    rawMetaData = new APMFactMetaDataRaw(this);
                    _vehicleTypeToParametersMap[currentCategory][name] = rawMetaData;
                    groupMembers[group] << name;
                }
                qCDebug(APMParameterMetaDataVerboseLog) << "inserting metadata for field" << name;
                rawMetaData->name = name;
                if (!category.isEmpty()) {
                    rawMetaData->category = category;
                }
                rawMetaData->group = group;
                rawMetaData->shortDescription = shortDescription;
                rawMetaData->longDescription = longDescription;
            } else {
                // We should be getting meta data now
                if (xmlState.top() != XmlStateFoundParameter) {
                    qCWarning(APMParameterMetaDataLog) << "Badly formed XML, while reading parameter fields wrong state";
                    return;
                }
                if (!badMetaData) {
                    if (!_parseParameterAttributes(xml, rawMetaData)) {
                        qCDebug(APMParameterMetaDataLog) << "Badly formed XML, failed to read parameter attributes";
                        return;
                    }
                    continue;
                }
            }
        } else if (xml.isEndElement()) {
            const QString elementName = xml.name().toString();

            if ((elementName == "param") && (xmlState.top() == XmlStateFoundParameter)) {
                // Done loading this parameter
                // Reset for next parameter
                qCDebug(APMParameterMetaDataVerboseLog) << "done loading parameter";
                rawMetaData = nullptr;
                badMetaData = false;
                xmlState.pop();
            } else if (elementName == "parameters") {
                qCDebug(APMParameterMetaDataVerboseLog) << "end of parameters for category: " << currentCategory;
                _correctGroupMemberships(_vehicleTypeToParametersMap[currentCategory], groupMembers);
                groupMembers.clear();
                xmlState.pop();
            } else if (elementName == "vehicles") {
                qCDebug(APMParameterMetaDataVerboseLog) << "vehicles end here, libraries will follow";
                xmlState.pop();
            }
        }
        (void) xml.readNext();
    }
}

void APMParameterMetaData::_correctGroupMemberships(ParameterNametoFactMetaDataMap &parameterToFactMetaDataMap, QMap<QString,QStringList> &groupMembers)
{
    for (const QString &groupName : groupMembers.keys()) {
        if (groupMembers[groupName].count() == 1) {
            for (const QString &parameter : groupMembers.value(groupName)) {
                parameterToFactMetaDataMap[parameter]->group = FactMetaData::defaultGroup();
            }
        }
    }
}

bool APMParameterMetaData::_skipXMLBlock(QXmlStreamReader &xml, const QString &blockName)
{
    QString elementName;
    do {
        (void) xml.readNext();
        elementName = xml.name().toString();
    } while ((elementName != blockName) && (xml.isEndElement()));
    return !xml.isEndDocument();
}

bool APMParameterMetaData::_parseParameterAttributes(QXmlStreamReader &xml, APMFactMetaDataRaw *rawMetaData)
{
    QString elementName = xml.name().toString();
    QList<QPair<QString,QString>> values;
    // as long as param doens't end
    while (!((elementName == "param") && xml.isEndElement())) {
        if (elementName.isEmpty()) {
            // skip empty elements. Somehow I am getting lot of these. Don't know what to do with them.
        } else if (elementName == "field") {
            const QString attributeName = xml.attributes().value("name").toString();

            if (attributeName == "Range") {
                const QString range = xml.readElementText().trimmed();
                QStringList rangeList = range.split(' ');
                if (rangeList.count() != 2) {
                    qCDebug(APMParameterMetaDataVerboseLog) << "space seperator didn't work',trying 'to' separator";
                    rangeList = range.split("to");
                    if (rangeList.count() != 2) {
                        qCDebug(APMParameterMetaDataVerboseLog) << " 'to' seperaator didn't work', trying '-' as seperator";
                        rangeList = range.split('-');
                        if (rangeList.count() != 2) {
                            qCDebug(APMParameterMetaDataLog) << "something wrong with range, all three separators have failed" << range;
                        }
                    }
                }

                // everything should be good. lets collect min and max
                if (rangeList.count() == 2) {
                    rawMetaData->min = rangeList.first().trimmed();
                    rawMetaData->max = rangeList.last().trimmed();

                    // sanitize min and max off any comments that they may have
                    if (rawMetaData->min.contains(' ')) {
                        rawMetaData->min = rawMetaData->min.split(' ').first();
                    }
                    if(rawMetaData->max.contains(' ')) {
                        rawMetaData->max = rawMetaData->max.split(' ').first();
                    }
                    qCDebug(APMParameterMetaDataVerboseLog) << "read field parameter" << "min:" << rawMetaData->min
                                                            << "max:" << rawMetaData->max;
                }
            } else if (attributeName == "Increment") {
                const QString increment = xml.readElementText();
                qCDebug(APMParameterMetaDataVerboseLog) << "read Increment: " << increment;
                rawMetaData->incrementSize = increment;
            } else if (attributeName == "Units") {
                const QString units = xml.readElementText();
                qCDebug(APMParameterMetaDataVerboseLog) << "read Units: " << units;
                rawMetaData->units = units;
            } else if (attributeName == "ReadOnly") {
                const QString strValue = xml.readElementText().trimmed();
                if (strValue.compare("true", Qt::CaseInsensitive) == 0) {
                    rawMetaData->readOnly = true;
                }
                qCDebug(APMParameterMetaDataVerboseLog) << "read ReadOnly: " << rawMetaData->readOnly;
            } else if (attributeName == "Bitmask") {
                bool parseError = false;

                const QString bitmaskString = xml.readElementText();
                qCDebug(APMParameterMetaDataVerboseLog) << "read Bitmask: " << bitmaskString;
                const QStringList bitmaskList = bitmaskString.split(",");
                if (!bitmaskList.isEmpty()) {
                    for (const QString &bitmask : bitmaskList) {
                        const QStringList pair = bitmask.split(":");
                        if (pair.count() == 2) {
                            rawMetaData->bitmask << QPair<QString, QString>(pair[0], pair[1]);
                        } else {
                            qCDebug(APMParameterMetaDataLog) << "parse error: bitmask:" << bitmaskString << "pair count:" << pair.count();
                            parseError = true;
                            break;
                        }
                    }
                }

                if (parseError) {
                    rawMetaData->bitmask.clear();
                }
            } else if (attributeName == "RebootRequired") {
                const QString strValue = xml.readElementText().trimmed();
                if (strValue.compare("true", Qt::CaseInsensitive) == 0) {
                    rawMetaData->rebootRequired = true;
                }
            }
        } else if (elementName == "values") {
            // doing nothing individual value will follow anyway. May be used for sanity checking.
        } else if (elementName == "value") {
            const QString valueValue = xml.attributes().value("code").toString();
            const QString valueName = xml.readElementText();
            qCDebug(APMParameterMetaDataVerboseLog) << "read value parameter" << "value desc:"
                                                    << valueName << "code:" << valueValue;
            values << QPair<QString,QString>(valueValue, valueName);
            rawMetaData->values = values;
        } else {
            qCWarning(APMParameterMetaDataLog) << "Unknown parameter element in XML: " << elementName;
        }
        (void) xml.readNext();
        elementName = xml.name().toString();
    }
    return true;
}

FactMetaData *APMParameterMetaData::getMetaDataForFact(const QString &name, MAV_TYPE vehicleType, FactMetaData::ValueType_t type)
{
    bool keepTrying = true;
    QString mavTypeString = _mavTypeToString(vehicleType);
    APMFactMetaDataRaw *rawMetaData = nullptr;

    // check if we have metadata for fact, use generic otherwise
    while (keepTrying) {
        if (_vehicleTypeToParametersMap[mavTypeString].contains(name)) {
            rawMetaData = _vehicleTypeToParametersMap[mavTypeString][name];
        } else if (_vehicleTypeToParametersMap["libraries"].contains(name)) {
            rawMetaData = _vehicleTypeToParametersMap["libraries"][name];
        }
        if (!rawMetaData && (mavTypeString == "Rover")) {
            // Hack city: Older versions of Rover have different name
            mavTypeString = "APMrover2";
        } else {
            keepTrying = false;
        }
    }

    FactMetaData *const metaData = new FactMetaData(type, this);

    // we don't have data for this fact
    if (!rawMetaData) {
        metaData->setCategory(QStringLiteral("Advanced"));
        metaData->setGroup(_groupFromParameterName(name));
        qCDebug(APMParameterMetaDataLog) << "No metaData for " << name << "using generic metadata";
        return metaData;
    }

    metaData->setName(rawMetaData->name);
    if (!rawMetaData->category.isEmpty()) {
        metaData->setCategory(rawMetaData->category);
    }
    metaData->setGroup(rawMetaData->group);
    metaData->setVehicleRebootRequired(rawMetaData->rebootRequired);
    metaData->setReadOnly(rawMetaData->readOnly);

    if (!rawMetaData->shortDescription.isEmpty()) {
        metaData->setShortDescription(rawMetaData->shortDescription);
    }

    if (!rawMetaData->longDescription.isEmpty()) {
        metaData->setLongDescription(rawMetaData->longDescription);
    }

    if (!rawMetaData->units.isEmpty()) {
        metaData->setRawUnits(rawMetaData->units);
    }

    if (!rawMetaData->min.isEmpty()) {
        QVariant varMin;
        QString errorString;
        if (metaData->convertAndValidateRaw(rawMetaData->min, false /* validate as well */, varMin, errorString)) {
            metaData->setRawMin(varMin);
        } else {
            qCDebug(APMParameterMetaDataLog) << "Invalid min value, name:" << metaData->name()
                                             << " type:" << metaData->type() << " min:" << rawMetaData->min
                                             << " error:" << errorString;
        }
    }

    if (!rawMetaData->max.isEmpty()) {
        QVariant varMax;
        QString errorString;
        if (metaData->convertAndValidateRaw(rawMetaData->max, false /* validate as well */, varMax, errorString)) {
            metaData->setRawMax(varMax);
        } else {
            qCDebug(APMParameterMetaDataLog) << "Invalid max value, name:" << metaData->name() << " type:"
                                             << metaData->type() << " max:" << rawMetaData->max
                                             << " error:" << errorString;
        }
    }

    if (!rawMetaData->values.isEmpty()) {
        QStringList enumStrings;
        QVariantList enumValues;

        for (int i = 0; i < rawMetaData->values.count(); i++) {
            QVariant enumValue;
            QString errorString;
            const QPair<QString, QString> enumPair = rawMetaData->values[i];

            if (metaData->convertAndValidateRaw(enumPair.first, false /* validate */, enumValue, errorString)) {
                enumValues << enumValue;
                enumStrings << enumPair.second;
            } else {
                qCDebug(APMParameterMetaDataLog) << "Invalid enum value, name:" << metaData->name()
                                                 << " type:" << metaData->type() << " value:" << enumPair.first
                                                 << " error:" << errorString;
                enumStrings.clear();
                enumValues.clear();
                break;
            }
        }

        if (!enumStrings.isEmpty()) {
            metaData->setEnumInfo(enumStrings, enumValues);
        }
    }

    if (!rawMetaData->bitmask.isEmpty()) {
        QStringList bitmaskStrings;
        QVariantList bitmaskValues;

        for (int i = 0; i < rawMetaData->bitmask.count(); i++) {
            QVariant bitmaskValue;
            QString errorString;
            const QPair<QString, QString> bitmaskPair = rawMetaData->bitmask[i];

            bool ok = false;
            unsigned int bitSet = bitmaskPair.first.toUInt(&ok);
            bitSet <<= 1;

            QVariant typedBitSet;

            switch (type) {
            case FactMetaData::valueTypeInt8:
                typedBitSet = QVariant(static_cast<signed char>(bitSet));
                break;
            case FactMetaData::valueTypeInt16:
                typedBitSet = QVariant(static_cast<short int>(bitSet));
                break;
            case FactMetaData::valueTypeInt32:
            case FactMetaData::valueTypeInt64:
                typedBitSet = QVariant(static_cast<int>(bitSet));
                break;
            case FactMetaData::valueTypeUint8:
            case FactMetaData::valueTypeUint16:
            case FactMetaData::valueTypeUint32:
            case FactMetaData::valueTypeUint64:
                typedBitSet = QVariant(bitSet);
                break;
            default:
                break;
            }

            if (typedBitSet.isNull()) {
                qCDebug(APMParameterMetaDataLog) << "Invalid type for bitmask, name:" << metaData->name()
                                                 << " type:" << metaData->type();
            }

            if (!ok) {
                qCDebug(APMParameterMetaDataLog) << "Invalid bitmask value, name:" << metaData->name()
                                                 << " type:" << metaData->type() << " value:" << bitSet
                                                 << " error: toUInt failed";
                bitmaskStrings.clear();
                bitmaskValues.clear();
                break;
            }

            if (metaData->convertAndValidateRaw(typedBitSet, false /* validate */, bitmaskValue, errorString)) {
                bitmaskValues << bitmaskValue;
                bitmaskStrings << bitmaskPair.second;
            } else {
                qCDebug(APMParameterMetaDataLog) << "Invalid bitmask value, name:" << metaData->name()
                                                 << " type:" << metaData->type() << " value:" << typedBitSet
                                                 << " error:" << errorString;
                bitmaskStrings.clear();
                bitmaskValues.clear();
                break;
            }
        }

        if (!bitmaskStrings.isEmpty()) {
            metaData->setBitmaskInfo(bitmaskStrings, bitmaskValues);
        }
    }

    if (!rawMetaData->incrementSize.isEmpty()) {
        bool ok;
        const double increment = rawMetaData->incrementSize.toDouble(&ok);
        if (ok) {
            metaData->setRawIncrement(increment);
        } else {
            qCDebug(APMParameterMetaDataLog) << "Invalid value for increment, name:" << metaData->name() << " increment:" << rawMetaData->incrementSize;
        }
    }

    // ArduPilot does not yet support decimal places meta data. So for P/I/D parameters we force to 6 places
    if ((name.endsWith(QStringLiteral("_P")) || name.endsWith(QStringLiteral("_I")) || name.endsWith(QStringLiteral("_D"))) &&
        (type == FactMetaData::valueTypeFloat || type == FactMetaData::valueTypeDouble)) {
        metaData->setDecimalPlaces(6);
    }

    return metaData;
}

void APMParameterMetaData::getParameterMetaDataVersionInfo(const QString &metaDataFile, int &majorVersion, int &minorVersion)
{
    static const QRegularExpression regex(".*\\.(\\d)\\.(\\d)\\.xml$");
    const QRegularExpressionMatch match = regex.match(metaDataFile);
    if (match.hasMatch() && (match.lastCapturedIndex() == 2)) {
        majorVersion = match.captured(1).toInt();
        minorVersion = match.captured(2).toInt();
    } else {
        majorVersion = -1;
        minorVersion = -1;
        qCWarning(APMParameterMetaDataLog) << "Unable to parse version from parameter meta data file name:" << metaDataFile;
    }
}
