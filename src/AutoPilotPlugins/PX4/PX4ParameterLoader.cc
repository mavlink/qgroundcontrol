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

#include "PX4ParameterLoader.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QGC_LOGGING_CATEGORY(PX4ParameterLoaderLog, "PX4ParameterLoaderLog")

bool PX4ParameterLoader::_parameterMetaDataLoaded = false;
QMap<QString, FactMetaData*> PX4ParameterLoader::_mapParameterName2FactMetaData;

PX4ParameterLoader::PX4ParameterLoader(AutoPilotPlugin* autopilot, UASInterface* uas, QObject* parent) :
    ParameterLoader(autopilot, uas, parent)
{
    Q_ASSERT(uas);
}

/// Converts a string to a typed QVariant
///     @param string String to convert
///     @param type Type for Fact which dictates the QVariant type as well
///     @param convertOk Returned: true: conversion success, false: conversion failure
/// @return Returns the correctly type QVariant
QVariant PX4ParameterLoader::_stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool* convertOk)
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

/// Load Parameter Fact meta data
///
/// The meta data comes from firmware parameters.xml file.
void PX4ParameterLoader::loadParameterFactMetaData(void)
{
    if (_parameterMetaDataLoaded) {
        return;
    }
    _parameterMetaDataLoaded = true;
    
    qCDebug(PX4ParameterLoaderLog) << "Loading PX4 parameter fact meta data";

    Q_ASSERT(_mapParameterName2FactMetaData.count() == 0);

    QString parameterFilename;
    
    // We want unit test builds to always use the resource based meta data to provide repeatable results
    if (!qgcApp()->runningUnitTests()) {
        // First look for meta data that comes from a firmware download. Fall back to resource if not there.
        QSettings settings;
        QDir parameterDir = QFileInfo(settings.fileName()).dir();
        parameterFilename = parameterDir.filePath("PX4ParameterFactMetaData.xml");
    }
	if (parameterFilename.isEmpty() || !QFile(parameterFilename).exists()) {
		parameterFilename = ":/AutoPilotPlugins/PX4/ParameterFactMetaData.xml";
	}
	
    qCDebug(PX4ParameterLoaderLog) << "Loading parameter meta data:" << parameterFilename;

    QFile xmlFile(parameterFilename);
    Q_ASSERT(xmlFile.exists());
    
    bool success = xmlFile.open(QIODevice::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);
    
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
                    qDebug() << "Parameter version stamp too old, skipping load. Found:" << intVersion << "Want: 3 File:" << parameterFilename;
                    return;
                }
                
                
            } else if (elementName == "group") {
                if (xmlState != XmlStateFoundVersion) {
                    // We didn't get a version stamp, assume older version we can't read
                    qDebug() << "Parameter version stamp not found, skipping load" << parameterFilename;
                    return;
                }
                xmlState = XmlStateFoundGroup;
                
                if (!xml.attributes().hasAttribute("name")) {
                    qWarning() << "Badly formed XML";
                    return;
                }
                factGroup = xml.attributes().value("name").toString();
                qCDebug(PX4ParameterLoaderLog) << "Found group: " << factGroup;
                
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
                QString strDefault = xml.attributes().value("default").toString();
                
                qCDebug(PX4ParameterLoaderLog) << "Found parameter name:" << name << " type:" << type << " default:" << strDefault;

                // Convert type from string to FactMetaData::ValueType_t
                
                struct String2Type {
                    const char*                 strType;
                    FactMetaData::ValueType_t   type;
                };
                
                static const struct String2Type rgString2Type[] = {
                    { "FLOAT",  FactMetaData::valueTypeFloat },
                    { "INT32",  FactMetaData::valueTypeInt32 },
                };
                static const size_t crgString2Type = sizeof(rgString2Type) / sizeof(rgString2Type[0]);
                
                bool found = false;
                FactMetaData::ValueType_t foundType;
                for (size_t i=0; i<crgString2Type; i++) {
                    const struct String2Type* info = &rgString2Type[i];
                    
                    if (type == info->strType) {
                        found = true;
                        foundType = info->type;
                        break;
                    }
                }
                if (!found) {
                    qWarning() << "Parameter meta data with bad type:" << type << " name:" << name;
                    return;
                }
                
                // Now that we know type we can create meta data object and add it to the system
                
                metaData = new FactMetaData(foundType);
                Q_CHECK_PTR(metaData);
                if (_mapParameterName2FactMetaData.contains(name)) {
                    // We can't trust the meta dafa since we have dups
                    qCWarning(PX4ParameterLoaderLog) << "Duplicate parameter found:" << name;
                    badMetaData = true;
                    // Reset to default meta data
                    _mapParameterName2FactMetaData[name] = metaData;
                } else {
                    _mapParameterName2FactMetaData[name] = metaData;
                    metaData->setName(name);
                    metaData->setGroup(factGroup);
                    
                    if (xml.attributes().hasAttribute("default") && !strDefault.isEmpty()) {
                        QVariant varDefault;
                        
                        if (metaData->convertAndValidate(strDefault, false, varDefault, errorString)) {
                            metaData->setDefaultValue(varDefault);
                        } else {
                            qCWarning(PX4ParameterLoaderLog) << "Invalid default value, name:" << name << " type:" << type << " default:" << strDefault << " error:" << errorString;
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
                        qCDebug(PX4ParameterLoaderLog) << "Short description:" << text;
                        metaData->setShortDescription(text);

                    } else if (elementName == "long_desc") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        text = text.replace("\n", " ");
                        qCDebug(PX4ParameterLoaderLog) << "Long description:" << text;
                        metaData->setLongDescription(text);
                        
                    } else if (elementName == "min") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterLoaderLog) << "Min:" << text;
                        
                        QVariant varMin;
                        if (metaData->convertAndValidate(text, true /* convertOnly */, varMin, errorString)) {
                            metaData->setMin(varMin);
                        } else {
                            qCWarning(PX4ParameterLoaderLog) << "Invalid min value, name:" << metaData->name() << " type:" << metaData->type() << " min:" << text << " error:" << errorString;
                        }
                        
                    } else if (elementName == "max") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterLoaderLog) << "Max:" << text;
                        
                        QVariant varMax;
                        if (metaData->convertAndValidate(text, true /* convertOnly */, varMax, errorString)) {
                            metaData->setMax(varMax);
                        } else {
                            qCWarning(PX4ParameterLoaderLog) << "Invalid max value, name:" << metaData->name() << " type:" << metaData->type() << " max:" << text << " error:" << errorString;
                        }
                        
                    } else if (elementName == "unit") {
                        Q_ASSERT(metaData);
                        QString text = xml.readElementText();
                        qCDebug(PX4ParameterLoaderLog) << "Unit:" << text;
                        metaData->setUnits(text);
                        
                    } else {
                        qDebug() << "Unknown element in XML: " << elementName;
                    }
                }
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "parameter") {
                // Done loading this parameter, validate default value
                if (metaData->defaultValueAvailable()) {
                    QVariant var;
                    
                    if (!metaData->convertAndValidate(metaData->defaultValue(), false /* convertOnly */, var, errorString)) {
                        qCWarning(PX4ParameterLoaderLog) << "Invalid default value, name:" << metaData->name() << " type:" << metaData->type() << " default:" << metaData->defaultValue() << " error:" << errorString;
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

void PX4ParameterLoader::clearStaticData(void)
{
    foreach(QString parameterName, _mapParameterName2FactMetaData.keys()) {
        delete _mapParameterName2FactMetaData[parameterName];
    }
    _mapParameterName2FactMetaData.clear();
    _parameterMetaDataLoaded = false;
}

/// Override from FactLoad which connects the meta data to the fact
void PX4ParameterLoader::_addMetaDataToFact(Fact* fact)
{
    if (_mapParameterName2FactMetaData.contains(fact->name())) {
        fact->setMetaData(_mapParameterName2FactMetaData[fact->name()]);
    } else {
        // Use generic meta data
        ParameterLoader::_addMetaDataToFact(fact);
    }
}
