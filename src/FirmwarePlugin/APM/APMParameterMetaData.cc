/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "APMParameterMetaData.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QGC_LOGGING_CATEGORY(APMParameterMetaDataLog, "APMParameterMetaDataLog")

bool                                          APMParameterMetaData::_parameterMetaDataLoaded = false;
QMap<QString, ParameterNametoFactMetaDataMap> APMParameterMetaData::_vehicleTypeToParametersMap;

APMParameterMetaData::APMParameterMetaData(QObject* parent) :
    QObject(parent)
{
    _loadParameterFactMetaData();
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
            convertTo = QVariant::UInt;
            break;
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            convertTo = QVariant::Int;
            break;
        case FactMetaData::valueTypeFloat:
            convertTo = QMetaType::Float;
            break;
        case FactMetaData::valueTypeDouble:
            convertTo = QVariant::Double;
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
            vehicleName = "ArduPlane";
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_SUBMARINE:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            vehicleName = "ArduCopter";
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            vehicleName = "Antenna Tracker";
        case MAV_TYPE_GENERIC:
        case MAV_TYPE_GCS:
        case MAV_TYPE_AIRSHIP:
        case MAV_TYPE_FREE_BALLOON:
        case MAV_TYPE_ROCKET:
            break;
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
            vehicleName = "ArduRover";
            break;
        case MAV_TYPE_FLAPPING_WING:
        case MAV_TYPE_KITE:
        case MAV_TYPE_ONBOARD_CONTROLLER:
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
        case MAV_TYPE_VTOL_RESERVED5:
        case MAV_TYPE_GIMBAL:
        case MAV_TYPE_ENUM_END:
        default:
            break;
    }
    return vehicleName;
}

/// Load Parameter Fact meta data
///
/// The meta data comes from firmware parameters.xml file.
void APMParameterMetaData::_loadParameterFactMetaData()
{
    if (_parameterMetaDataLoaded) {
        return;
    }
    _parameterMetaDataLoaded = true;

    QRegExp parameterCategories = QRegExp("ArduCopter|ArduPlane|APMrover2|AntennaTracker");
    QString currentCategory;

    QString parameterFilename;

    // Fixme:: always picking up the bundled xml, we would like to update it from web
    // just not sure right now as the xml is in bad shape.
    if (parameterFilename.isEmpty() || !QFile(parameterFilename).exists()) {
        parameterFilename = ":/FirmwarePlugin/APM/apm.pdef.xml";
    }

    qCDebug(APMParameterMetaDataLog) << "Loading parameter meta data:" << parameterFilename;

    QFile xmlFile(parameterFilename);
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

    QString             errorString;
    bool                badMetaData = true;
    QStack<int>         xmlState;
    APMFactMetaDataRaw* rawMetaData = NULL;

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
                        qCDebug(APMParameterMetaDataLog) << "not interested in this block of parameters skip";
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
                QString group = name.split('_').first();
                group = group.remove(QRegExp("[0-9]*$")); // remove any numbers from the end

                QString shortDescription = xml.attributes().value("humanName").toString();
                QString longDescription = xml.attributes().value("docmentation").toString();
                QString userLevel = xml.attributes().value("user").toString();

                qCDebug(APMParameterMetaDataLog) << "Found parameter name:" << name
                          << "short Desc:" << shortDescription
                          << "longDescription:" << longDescription
                          << "user level: " << userLevel
                          << "group: " << group;

                Q_ASSERT(!rawMetaData);
                rawMetaData = new APMFactMetaDataRaw();
                if (_vehicleTypeToParametersMap[currentCategory].contains(name)) {
                    // We can't trust the meta dafa since we have dups
                    qCWarning(APMParameterMetaDataLog) << "Duplicate parameter found:" << name;
                    badMetaData = true;
                } else {
                    qCDebug(APMParameterMetaDataLog) << "inserting metadata for field" << name;
                    _vehicleTypeToParametersMap[currentCategory][name] = rawMetaData;
                    rawMetaData->name = name;
                    rawMetaData->group = group;
                    rawMetaData->shortDescription = shortDescription;
                    rawMetaData->longDescription = longDescription;

                    groupMembers[group] << name;
                }

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
                qCDebug(APMParameterMetaDataLog) << "done loading parameter";
                rawMetaData = NULL;
                badMetaData = false;
                xmlState.pop();
            } else if (elementName == "parameters") {
                qCDebug(APMParameterMetaDataLog) << "end of parameters for category: " << currentCategory;
                correctGroupMemberships(_vehicleTypeToParametersMap[currentCategory], groupMembers);
                groupMembers.clear();
                xmlState.pop();
            } else if (elementName == "vehicles") {
                qCDebug(APMParameterMetaDataLog) << "vehicles end here, libraries will follow";
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
                    parameterToFactMetaDataMap[parameter]->group = "others";
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
            // skip empty elements. Somehow I am getting lot of these. Dont know what to do with them.
        } else if (elementName == "field") {
            QString attributeName = xml.attributes().value("name").toString();
            if ( attributeName == "Range") {
                QString range = xml.readElementText().trimmed();
                QStringList rangeList = range.split(' ');
                if (rangeList.count() != 2) {
                    qCDebug(APMParameterMetaDataLog) << "space seperator didn't work',trying 'to' separator";
                    rangeList = range.split("to");
                    if (rangeList.count() != 2) {
                        qCDebug(APMParameterMetaDataLog) << " 'to' seperaator didn't work', trying '-' as seperator";
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
                    qCDebug(APMParameterMetaDataLog) << "read field parameter " << "min: " << rawMetaData->min
                                                     << "max: " << rawMetaData->max;
                }
            } else if (attributeName == "Increment") {
                QString increment = xml.readElementText();
                qCDebug(APMParameterMetaDataLog) << "read Increment: " << increment;
                rawMetaData->incrementSize = increment;
            } else if (attributeName == "Units") {
                QString units = xml.readElementText();
                qCDebug(APMParameterMetaDataLog) << "read Units: " << units;
                rawMetaData->units = units;
            }
        } else if (elementName == "values") {
            // doing nothing individual value will follow anyway. May be used for sanity checking.
        } else if (elementName == "value") {
            QString valueValue = xml.attributes().value("code").toString();
            QString valueName = xml.readElementText();
            qCDebug(APMParameterMetaDataLog) << "read value parameter " << "value desc: "
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

/// Override from FactLoad which connects the meta data to the fact
void APMParameterMetaData::addMetaDataToFact(Fact* fact, MAV_TYPE vehicleType)
{
    _loadParameterFactMetaData();

    const QString mavTypeString = mavTypeToString(vehicleType);
    APMFactMetaDataRaw* rawMetaData = NULL;

    // check if we have metadata for fact, use generic otherwise
    if (_vehicleTypeToParametersMap[mavTypeString].contains(fact->name())) {
        rawMetaData = _vehicleTypeToParametersMap[mavTypeString][fact->name()];
    } else if (_vehicleTypeToParametersMap["libraries"].contains(fact->name())) {
        rawMetaData = _vehicleTypeToParametersMap["libraries"][fact->name()];
    }

    FactMetaData *metaData = new FactMetaData(fact->type(), fact);

    // we don't have data for this fact
    if (!rawMetaData) {
        fact->setMetaData(metaData);
        qCDebug(APMParameterMetaDataLog) << "No metaData for " << fact->name() << "using generic metadata";
        return;
    }

    metaData->setName(rawMetaData->name);
    metaData->setGroup(rawMetaData->group);

    if (!rawMetaData->shortDescription.isEmpty()) {
        metaData->setShortDescription(rawMetaData->shortDescription);
    }

    if (!rawMetaData->longDescription.isEmpty()) {
        metaData->setLongDescription(rawMetaData->longDescription);
    }

    if (!rawMetaData->units.isEmpty()) {
        metaData->setUnits(rawMetaData->units);
    }

    if (!rawMetaData->min.isEmpty()) {
        QVariant varMin;
        QString errorString;
        if (metaData->convertAndValidate(rawMetaData->min, true /* convertOnly */, varMin, errorString)) {
            metaData->setMin(varMin);
        } else {
            qCDebug(APMParameterMetaDataLog) << "Invalid min value, name:" << metaData->name()
                                             << " type:" << metaData->type() << " min:" << rawMetaData->min
                                             << " error:" << errorString;
        }
    }

    if (!rawMetaData->max.isEmpty()) {
        QVariant varMax;
        QString errorString;
        if (metaData->convertAndValidate(rawMetaData->max, true /* convertOnly */, varMax, errorString)) {
            metaData->setMax(varMax);
        } else {
            qCDebug(APMParameterMetaDataLog) << "Invalid max value, name:" << metaData->name() << " type:"
                                             << metaData->type() << " max:" << rawMetaData->max
                                             << " error:" << errorString;
        }
    }

    // FixMe:: not handling values and increment size as their is no place for them in FactMetaData and no ui
    fact->setMetaData(metaData);
}
