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

#include "PX4ParameterFacts.h"
#include "QGCApplication.h"

#include <QFile>
#include <QDebug>

Q_LOGGING_CATEGORY(PX4ParameterFactsMetaDataLog, "PX4ParameterFactsMetaDataLog")

bool PX4ParameterFacts::_parameterMetaDataLoaded = false;
QMap<QString, FactMetaData*> PX4ParameterFacts::_mapParameterName2FactMetaData;

PX4ParameterFacts::PX4ParameterFacts(UASInterface* uas, QObject* parent) :
    FactLoader(uas, parent)
{
    Q_ASSERT(uas);
}

void PX4ParameterFacts::deleteParameterFactMetaData(void)
{
    foreach (QString param, _mapParameterName2FactMetaData.keys()) {
        delete _mapParameterName2FactMetaData[param];
    }
    _mapParameterName2FactMetaData.clear();
}

/// Parse the Parameter element of parameter xml meta data
///     @param[in] xml stream reader
///     @param[in] group fact group associated with this Param element
/// @return Returns the meta data object for this parameter
FactMetaData* PX4ParameterFacts::_parseParameter(QXmlStreamReader& xml, const QString& group)
{
    Q_UNUSED(group);
    
    QString name = xml.attributes().value("name").toString();
    QString type = xml.attributes().value("type").toString();
    
    qCDebug(PX4ParameterFactsMetaDataLog) << "Parsed parameter: " << name << type;

    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(!type.isEmpty());
    
    FactMetaData* metaData = new FactMetaData();
    Q_CHECK_PTR(metaData);
    
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
    for (size_t i=0; i<crgString2Type; i++) {
        const struct String2Type* info = &rgString2Type[i];
        
        if (type == info->strType) {
            found = true;
            metaData->type = info->type;
            break;
        }
    }
    Q_UNUSED(found);
    Q_ASSERT(found);
    
    _initMetaData(metaData);
    
    // FIXME: Change to change the parameter build scheme in Firmware to get rid of ifdef dup problem
    if (!_mapParameterName2FactMetaData.contains(name)) {
        _mapParameterName2FactMetaData[name] = metaData;
    }
    
    return metaData;
}

/// This will fill in missing meta data such as range info
void PX4ParameterFacts::_initMetaData(FactMetaData* metaData)
{
    switch (metaData->type) {
        case FactMetaData::valueTypeUint8:
            metaData->min = QVariant(std::numeric_limits<quint8>::min());
            metaData->max = QVariant(std::numeric_limits<quint8>::max());
            break;
        case FactMetaData::valueTypeInt8:
            metaData->min = QVariant(std::numeric_limits<qint8>::min());
            metaData->max = QVariant(std::numeric_limits<qint8>::max());
            break;
        case FactMetaData::valueTypeUint16:
            metaData->min = QVariant(std::numeric_limits<quint16>::min());
            metaData->max = QVariant(std::numeric_limits<quint16>::max());
            break;
        case FactMetaData::valueTypeInt16:
            metaData->min = QVariant(std::numeric_limits<qint16>::min());
            metaData->max = QVariant(std::numeric_limits<qint16>::max());
            break;
        case FactMetaData::valueTypeUint32:
            metaData->min = QVariant(std::numeric_limits<quint32>::min());
            metaData->max = QVariant(std::numeric_limits<quint32>::max());
            break;
        case FactMetaData::valueTypeInt32:
            metaData->min = QVariant(std::numeric_limits<qint32>::min());
            metaData->max = QVariant(std::numeric_limits<qint32>::max());
            break;
        case FactMetaData::valueTypeFloat:
            metaData->min = QVariant(std::numeric_limits<float>::min());
            metaData->max = QVariant(std::numeric_limits<float>::max());
            break;
        case FactMetaData::valueTypeDouble:
            metaData->min = QVariant(std::numeric_limits<double>::min());
            metaData->max = QVariant(std::numeric_limits<double>::max());
            break;
    }
}

/// Converts a string to a typed QVariant
/// @param type Type for Fact which dictates the QVariant type as well
/// @param failOk - true: Don't assert if convert fails
/// @return Returns the correctly type QVariant
QVariant PX4ParameterFacts::_stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk)
{
    QVariant var(string);
    bool convertOk;

    Q_UNUSED(convertOk);
    
    switch (type) {
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            convertOk = var.convert(QVariant::UInt);
            if (!failOk) {
                Q_ASSERT(convertOk);
            }
            break;
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            convertOk = var.convert(QVariant::Int);
            if (!failOk) {
                Q_ASSERT(convertOk);
            }
            break;
        case FactMetaData::valueTypeFloat:
        case FactMetaData::valueTypeDouble:
            convertOk = var.convert(QVariant::Double);
            if (!failOk) {
                Q_ASSERT(convertOk);
            }
            break;
    }
    
    return var;
}

/// Load Parameter Fact meta data
///
/// The meta data comes from firmware parameters.xml file.
void PX4ParameterFacts::loadParameterFactMetaData(void)
{
    if (_parameterMetaDataLoaded) {
        return;
    }
    _parameterMetaDataLoaded = true;
    
    qCDebug(PX4ParameterFactsMetaDataLog) << "Loading PX4 parameter fact meta data";

    Q_ASSERT(_mapParameterName2FactMetaData.count() == 0);
    
    QFile xmlFile(":/AutoPilotPlugins/PX4/ParameterFactMetaData.xml");
    
    Q_ASSERT(xmlFile.exists());
    
    bool success = xmlFile.open(QIODevice::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);
    
    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    
    QString factGroup;
    FactMetaData* metaData = NULL;
    
    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();
            if (elementName == "parameters") {
                // Just move to next state
            } else if (elementName == "group") {
                factGroup = xml.attributes().value("name").toString();
                qCDebug(PX4ParameterFactsMetaDataLog) << "Found group: " << factGroup;
            } else if (elementName == "parameter") {
                metaData = _parseParameter(xml, factGroup);
            } else if (elementName == "short_desc") {
                Q_ASSERT(metaData);
                QString text = xml.readElementText();
                metaData->shortDescription = text;
            } else if (elementName == "long_desc") {
                Q_ASSERT(metaData);
                QString text = xml.readElementText();
                metaData->longDescription = text;
            } else if (elementName == "default") {
                Q_ASSERT(metaData);
                QString text = xml.readElementText();
                // FIXME: failOk=true Is a hack to handle enum values in default value. Need
                // to implement enums in the meta data.
                metaData->defaultValue = _stringToTypedVariant(text, metaData->type, true /* failOk */);
            } else if (elementName == "min") {
                Q_ASSERT(metaData);
                QString text = xml.readElementText();
                metaData->min = _stringToTypedVariant(text, metaData->type);
            } else if (elementName == "max") {
                Q_ASSERT(metaData);
                QString text = xml.readElementText();
                metaData->max = _stringToTypedVariant(text, metaData->type);
                metaData->shortDescription = text;
            } else if (elementName == "unit") {
                Q_ASSERT(metaData);
                QString text = xml.readElementText();
                metaData->units = text;
            }
        } else if (xml.isEndElement() && xml.name() == "parameter") {
            metaData = NULL;
        }
        xml.readNext();
    }
}

void PX4ParameterFacts::clearStaticData(void)
{
    foreach(QString parameterName, _mapParameterName2FactMetaData.keys()) {
        delete _mapParameterName2FactMetaData[parameterName];
    }
    _mapParameterName2FactMetaData.clear();
    _parameterMetaDataLoaded = false;
}