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

Q_LOGGING_CATEGORY(PX4ParameterFactsLog, "PX4ParameterFactsLog")
Q_LOGGING_CATEGORY(PX4ParameterFactsMetaDataLog, "PX4ParameterFactsMetaDataLog")

bool PX4ParameterFacts::_parameterMetaDataLoaded = false;
QMap<QString, FactMetaData*> PX4ParameterFacts::_mapParameterName2FactMetaData;

PX4ParameterFacts::PX4ParameterFacts(UASInterface* uas, QObject* parent) :
    QObject(parent),
    _lastSeenComponent(-1),
    _paramMgr(NULL),
    _factsReady(false)
{
    Q_ASSERT(uas);

    _uasId = uas->getUASID();
    
    _paramMgr = uas->getParamManager();
    Q_ASSERT(_paramMgr);
    
    // We need to be initialized before param mgr starts sending parameters so we catch each one
    Q_ASSERT(!_paramMgr->parametersReady());
    
    // We need to know when the param mgr is done sending the initial set of paramters
    connect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_paramMgrParameterListUpToDate()));
    
    // UASInterface::parameterChanged has multiple overrides so we need to use SIGNAL/SLOT style connect
    connect(uas, SIGNAL(parameterChanged(int, int, QString, QVariant)), this, SLOT(_parameterChanged(int, int, QString, QVariant)));
}

PX4ParameterFacts::~PX4ParameterFacts()
{
    foreach(Fact* fact, _mapFact2ParameterName.keys()) {
        delete fact;
    }
    _mapParameterName2Fact.clear();
    _mapFact2ParameterName.clear();
}

void PX4ParameterFacts::deleteParameterFactMetaData(void)
{
    foreach (QString param, _mapParameterName2FactMetaData.keys()) {
        delete _mapParameterName2FactMetaData[param];
    }
    _mapParameterName2FactMetaData.clear();
}

/// Connected to QGCUASParmManager::parameterChanged
///
/// When a new parameter is seen it is added to the system. If the parameter is already known it is updated.
void PX4ParameterFacts::_parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    // Is this for our uas?
    if (uas != _uasId) {
        return;
    }
    
    if (_lastSeenComponent == -1) {
        _lastSeenComponent = component;
    } else {
        // Code cannot handle parameters coming form different components yets
        Q_ASSERT(component == _lastSeenComponent);
    }
    
    // If we don't have meta data for the parameter it can't be part of the FactSystem
    if (!_mapParameterName2FactMetaData.contains(parameterName)) {
        // FIXME: Debug or Warning. Warning will fail TC
        qDebug() << "FactSystem meta data out of date. Missing parameter:" << parameterName;
        return;
    }
    
    if (!_mapParameterName2Fact.contains(parameterName)) {
        Fact* fact = new Fact(this);
        
        fact->setMetaData(_mapParameterName2FactMetaData[parameterName]);
        
        _mapParameterName2Fact[parameterName] = fact;
        _mapFact2ParameterName[fact] = parameterName;
        
        // We need to know when the fact changes so that we can send the new value to the parameter manager
        connect(fact, &Fact::_containerValueChanged, this, &PX4ParameterFacts::_valueUpdated);
        
        //qDebug() << "Adding new fact" << parameterName;
    }
    //qDebug() << "Updating fact value" << parameterName << value;
    _mapParameterName2Fact[parameterName]->_containerSetValue(value);
}

/// Connected to Fact::valueUpdated
///
/// Sets the new value into the Parameter Manager. Paramter is persisted after send.
void PX4ParameterFacts::_valueUpdated(QVariant value)
{
    Fact* fact = qobject_cast<Fact*>(sender());
    Q_ASSERT(fact);
    
    Q_ASSERT(_lastSeenComponent != -1);
    Q_ASSERT(_paramMgr);
    Q_ASSERT(_mapFact2ParameterName.contains(fact));
    
    QVariant typedValue;
    switch (fact->type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue = QVariant(value.value<int>());
            
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue = QVariant(value.value<uint>());
            break;
            
        case FactMetaData::valueTypeFloat:
            typedValue = QVariant(value.toFloat());
            break;
            
        case FactMetaData::valueTypeDouble:
            typedValue = QVariant(value.toDouble());
            break;
    }
    
    _paramMgr->setParameter(_lastSeenComponent, _mapFact2ParameterName[fact], typedValue);
    _paramMgr->sendPendingParameters(true /* persistAfterSend */, false /* forceSend */);
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
                qCDebug(PX4ParameterFactsLog) << "Found group: " << factGroup;
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

// Called when param mgr list is up to date
void PX4ParameterFacts::_paramMgrParameterListUpToDate(void)
{
    if (!_factsReady) {
        _factsReady = true;
        
        // We don't need this any more
        disconnect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_paramMgrParameterListUpToDate()));

        // There may be parameterUpdated signals still in our queue. Flush them out.
        qgcApp()->processEvents();
        
        // We should have all paramters now so we can signal ready
        emit factsReady();
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