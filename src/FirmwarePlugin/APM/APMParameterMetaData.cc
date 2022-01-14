/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMParameterMetaData.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QStack>

static const char* kInvalidConverstion = "Internal Error: No support for string parameters";

QGC_LOGGING_CATEGORY(APMParameterMetaDataLog,           "APMParameterMetaDataLog")
QGC_LOGGING_CATEGORY(APMParameterMetaDataVerboseLog,    "APMParameterMetaDataVerboseLog")

APMParameterMetaData::APMParameterMetaData(void)
    : _parameterMetaDataLoaded(false)
{

}

/// Converts a string to a typed QVariant
///     @param string String to convert
///     @param type Type for Fact which dictates the QVariant type as well
///     @param convertOk Returned: true: conversion success, false: conversion failure
/// @return Returns the correctly type QVariant
QVariant APMParameterMetaData::_stringToTypedVariant(const QString& string,
                                                     FactMetaData::ValueType_t type, bool* convertOk)
{
    QVariant var(string);

    int convertTo = QVariant::Int; // keep compiler warning happy
    switch (type) {
    case FactMetaData::valueTypeUint8:
    case FactMetaData::valueTypeUint16:
    case FactMetaData::valueTypeUint32:
    case FactMetaData::valueTypeUint64:
        convertTo = QVariant::UInt;
        break;
    case FactMetaData::valueTypeInt8:
    case FactMetaData::valueTypeInt16:
    case FactMetaData::valueTypeInt32:
    case FactMetaData::valueTypeInt64:
        convertTo = QVariant::Int;
        break;
    case FactMetaData::valueTypeFloat:
        convertTo = QMetaType::Float;
        break;
    case FactMetaData::valueTypeElapsedTimeInSeconds:
    case FactMetaData::valueTypeDouble:
        convertTo = QVariant::Double;
        break;
    case FactMetaData::valueTypeString:
        qWarning() << kInvalidConverstion;
        convertTo = QVariant::String;
        break;
    case FactMetaData::valueTypeBool:
        qWarning() << kInvalidConverstion;
        convertTo = QVariant::Bool;
        break;
    case FactMetaData::valueTypeCustom:
        qWarning() << kInvalidConverstion;
        convertTo = QVariant::ByteArray;
        break;
    }

    *convertOk = var.convert(convertTo);

    return var;
}

QString APMParameterMetaData::mavTypeToString(MAV_TYPE vehicleTypeEnum)
{
    QString vehicleName;

    switch(vehicleTypeEnum) {
        case MAV_TYPE_FIXED_WING:
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
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

QString APMParameterMetaData::_groupFromParameterName(const QString& name)
{
    QString group = name.split('_').first();
    return group.remove(QRegExp("[0-9]*$")); // remove any numbers from the end
}


void APMParameterMetaData::loadParameterFactMetaDataFile(const QString& metaDataFile)
{
    if (_parameterMetaDataLoaded) {
        return;
    }
    _parameterMetaDataLoaded = true;

    QRegExp parameterCategories = QRegExp("ArduCopter|ArduPlane|APMrover2|Rover|ArduSub|AntennaTracker");
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

    bool                badMetaData = true;
    QStack<int>         xmlState;
    APMFactMetaDataRaw* rawMetaData = nullptr;

    xmlState.push(XmlStateNone);

    QMap<QString,QStringList> groupMembers; //used to remove groups with single item

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

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
                    if (nameValue.contains(parameterCategories)) {
                        xmlState.push(XmlStateFoundParameters);
                        currentCategory = nameValue;
                    } else if(xmlState.top() == XmlStateFoundLibraries) {
                        // we handle all libraries section under the same category libraries
                        // so not setting currentCategory
                        xmlState.push(XmlStateFoundParameters);
                    } else {
                        qCDebug(APMParameterMetaDataVerboseLog) << "not interested in this block of parameters, skipping:" << nameValue;
                        if (skipXMLBlock(xml, "parameters")) {
                            qCWarning(APMParameterMetaDataLog) << "something wrong with the xml, skip of the xml failed";
                            return;
                        }
                        xml.readNext();
                        continue;
                    }
                }
            }  else if (elementName == "param") {
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
                QString group = _groupFromParameterName(name);

                QString category = xml.attributes().value("user").toString();

                QString shortDescription = xml.attributes().value("humanName").toString();
                QString longDescription = xml.attributes().value("documentation").toString();

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
                    if (!parseParameterAttributes(xml, rawMetaData)) {
                        qCDebug(APMParameterMetaDataLog) << "Badly formed XML, failed to read parameter attributes";
                        return;
                    }
                    continue;
                }
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "param" && xmlState.top() == XmlStateFoundParameter) {
                // Done loading this parameter
                // Reset for next parameter
                qCDebug(APMParameterMetaDataVerboseLog) << "done loading parameter";
                rawMetaData = nullptr;
                badMetaData = false;
                xmlState.pop();
            } else if (elementName == "parameters") {
                qCDebug(APMParameterMetaDataVerboseLog) << "end of parameters for category: " << currentCategory;
                correctGroupMemberships(_vehicleTypeToParametersMap[currentCategory], groupMembers);
                groupMembers.clear();
                xmlState.pop();
            } else if (elementName == "vehicles") {
                qCDebug(APMParameterMetaDataVerboseLog) << "vehicles end here, libraries will follow";
                xmlState.pop();
            }
        }
        xml.readNext();
    }
}

void APMParameterMetaData::correctGroupMemberships(ParameterNametoFactMetaDataMap& parameterToFactMetaDataMap,
                                                   QMap<QString,QStringList>& groupMembers)
{
    foreach(const QString& groupName, groupMembers.keys()) {
            if (groupMembers[groupName].count() == 1) {
                foreach(const QString& parameter, groupMembers.value(groupName)) {
                    parameterToFactMetaDataMap[parameter]->group = FactMetaData::defaultGroup();
                }
            }
        }
}

bool APMParameterMetaData::skipXMLBlock(QXmlStreamReader& xml, const QString& blockName)
{
    QString elementName;
    do {
        xml.readNext();
        elementName = xml.name().toString();
    } while ((elementName != blockName) && (xml.isEndElement()));
    return !xml.isEndDocument();
}

bool APMParameterMetaData::parseParameterAttributes(QXmlStreamReader& xml, APMFactMetaDataRaw* rawMetaData)
{
    QString elementName = xml.name().toString();
    QList<QPair<QString,QString> > values;
    // as long as param doens't end
    while (!(elementName == "param" && xml.isEndElement())) {
        if (elementName.isEmpty()) {
            // skip empty elements. Somehow I am getting lot of these. Don't know what to do with them.
        } else if (elementName == "field") {
            QString attributeName = xml.attributes().value("name").toString();

            if ( attributeName == "Range") {
                QString range = xml.readElementText().trimmed();
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
                    qCDebug(APMParameterMetaDataVerboseLog) << "read field parameter " << "min: " << rawMetaData->min
                                                     << "max: " << rawMetaData->max;
                }
            } else if (attributeName == "Increment") {
                QString increment = xml.readElementText();
                qCDebug(APMParameterMetaDataVerboseLog) << "read Increment: " << increment;
                rawMetaData->incrementSize = increment;
            } else if (attributeName == "Units") {
                QString units = xml.readElementText();
                qCDebug(APMParameterMetaDataVerboseLog) << "read Units: " << units;
                rawMetaData->units = units;
            } else if (attributeName == "ReadOnly") {
                QString strValue = xml.readElementText().trimmed();
                if (strValue.compare("true", Qt::CaseInsensitive) == 0) {
                    rawMetaData->readOnly = true;
                }
                qCDebug(APMParameterMetaDataVerboseLog) << "read ReadOnly: " << rawMetaData->readOnly;
            } else if (attributeName == "Bitmask") {
                bool    parseError = false;

                QString bitmaskString = xml.readElementText();
                qCDebug(APMParameterMetaDataVerboseLog) << "read Bitmask: " << bitmaskString;
                QStringList bitmaskList = bitmaskString.split(",");
                if (bitmaskList.count() > 0) {
                    foreach (const QString& bitmask, bitmaskList) {
                        QStringList pair = bitmask.split(":");
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
                QString strValue = xml.readElementText().trimmed();
                if (strValue.compare("true", Qt::CaseInsensitive) == 0) {
                    rawMetaData->rebootRequired = true;
                }
            }
        } else if (elementName == "values") {
            // doing nothing individual value will follow anyway. May be used for sanity checking.
        } else if (elementName == "value") {
            QString valueValue = xml.attributes().value("code").toString();
            QString valueName = xml.readElementText();
            qCDebug(APMParameterMetaDataVerboseLog) << "read value parameter " << "value desc: "
                                             << valueName << "code: " << valueValue;
            values << QPair<QString,QString>(valueValue, valueName);
            rawMetaData->values = values;
        } else {
            qCWarning(APMParameterMetaDataLog) << "Unknown parameter element in XML: " << elementName;
        }
        xml.readNext();
        elementName = xml.name().toString();
    }
    return true;
}

FactMetaData* APMParameterMetaData::getMetaDataForFact(const QString& name, MAV_TYPE vehicleType, FactMetaData::ValueType_t type)
{
    bool                keepTrying      = true;
    QString             mavTypeString   = mavTypeToString(vehicleType);
    APMFactMetaDataRaw* rawMetaData     = nullptr;

    // check if we have metadata for fact, use generic otherwise
    while (keepTrying) {
        if (_vehicleTypeToParametersMap[mavTypeString].contains(name)) {
            rawMetaData = _vehicleTypeToParametersMap[mavTypeString][name];
        } else if (_vehicleTypeToParametersMap["libraries"].contains(name)) {
            rawMetaData = _vehicleTypeToParametersMap["libraries"][name];
        }
        if (!rawMetaData && mavTypeString == "Rover") {
            // Hack city: Older versions of Rover have different name
            mavTypeString = "APMrover2";
        } else {
            keepTrying = false;
        }
    }

    FactMetaData *metaData = new FactMetaData(type, this);

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

    if (rawMetaData->values.count() > 0) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        for (int i=0; i<rawMetaData->values.count(); i++) {
            QVariant    enumValue;
            QString     errorString;
            QPair<QString, QString> enumPair = rawMetaData->values[i];

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

        if (enumStrings.count() != 0) {
            metaData->setEnumInfo(enumStrings, enumValues);
        }
    }

    if (rawMetaData->bitmask.count() > 0) {
        QStringList     bitmaskStrings;
        QVariantList    bitmaskValues;

        for (int i=0; i<rawMetaData->bitmask.count(); i++) {
            QVariant    bitmaskValue;
            QString     errorString;
            QPair<QString, QString> bitmaskPair = rawMetaData->bitmask[i];

            bool ok = false;
            unsigned int bitSet = bitmaskPair.first.toUInt(&ok);
            bitSet = 1 << bitSet;

            QVariant typedBitSet;

            switch (type) {
            case FactMetaData::valueTypeInt8:
                typedBitSet = QVariant((signed char)bitSet);
                break;

            case FactMetaData::valueTypeInt16:
                typedBitSet = QVariant((short int)bitSet);
                break;

            case FactMetaData::valueTypeInt32:
            case FactMetaData::valueTypeInt64:
                typedBitSet = QVariant((int)bitSet);
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

        if (bitmaskStrings.count() != 0) {
            metaData->setBitmaskInfo(bitmaskStrings, bitmaskValues);
        }
    }

    if (!rawMetaData->incrementSize.isEmpty()) {
        double  increment;
        bool    ok;
        increment = rawMetaData->incrementSize.toDouble(&ok);
        if (ok) {
            metaData->setRawIncrement(increment);
        } else {
            qCDebug(APMParameterMetaDataLog) << "Invalid value for increment, name:" << metaData->name() << " increment:" << rawMetaData->incrementSize;
        }
    }

    // ArduPilot does not yet support decimal places meta data. So for P/I/D parameters we force to 6 places
    if ((name.endsWith(QStringLiteral("_P")) ||
         name.endsWith(QStringLiteral("_I")) ||
         name.endsWith(QStringLiteral("_D"))) &&
            (type == FactMetaData::valueTypeFloat ||
             type == FactMetaData::valueTypeDouble)) {
        metaData->setDecimalPlaces(6);
    }

    return metaData;
}

void APMParameterMetaData::getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    majorVersion = -1;
    minorVersion = -1;

    // Meta data version is hacked in for now based on file name
    QRegExp regExp(".*\\.(\\d)\\.(\\d)\\.xml$");
    if (regExp.exactMatch(metaDataFile) && regExp.captureCount() == 2) {
        majorVersion = regExp.cap(2).toInt();
        minorVersion = 0;
    } else {
        qWarning() << QStringLiteral("Unable to parse version from parameter meta data file name: '%1'").arg(metaDataFile);
    }
}
