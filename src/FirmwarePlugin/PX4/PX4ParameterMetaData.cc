/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "PX4ParameterMetaData.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QGC_LOGGING_CATEGORY(PX4ParameterMetaDataLog, "PX4ParameterMetaDataLog")

PX4ParameterMetaData::PX4ParameterMetaData(void)
    : _parameterMetaDataLoaded(false)
{

}

/// Converts a string to a typed QVariant
///     @param string String to convert
///     @param type Type for Fact which dictates the QVariant type as well
///     @param convertOk Returned: true: conversion success, false: conversion failure
/// @return Returns the correctly type QVariant
QVariant PX4ParameterMetaData::_stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool* convertOk)
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

void PX4ParameterMetaData::loadParameterFactMetaDataFile(const QString& metaDataFile)
{
    qCDebug(ParameterLoaderLog) << "PX4ParameterMetaData::loadParameterFactMetaDataFile" << metaDataFile;

    if (_parameterMetaDataLoaded) {
        qWarning() << "Internal error: parameter meta data loaded more than once";
        return;
    }
    _parameterMetaDataLoaded = true;
	
    qCDebug(PX4ParameterMetaDataLog) << "Loading parameter meta data:" << metaDataFile;

    QFile xmlFile(metaDataFile);

    if (!xmlFile.exists()) {
        qWarning() << "Internal error: metaDataFile mission" << metaDataFile;
        return;
    }
    
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Internal error: Unable to open parameter file:" << metaDataFile << xmlFile.errorString();
        return;
    }
    
    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qWarning() << "Badly formed XML" << xml.errorString();
        return;
    }
    
    QString         factGroup;
    QString         errorString;
    FactMetaData*   metaData = NULL;
    int             xmlState = XmlStateNone;
    bool            badMetaData = true;
    
    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();
            
            if (elementName == "parameters") {
                if (xmlState != XmlStateNone) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                xmlState = XmlStateFoundParameters;
                
            } else if (elementName == "version") {
                if (xmlState != XmlStateFoundParameters) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                xmlState = XmlStateFoundVersion;
                
                bool convertOk;
                QString strVersion = xml.readElementText();
                int intVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                if (intVersion <= 2) {
                    // We can't read these old files
                    qDebug() << "Parameter version stamp too old, skipping load. Found:" << intVersion << "Want: 3 File:" << metaDataFile;
                    return;
                }
                
            } else if (elementName == "parameter_version_major") {
                // Just skip over for now
            } else if (elementName == "parameter_version_minor") {
                // Just skip over for now

            } else if (elementName == "group") {
                if (xmlState != XmlStateFoundVersion) {
                    // We didn't get a version stamp, assume older version we can't read
                    qDebug() << "Parameter version stamp not found, skipping load" << metaDataFile;
                    return;
                }
                xmlState = XmlStateFoundGroup;
                
                if (!xml.attributes().hasAttribute("name")) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                factGroup = xml.attributes().value("name").toString();
                qCDebug(PX4ParameterMetaDataLog) << "Found group: " << factGroup;
                
            } else if (elementName == "parameter") {
                if (xmlState != XmlStateFoundGroup) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                xmlState = XmlStateFoundParameter;
                
                if (!xml.attributes().hasAttribute("name") || !xml.attributes().hasAttribute("type")) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                
                QString name = xml.attributes().value("name").toString();
                QString type = xml.attributes().value("type").toString();
                QString strDefault =    xml.attributes().value("default").toString();
                
                qCDebug(PX4ParameterMetaDataLog) << "Found parameter name:" << name << " type:" << type << " default:" << strDefault;

                // Convert type from string to FactMetaData::ValueType_t
                bool unknownType;
                FactMetaData::ValueType_t foundType = FactMetaData::stringToType(type, unknownType);
                if (unknownType) {
                    qWarning() << "Parameter meta data with bad type:" << type << " name:" << name;
                    return;
                }
                
                // Now that we know type we can create meta data object and add it to the system
                
                metaData = new FactMetaData(foundType);
                Q_CHECK_PTR(metaData);
                if (_mapParameterName2FactMetaData.contains(name)) {
                    // We can't trust the meta dafa since we have dups
                    qCWarning(PX4ParameterMetaDataLog) << "Duplicate parameter found:" << name;
                    badMetaData = true;
                    // Reset to default meta data
                    _mapParameterName2FactMetaData[name] = metaData;
                } else {
                    _mapParameterName2FactMetaData[name] = metaData;
                    metaData->setName(name);
                    metaData->setGroup(factGroup);
                    
                    if (xml.attributes().hasAttribute("default") && !strDefault.isEmpty()) {
                        QVariant varDefault;
                        
                        if (metaData->convertAndValidateRaw(strDefault, false, varDefault, errorString)) {
                            metaData->setRawDefaultValue(varDefault);
                        } else {
                            qCWarning(PX4ParameterMetaDataLog) << "Invalid default value, name:" << name << " type:" << type << " default:" << strDefault << " error:" << errorString;
                        }
                    }
                }
                
            } else {
                // We should be getting meta data now
                if (xmlState != XmlStateFoundParameter) {
                    qWarning() << "Badly formed XML";
                    return;
                }

                if (!badMetaData) {
                    if (elementName == "short_desc") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        text = text.replace("\n", " ");
                        qCDebug(PX4ParameterMetaDataLog) << "Short description:" << text;
                        metaData->setShortDescription(text);

                    } else if (elementName == "long_desc") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        text = text.replace("\n", " ");
                        qCDebug(PX4ParameterMetaDataLog) << "Long description:" << text;
                        metaData->setLongDescription(text);
                        
                    } else if (elementName == "min") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterMetaDataLog) << "Min:" << text;
                        
                        QVariant varMin;
                        if (metaData->convertAndValidateRaw(text, true /* convertOnly */, varMin, errorString)) {
                            metaData->setRawMin(varMin);
                        } else {
                            qCWarning(PX4ParameterMetaDataLog) << "Invalid min value, name:" << metaData->name() << " type:" << metaData->type() << " min:" << text << " error:" << errorString;
                        }
                        
                    } else if (elementName == "max") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterMetaDataLog) << "Max:" << text;
                        
                        QVariant varMax;
                        if (metaData->convertAndValidateRaw(text, true /* convertOnly */, varMax, errorString)) {
                            metaData->setRawMax(varMax);
                        } else {
                            qCWarning(PX4ParameterMetaDataLog) << "Invalid max value, name:" << metaData->name() << " type:" << metaData->type() << " max:" << text << " error:" << errorString;
                        }
                        
                    } else if (elementName == "unit") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterMetaDataLog) << "Unit:" << text;
                        metaData->setRawUnits(text);
                        
                    } else if (elementName == "decimal") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterMetaDataLog) << "Decimal:" << text;

                        bool convertOk;
                        QVariant varDecimals = QVariant(text).toUInt(&convertOk);
                        if (convertOk) {
                            metaData->setDecimalPlaces(varDecimals.toInt());
                        } else {
                            qCWarning(PX4ParameterMetaDataLog) << "Invalid decimals value, name:" << metaData->name() << " type:" << metaData->type() << " decimals:" << text << " error: invalid number";
                        }

                    } else if (elementName == "reboot_required") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterMetaDataLog) << "RebootRequired:" << text;
                        if (text.compare("true", Qt::CaseInsensitive) == 0) {
                            metaData->setRebootRequired(true);
                        }

                    } else if (elementName == "values") {
                        // doing nothing individual value will follow anyway. May be used for sanity checking.

                    } else if (elementName == "value") {
                        QString enumValueStr = xml.attributes().value("code").toString();
                        QString enumString = xml.readElementText();
                        qCDebug(PX4ParameterMetaDataLog) << "parameter value:"
                                                         << "value desc:" << enumString << "code:" << enumValueStr;

                        QVariant    enumValue;
                        QString     errorString;
                        if (metaData->convertAndValidateRaw(enumValueStr, false /* validate */, enumValue, errorString)) {
                            metaData->addEnumInfo(enumString, enumValue);
                        } else {
                            qCDebug(PX4ParameterMetaDataLog) << "Invalid enum value, name:" << metaData->name()
                                                             << " type:" << metaData->type() << " value:" << enumValueStr
                                                             << " error:" << errorString;
                        }
                    } else if (elementName == "increment") {
                        Q_ASSERT(metaData);
                        double  increment;
                        bool    ok;
                        QString text = xml.readElementText();
                        increment = text.toDouble(&ok);
                        if (ok) {
                            metaData->setIncrement(increment);
                        } else {
                            qCWarning(PX4ParameterMetaDataLog) << "Invalid value for increment, name:" << metaData->name() << " increment:" << text;
                        }

                    } else if (elementName == "boolean") {
                        QVariant    enumValue;
                        metaData->convertAndValidateRaw(1, false /* validate */, enumValue, errorString);
                        metaData->addEnumInfo(tr("Enabled"), enumValue);
                        metaData->convertAndValidateRaw(0, false /* validate */, enumValue, errorString);
                        metaData->addEnumInfo(tr("Disabled"), enumValue);

                    } else if (elementName == "bitmask") {
                        // doing nothing individual bits will follow anyway. May be used for sanity checking.

                    } else if (elementName == "bit") {
                        bool ok = false;
                        unsigned char bit = xml.attributes().value("index").toString().toUInt(&ok);
                        if (ok) {
                            QString bitDescription = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "parameter value:"
                                                             << "index:" << bit << "description:" << bitDescription;

                            if (bit < 31) {
                                QVariant bitmaskRawValue = 1 << bit;
                                QVariant bitmaskValue;
                                QString errorString;
                                if (metaData->convertAndValidateRaw(bitmaskRawValue, true, bitmaskValue, errorString)) {
                                    metaData->addBitmaskInfo(bitDescription, bitmaskValue);
                                } else {
                                    qCDebug(PX4ParameterMetaDataLog) << "Invalid bitmask value, name:" << metaData->name()
                                                                     << " type:" << metaData->type() << " value:" << bitmaskValue
                                                                     << " error:" << errorString;
                                }
                            } else {
                                qCWarning(PX4ParameterMetaDataLog) << "Invalid value for bitmask, bit:" << bit;
                            }
                        }
                    } else {
                        qCDebug(PX4ParameterMetaDataLog) << "Unknown element in XML: " << elementName;
                    }
                }
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "parameter") {
                // Done loading this parameter, validate default value
                if (metaData->defaultValueAvailable()) {
                    QVariant var;
                    
                    if (!metaData->convertAndValidateRaw(metaData->rawDefaultValue(), false /* convertOnly */, var, errorString)) {
                        qCWarning(PX4ParameterMetaDataLog) << "Invalid default value, name:" << metaData->name() << " type:" << metaData->type() << " default:" << metaData->rawDefaultValue() << " error:" << errorString;
                    }
                }
                
                // Reset for next parameter
                metaData = NULL;
                badMetaData = false;
                xmlState = XmlStateFoundGroup;
            } else if (elementName == "group") {
                xmlState = XmlStateFoundVersion;
            } else if (elementName == "parameters") {
                xmlState = XmlStateFoundParameters;
            }
        }
        xml.readNext();
    }
}

void PX4ParameterMetaData::addMetaDataToFact(Fact* fact, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType)

    if (_mapParameterName2FactMetaData.contains(fact->name())) {
        fact->setMetaData(_mapParameterName2FactMetaData[fact->name()]);
    }
}

void PX4ParameterMetaData::getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    QFile xmlFile(metaDataFile);

    if (!xmlFile.exists()) {
        qWarning() << "Internal error: metaDataFile mission" << metaDataFile;
        return;
    }

    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Internal error: Unable to open parameter file:" << metaDataFile << xmlFile.errorString();
        return;
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qWarning() << "Badly formed XML" << xml.errorString();
        return;
    }

    majorVersion = -1;
    minorVersion = -1;

    while (!xml.atEnd() && (majorVersion == -1 || minorVersion == -1)) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "parameter_version_major") {
                bool convertOk;
                QString strVersion = xml.readElementText();
                majorVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qWarning() << "Badly formed XML";
                    return;
                }
            } else if (elementName == "parameter_version_minor") {
                bool convertOk;
                QString strVersion = xml.readElementText();
                minorVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qWarning() << "Badly formed XML";
                    return;
                }
            }
        }
        xml.readNext();
    }

    // Assume defaults if not found
    if (majorVersion == -1) {
        majorVersion = 1;
    }
    if (minorVersion == -1) {
        minorVersion = 1;
    }
}
