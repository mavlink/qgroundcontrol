#include "FactSystem.h"
#include "UASInterface.h"
#include "MAVLinkProtocol.h"
#include <limits>

#include <QXmlStreamReader>
#include <QDebug>
#include <QStringList>
#include <QApplication>

typedef struct {
    MAV_AUTOPILOT   autopilot;
    const char*     directory;
} AutopilotDirectory_t;

static const AutopilotDirectory_t rgAutopilotDirectory[] = {
    { MAV_AUTOPILOT_GENERIC,        "Generic" },
    { MAV_AUTOPILOT_PIXHAWK,        "Pixhawk" },
    { MAV_AUTOPILOT_SLUGS,          "Slugs" },
    { MAV_AUTOPILOT_ARDUPILOTMEGA,  "ArdupilotMega" },
};

typedef struct {
    MAV_TYPE    mavType;
    const char* directory;
} MavTypeDirectory_t;

static const MavTypeDirectory_t rgMavTypeDirectory[] = {
    { MAV_TYPE_GENERIC,         "Generic" },
    { MAV_TYPE_FIXED_WING,      "FixedWing" },
    { MAV_TYPE_QUADROTOR,       "QuadRotor" },
    { MAV_TYPE_COAXIAL,         "Coaxial" },
    { MAV_TYPE_HELICOPTER,      "Helicopter" },
    { MAV_TYPE_ANTENNA_TRACKER, "AntennaTracker" },
    { MAV_TYPE_AIRSHIP,         "Airship" },
    { MAV_TYPE_FREE_BALLOON,    "FreeBalloon" },
    { MAV_TYPE_ROCKET,          "Rocket" },
    { MAV_TYPE_GROUND_ROVER,    "GroundRover" },
    { MAV_TYPE_SURFACE_BOAT,    "SurfaceBoat" },
    { MAV_TYPE_SUBMARINE,       "Submarine" },
    { MAV_TYPE_HEXAROTOR,       "HexaRotor" },
    { MAV_TYPE_OCTOROTOR,       "OctoRotor" },
    { MAV_TYPE_TRICOPTER,       "TriCopter" },
    { MAV_TYPE_FLAPPING_WING,   "FlappingWing" },
    { MAV_TYPE_KITE,            "Kite" },
};

FactHandler::FactHandler(QObject* parent) :
    QObject(parent),
    _uas(NULL)
{

}

void FactHandler::setup(UASInterface* uas)
{
    bool connected;
    
    _uas = uas;
    
    // Connect to the rule handler so we receive new rule facts
    connected = connect(&_uas->getFactRuleHandler(),
                        SIGNAL(evaluatedRuleData(int, const QStringList&, QList<float>&)),
                        this,
                        SLOT(_receivedRuleData(int, const QStringList&, QList<float>&)));
    Q_ASSERT_X(connected, "FactRuleHandler::evaluatedRuleData", "connect failed");
    
    connected = connect(&uas->getFactMavShim(),
                        SIGNAL(receivedParamValue(int, const QString&, quint8, float, quint16, quint16)),
                        this,
                        SLOT(_receivedParamValue(int, const QString&, quint8, float, quint16, quint16)));
    Q_ASSERT_X(connected, "MavLinkProtocol::receivedParamValue", "connect failed");
    
    connected = connect(&uas->getFactMavShim(),
                        SIGNAL(receivedTelemValue(int, const QString&, quint8, QVariant)),
                        this,
                        SLOT(_receivedTelemValue(int, const QString&, quint8, QVariant)));
    Q_ASSERT_X(connected, "MavLinkProtocol::receivedTelemValue", "connect failed");
    
    
    connected = connect(&_uas->getFactMavShim(),
                        SIGNAL(mavtypeKnown(int, uint8_t, uint8_t)),
                        this,
                        SLOT(_loadMetaData(int, uint8_t, uint8_t)));
    Q_ASSERT_X(connected, "FactMavShim::mavtypeKnown", "connect failed");
    
    // Hack in a setting fact for testing
    Fact fact;
    fact.setProvider(Fact::settingProvider);
    fact.setId("RC_CHANNELS_USED");
    fact.setType(Fact::valueTypeInt16);
    fact.setValue(4);
    _providerFacts[Fact::settingProvider].insert(fact.id(), fact);
}

/// Signalled whenever a parameter is received through mavlink
void FactHandler::_receivedParamValue(int uasId, const QString& factId, quint8 type, float value, quint16 index, quint16 count)
{
    Q_UNUSED(index);
    Q_UNUSED(count);
    
    Q_ASSERT(_uas);
    
    if (uasId != _uas->getUASID()) {
        return;
    }
    
    Q_ASSERT_X(containsFact(Fact::parameterProvider, factId),
               "Facthandler::_receivedParamValue",
               QString("Received value for unknown parameter fact(%1)").arg(factId).toAscii().constData());
    
    Fact& fact = getFact(Fact::parameterProvider, factId);
    
    // Initial loading of Fact is from meta data which does not have type, so make sure to update type
    
    Fact::ValueType_t valueType;
    switch (type) {
        case MAV_PARAM_TYPE_UINT8:
            valueType = Fact::valueTypeUint8;
            break;
        case MAV_PARAM_TYPE_INT8:
            valueType = Fact::valueTypeInt8;
            break;
        case MAV_PARAM_TYPE_UINT16:
            valueType = Fact::valueTypeUint16;
            break;
        case MAV_PARAM_TYPE_INT16:
            valueType = Fact::valueTypeInt16;
            break;
        case MAV_PARAM_TYPE_UINT32:
            valueType = Fact::valueTypeUint32;
            break;
        case MAV_PARAM_TYPE_INT32:
            valueType = Fact::valueTypeInt32;
            break;
        case MAV_PARAM_TYPE_UINT64:
            valueType = Fact::valueTypeUint64;
            break;
        case MAV_PARAM_TYPE_INT64:
            valueType = Fact::valueTypeInt64;
            break;
        case MAV_PARAM_TYPE_REAL32:
            valueType = Fact::valueTypeFloat;
            break;
        case MAV_PARAM_TYPE_REAL64:
            valueType = Fact::valueTypeDouble;
            break;
    }
    fact.setType(valueType);

    if (fact.isModified() || (fact.value() != value)) {
        fact.setModified(false);
        fact.setValue(value);
        emit factUpdated(_uas->getUASID(), Fact::parameterProvider, factId);
    }
}

/// Signalled when new telemetry data is received through mavlink. We use this to update the associcated fact
///     @param rawIds list of raw fact ids which we have telem data on
///     @param values list of values associated with the specified telem fact ids in same index order (rawIDs[n].value = values[n])
void FactHandler::_receivedTelemValue(int uasId, const QString& rawId, quint8 type, QVariant value)
{
    Q_ASSERT(_uas);
    
    if (uasId != _uas->getUASID()) {
        return;
    }

    //qDebug() << "Telemetry fact signal" << factId << factValue;

    Q_ASSERT_X(containsFact(Fact::telemetryProvider, rawId),
               "Facthandler::_receivedTelemValue",
               QString("Received data from unknown telemetry fact(%1)").arg(rawId).toAscii().constData());
    
    Fact& fact = getFact(Fact::telemetryProvider, rawId);
    
    fact.setValue(value);
    emit factUpdated(_uas->getUASID(), Fact::telemetryProvider, rawId);
}

void FactHandler::_receivedRuleData(int uasId, const QStringList& rawIds, QList<float>& values)
{
    Q_ASSERT(_uas);
    
    if (uasId != _uas->getUASID()) {
        return;
    }
    
    Q_ASSERT(rawIds.count() == values.count());
    
    for (int i=0; i<rawIds.count(); i++) {
        QString factId = rawIds[i];
        float factValue = values[i];
        
        //qDebug() << "Rule fact signal" << factId << factValue;
        
        Q_ASSERT_X(containsFact(Fact::ruleProvider, factId),
                   "Facthandler::_receivedRuleData",
                   QString("Received data from unknown rule fact(%1)").arg(factId).toAscii().constData());
        
        Fact& fact = getFact(Fact::ruleProvider, factId);
        
        if (fact.value() != factValue) {
            fact.setValue(factValue);
            emit factUpdated(_uas->getUASID(), Fact::ruleProvider, factId);
        }
    }
}


bool FactHandler::containsFact(Fact::Provider_t provider, const QString& id) const
{
    return _providerFacts.contains(provider) && _providerFacts[provider].contains(id);
}

Fact& FactHandler::getFact(Fact::Provider_t provider, const QString& id)
{
    Q_ASSERT_X(containsFact(provider, id),
               "FactHandler::getFact",
               QString("Unknown fact(%1%2) requested").arg(Fact::getProviderPrefix(provider)).arg(id).toAscii().constData());
    return _providerFacts[provider][id];
}

/// Loads the meta data associated with parameters and telemetry, also loads parameter defaults
///     @param autopilot as specified by the MAV_AUTOPILOT enum
///     @param mavtype as specified by the MAV_TYPE enum
void FactHandler::_loadMetaData(int uasId, uint8_t autopilot, uint8_t mavType)
{
    Q_ASSERT(_uas);
    
    if (uasId != _uas->getUASID()) {
        return;
    }
    
    disconnect(&_uas->getFactMavShim(),
                        SIGNAL(mavtypeKnown(int, uint8_t, uint8_t)),
                        this,
                        SLOT(_loadMetaData(int, uint8_t, uint8_t)));
    
    // Figure out which directories to pull meta data from
    
    Q_ASSERT(rgAutopilotDirectory[0].autopilot == MAV_AUTOPILOT_GENERIC);   // default to generic, make sure we can find it
    QString autopilotDirectory;
    for (size_t i=0; i<sizeof(rgAutopilotDirectory)/sizeof(rgAutopilotDirectory[0]); i++) {
        if (rgAutopilotDirectory[i].autopilot == autopilot) {
            autopilotDirectory = rgAutopilotDirectory[i].directory;
        }
    }
    Q_ASSERT(!autopilotDirectory.isEmpty());
    if (autopilotDirectory.isEmpty()) {
        autopilotDirectory = rgAutopilotDirectory[0].directory;
    }
    
    Q_ASSERT(rgMavTypeDirectory[0].mavType == MAV_TYPE_GENERIC);   // default to generic, make sure we can find it
    QString mavTypeDirectory;
    for (size_t i=0; i<sizeof(rgMavTypeDirectory)/sizeof(rgMavTypeDirectory[0]); i++) {
        if (rgMavTypeDirectory[i].mavType == mavType) {
            mavTypeDirectory = rgMavTypeDirectory[i].directory;
        }
    }
    Q_ASSERT(!mavTypeDirectory.isEmpty());
    if (mavTypeDirectory.isEmpty()) {
        mavTypeDirectory = rgMavTypeDirectory[0].directory;
    }
    
    QString parameterMetaDataFile = QString("/files/MetaData/%1/%2/parameter.metadata.xml").arg(autopilotDirectory).arg(mavTypeDirectory);
    QString telemetryCommonMetaDataFile = QString("/files/MetaData/%1/mavlinkcommon.metadata.xml").arg(autopilotDirectory);
    QString telemetrySpecificMetaDataFile = QString("/files/MetaData/%1/mavlink.metadata.xml").arg(autopilotDirectory);
    QString parameterDefaultsFile = QString("/files/MetaData/%1/%2/parameter.defaults.param").arg(autopilotDirectory).arg(mavTypeDirectory);
    
    // FIXME: Rule meta data is common to autopilot for now
    QString ruleMetaDataFile = QString("/files/MetaData/%1/rule.metadata.xml").arg(autopilotDirectory);
    QString ruleLuaFile = QString("/files/MetaData/%1/commonrules.lua").arg(autopilotDirectory);
    
    // FIXME: This is nasty
    Q_ASSERT(_uas);
    _uas->getFactRuleHandler().setRuleFile(ruleLuaFile);
    
    qDebug() << "Parameter meta data" << parameterMetaDataFile;
    qDebug() << "Rule meta data" << ruleMetaDataFile;
    qDebug() << "Telemetry common meta data" << telemetryCommonMetaDataFile;
    qDebug() << "Telemetry specific meta data" << telemetrySpecificMetaDataFile;
    qDebug() << "Parameter defaults" << parameterDefaultsFile;
    
    _loadXmlMetaData(parameterMetaDataFile, Fact::parameterProvider);
    _loadXmlMetaData(ruleMetaDataFile, Fact::ruleProvider);
    _loadMavlinkMetaData(telemetryCommonMetaDataFile);
    _loadMavlinkMetaData(telemetrySpecificMetaDataFile);
    _loadParameterDefaults(parameterDefaultsFile);
}

/// Parses the field element
///     @param[in] factId fact that this field element is associated with
///     @param[in] provider fact subtype that this field element is associated with
void FactHandler::_parseField(QXmlStreamReader& xml, Fact::Provider_t provider, const QString& factId)
{
    Q_ASSERT(containsFact(provider, factId));
    
    Fact& fact = getFact(provider, factId);
    
    QString fieldType = xml.attributes().value("name").toString();
    QString text = xml.readElementText();
    
    if (fieldType == "Range") {
        //Some range fields list "0-10" and some list "0 10". Handle both.
        if (text.split(" ").size() > 1)
        {
            float rangeMin = text.split(" ")[0].trimmed().toFloat();
            float rangeMax = text.split(" ")[1].trimmed().toFloat();
            fact.setRangeMin(rangeMin);
            fact.setRangeMax(rangeMax);
        } else if (text.split("-").size() > 1) {
            float rangeMin = text.split("-")[0].trimmed().toFloat();
            float rangeMax = text.split("-")[1].trimmed().toFloat();
            fact.setRangeMin(rangeMin);
            fact.setRangeMax(rangeMax);
        } else {
            Q_ASSERT_X(false,
                       "FactMetaDataHandler::_parseField",
                       QString("Unknown range format for fact(%1) range(%2) xml(%3)").arg(factId).arg(text).arg(xml.tokenString()).toAscii().constData());
        }
    } else if (fieldType == "Increment") {
        fact.setIncrement(text.trimmed().toFloat());
    } else if (fieldType == "Units") {
        fact.setUnits(text);
    } else {
        Q_ASSERT_X(false,
                   "FactMetaDataHandler::_parseField",
                   QString("Unknown field type(%1) fact(%2) xml(%3)").arg(fieldType).arg(factId).arg(xml.tokenString()).toAscii().constData());
    }
}

/// Parse the Value element
///     @param[in] factId fact that this field element is associated with
///     @param[in] provider fact provider that this field element is associated with
void FactHandler::_parseValue(QXmlStreamReader& xml, Fact::Provider_t provider, const QString& factId)
{
    Q_ASSERT(containsFact(provider, factId));
    
    Fact& fact = getFact(provider, factId);
    
    Fact::ValueLabel_t label;
    
    bool converted;
    if (fact.valueIsBitmask()) {
        int exponent = xml.attributes().value("code").toString().toInt(&converted);
        label.value = 2 ^ exponent;
    } else {
        label.value = xml.attributes().value("code").toString().toFloat(&converted);
    }
    Q_ASSERT_X(converted,
               "FactHandler::_parseValue",
               QString("Bad number in value provider(%1) factId(%2) xml(%3)").arg(provider).arg(factId).arg(xml.tokenString()).toAscii().constData());
    label.label = xml.readElementText();
    
    fact.addValueLabel(label);
}

/// Parse the Param element
///     @param[in] xml stream reader
///     @param[in] provider fact provider which this parameter belongs to
///     @param[in] group fact group associated with this Param element
/// @return Returns the fact id for this Parameter
QString FactHandler::_parseParam(QXmlStreamReader& xml, Fact::Provider_t provider, const QString& group)
{
    
    QString rawFactId = xml.attributes().value("name").toString();
    if (rawFactId.contains(":"))
    {
        rawFactId = rawFactId.split(":")[1];
    }
    QString factId = rawFactId;
    
    // Sometimes parameters end up more than once in the xml file. This is caused by ifdef's in the code
    // which the apm parser doesn't handle. Last one wins.
    if (containsFact(provider, factId)) {
        _removeFact(provider, factId);
    }
    
    Fact fact;
    fact.setProvider(provider);
    fact.setId(factId);
    fact.setGroup(group);
    fact.setName(xml.attributes().value("humanName").toString());
    fact.setReadOnly(provider != Fact::parameterProvider);
    
    if (xml.attributes().hasAttribute("documentation")) {
        fact.setDescription(xml.attributes().value("documentation").toString());
    }
    
    //qDebug() << "Added metadata" << provider << factId;
    
    // Keep track of fact id lists
    _providerFactIds[provider] += factId;
    
    // Add the new meta data
    _providerFacts[provider][factId] = fact;
    
    return factId;
}

/// This will fill in missing meta data such as range info
void FactHandler::_finalizeFact(Fact::Provider_t provider, const QString& factId)
{
    Q_ASSERT(containsFact(provider, factId));
    Fact& fact = getFact(provider, factId);
    
    if (!fact.hasRange()) {
        switch (fact.type()) {
            case Fact::valueTypeChar:
                fact.setRangeMin(std::numeric_limits<unsigned char>::min());
                fact.setRangeMax(std::numeric_limits<unsigned char>::max());
                break;
            case Fact::valueTypeUint8:
                fact.setRangeMin(std::numeric_limits<quint8>::min());
                fact.setRangeMax(std::numeric_limits<quint8>::max());
                break;
            case Fact::valueTypeInt8:
                fact.setRangeMin(std::numeric_limits<qint8>::min());
                fact.setRangeMax(std::numeric_limits<qint8>::max());
                break;
            case Fact::valueTypeUint16:
                fact.setRangeMin(std::numeric_limits<quint16>::min());
                fact.setRangeMax(std::numeric_limits<quint16>::max());
                break;
            case Fact::valueTypeInt16:
                fact.setRangeMin(std::numeric_limits<qint16>::min());
                fact.setRangeMax(std::numeric_limits<qint16>::max());
                break;
            case Fact::valueTypeUint32:
                fact.setRangeMin(std::numeric_limits<quint32>::min());
                fact.setRangeMax(std::numeric_limits<quint32>::max());
                break;
            case Fact::valueTypeInt32:
                fact.setRangeMin(std::numeric_limits<qint32>::min());
                fact.setRangeMax(std::numeric_limits<qint32>::max());
                break;
            case Fact::valueTypeUint64:
                fact.setRangeMin(std::numeric_limits<quint64>::min());
                fact.setRangeMax(std::numeric_limits<quint64>::max());
                break;
            case Fact::valueTypeInt64:
                fact.setRangeMin(std::numeric_limits<qint64>::min());
                fact.setRangeMax(std::numeric_limits<qint64>::max());
                break;
            case Fact::valueTypeFloat:
                fact.setRangeMin(std::numeric_limits<float>::min());
                fact.setRangeMax(std::numeric_limits<float>::max());
                break;
            case Fact::valueTypeDouble:
                fact.setRangeMin(std::numeric_limits<double>::min());
                fact.setRangeMax(std::numeric_limits<double>::max());
                break;
        }
    }
}

/// Loads the specified meta data file which is in the APM parameter meta data format
///     @param filename file to load (relative path from app location)
///     @param provider provider for which we are loading metadata
void FactHandler::_loadXmlMetaData(const QString& filename, Fact::Provider_t provider)
{
    QFile xmlFile(qApp->applicationDirPath() + filename);
    
    Q_ASSERT(xmlFile.exists());
    
    bool bRet = xmlFile.open(QIODevice::ReadOnly);
    Q_ASSERT(bRet == true);
    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    
    QString factId;
    QString factGroup;
    
    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();
            if (elementName == "paramfile") {
                // init parser state
            } else if (elementName == "vehicles") {
                //
            } else if(elementName == "libraries") {
                //
            } else if (elementName == "parameters") {
                factGroup = xml.attributes().value("name").toString();
            } else if (elementName == "param") {
                factId = _parseParam(xml, provider, factGroup);
            } else if (elementName == "values") {
                // just move on to next element
            } else if (elementName == "value") {
                _parseValue(xml, provider, factId);
            } else if (elementName == "field") {
                _parseField(xml, provider, factId);
            }
        } else if (xml.isEndElement() && xml.name() == "param") {
            _finalizeFact(provider, factId);
            factId.clear();
        }
        xml.readNext();
    }
}

void FactHandler::_loadParameterDefaults(const QString& filename)
{
    QFile defaultsFile(qApp->applicationDirPath() + filename);
    
    Q_ASSERT(defaultsFile.exists());
    
    bool bRet = defaultsFile.open(QIODevice::ReadOnly);
    Q_ASSERT(bRet == true);
    
    int lineCount = 0;
    QTextStream defaultsStream(&defaultsFile);
    do {
        QString line(defaultsStream.readLine());
        QRegExp parser("(.*),(.*)");
        
        if (!line.startsWith("#")) {
            int match = parser.indexIn(line);
            if (match == -1 || parser.captureCount() != 2) {
                qDebug() << QString("Incorrectly formatted parameter default file: file(%1:%2) '%3'").arg(defaultsFile.fileName()).arg(lineCount).arg(line);
                Q_ASSERT(match != -1);
                Q_ASSERT(parser.captureCount() == 2);
            }
            
            QString factId = parser.cap(1);
            QString strValue = parser.cap(2);
            
            Q_ASSERT_X(containsFact(Fact::parameterProvider, factId),
                       "FactHandler::_loadParameterDefaults",
                       QString("Missing parameter for default value fact(%1)").arg(factId).toAscii().constData());
            
            bool converted;
            
            Fact& fact = getFact(Fact::parameterProvider, factId);
            fact.setDefaultValue(strValue.toFloat(&converted));
            Q_ASSERT(converted);
        }
        
        lineCount++;
    } while (!defaultsStream.atEnd());
}

void FactHandler::_removeFact(Fact::Provider_t provider, const QString& factId)
{
    Q_ASSERT(_providerFacts.contains(provider));
    _providerFacts[provider].take(factId);
}

const QString& FactHandler::getFactIdByIndex(Fact::Provider_t provider, int index) const
{
    Q_ASSERT(_providerFactIds.contains(provider));
    Q_ASSERT(index >= 0 && index < _providerFactIds[provider].count());
    
    return _providerFactIds[provider][index];
}

QStringListIterator FactHandler::factIdIterator(Fact::Provider_t provider)
{
    Q_ASSERT(_providerFactIds.contains(provider));
    return QStringListIterator(_providerFactIds[provider]);
}

int FactHandler::getFactCount(Fact::Provider_t provider) const
{
    Q_ASSERT(_providerFactIds.contains(provider));
    return _providerFactIds[provider].count();
}

QStringList FactHandler::getFactIdsForGroup(Fact::Provider_t provider, const QString& group) const
{
    Q_ASSERT(_providerFacts.contains(provider));
    
    QStringList factIds;
    
    QMapIterator<QString, Fact> i(_providerFacts[provider]);
    while (i.hasNext()) {
        i.next();
        const Fact& fact = i.value();
        if (fact.group() == group) {
            factIds += fact.id();
        }
    }
    
    return factIds;
}

/// Parse the mavlink message element
///     @param[in] xml stream reader
/// @return Returns the group for this message
QString FactHandler::_parseMavlinkMessage(QXmlStreamReader& xml)
{
    return xml.attributes().value("name").toString();
}

/// Parse the mavlink field element
///     @param[in] xml stream reader
///     @param[in] group group this field is associated with
/// @return Returns the group for this message
void FactHandler::_parseMavlinkField(QXmlStreamReader& xml, const QString& group)
{
    typedef struct {
        const char*         str;
        Fact::ValueType_t   type;
    } TypeStrXlat_t;
    
    static const TypeStrXlat_t rgTypeStrXlat[] = {
        { "char",       Fact::valueTypeChar },
        { "uint8_t",    Fact::valueTypeUint8 },
        { "int8_t",     Fact::valueTypeInt8 },
        { "uint16_t",   Fact::valueTypeUint16 },
        { "int16_t",    Fact::valueTypeInt16 },
        { "uint32_t",   Fact::valueTypeUint32 },
        { "int32_t",    Fact::valueTypeInt32 },
        { "uint64",     Fact::valueTypeUint64 },
        { "int64_t",    Fact::valueTypeInt64 },
        { "float",      Fact::valueTypeFloat },
        { "double",     Fact::valueTypeDouble }
    };
    
    // Add the new fact
    QString name = xml.attributes().value("name").toString();
    QString factId = group + "_" + name;
    Fact& fact = _providerFacts[Fact::telemetryProvider][factId];
    _providerFactIds[Fact::telemetryProvider] += factId;
    fact.setId(factId);
    fact.setGroup(group);
    fact.setName(name);

    // Translate type
    Fact::ValueType_t type;
    QString typeStr = xml.attributes().value("type").toString();
    for (size_t i=0; i<sizeof(rgTypeStrXlat)/sizeof(rgTypeStrXlat[0]); i++) {
        const TypeStrXlat_t* pxlat = &rgTypeStrXlat[i];
        if (typeStr.startsWith(pxlat->str)) {
            fact.setType(pxlat->type);

            // Look for an array
            QRegExp reg("\\[([0-9]*)\\]$");
            if (reg.indexIn(typeStr) != -1 && reg.captureCount() == 1) {
                bool converted;
                int arrayLength = reg.cap(1).toInt(&converted);
                Q_ASSERT_X(converted,
                           "FactHandler::_parseMavlinkField",
                           QString("Invalid array count in mavlink field type fact(%1) type(%2)").arg(factId).arg(typeStr).toAscii().constData());
                fact.setArrayLength(arrayLength);
            }
            break;
        }
    }
    
    // Description
    fact.setDescription(xml.readElementText());
}

void FactHandler::_loadMavlinkMetaData(const QString& filename)
{
    QFile xmlFile(qApp->applicationDirPath() + filename);
    
    Q_ASSERT(xmlFile.exists());
    
    bool bRet = xmlFile.open(QIODevice::ReadOnly);
    Q_ASSERT(bRet == true);
    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    
    QString group;
    
    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();
            if (elementName == "enum") {
            } else if (elementName == "message") {
                group = _parseMavlinkMessage(xml);
            } else if(elementName == "description") {
                //
            } else if (elementName == "field") {
                _parseMavlinkField(xml, group);
            }
        } else if (xml.isEndElement() && xml.name() == "message") {
            group.clear();
        }
        xml.readNext();
    }
}

