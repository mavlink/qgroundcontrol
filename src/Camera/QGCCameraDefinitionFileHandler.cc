/*!
 *   @file
 *   @brief Camera Definition File Parser
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#include "QGCCameraDefinitionFileHandler.h"

QGC_LOGGING_CATEGORY(CameraDefinitionHandler, "CameraDefinitionHandler")

static const char* kCondition       = "condition";
static const char* kControl         = "control";
static const char* kDefault         = "default";
static const char* kDefnition       = "definition";
static const char* kDescription     = "description";
static const char* kExclusion       = "exclude";
static const char* kExclusions      = "exclusions";
static const char* kLocale          = "locale";
static const char* kLocalization    = "localization";
static const char* kMax             = "max";
static const char* kMin             = "min";
static const char* kModel           = "model";
static const char* kName            = "name";
static const char* kOption          = "option";
static const char* kOptions         = "options";
static const char* kOriginal        = "original";
static const char* kParameter       = "parameter";
static const char* kParameterrange  = "parameterrange";
static const char* kParameterranges = "parameterranges";
static const char* kParameters      = "parameters";
static const char* kReadOnly        = "readonly";
static const char* kWriteOnly       = "writeonly";
static const char* kRoption         = "roption";
static const char* kStep            = "step";
static const char* kDecimalPlaces   = "decimalPlaces";
static const char* kStrings         = "strings";
static const char* kTranslated      = "translated";
static const char* kType            = "type";
static const char* kUnit            = "unit";
static const char* kUpdate          = "update";
static const char* kUpdates         = "updates";
static const char* kValue           = "value";
static const char* kVendor          = "vendor";
static const char* kVersion         = "version";

//static const char* kPhotoMode       = "PhotoMode";
//static const char* kPhotoLapse      = "PhotoLapse";
//static const char* kPhotoLapseCount = "PhotoLapseCount";
//static const char* kThermalOpacity  = "ThermalOpacity";
//static const char* kThermalMode     = "ThermalMode";

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, bool& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue() != "0";
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, int& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue().toInt();
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, QString& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue();
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_value(QDomNode& element, const char* tagName, QString& target)
{
    QDomElement de = element.firstChildElement(tagName);
    if(de.isNull()) {
        return false;
    }
    target = de.text();
    return true;
}

//-----------------------------------------------------------------------------
QGCCameraDefinitionFileHandler::QGCCameraDefinitionFileHandler(QObject *parent)
    : QObject(parent)
{
}

//-----------------------------------------------------------------------------
QGCCameraDefinitionFileHandler::~QGCCameraDefinitionFileHandler()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::parse(QByteArray& bytes)
{
    QByteArray originalData(bytes);
    //-- Handle localization
    if(!_handleLocalization(bytes)) {
        return false;
    }
    int errorLine;
    QString errorMsg;
    QDomDocument doc;
    if(!doc.setContent(bytes, false, &errorMsg, &errorLine)) {
        qCritical() << "Unable to parse camera definition file on line:" << errorLine;
        qCritical() << errorMsg;
        return false;
    }
    //-- Load camera constants
    QDomNodeList defElements = doc.elementsByTagName(kDefnition);
    if(!defElements.size() || !_loadConstants(defElements)) {
        qWarning() <<  "Unable to load camera constants from camera definition";
        return false;
    }
    //-- Load camera parameters
    QDomNodeList paramElements = doc.elementsByTagName(kParameters);
    if(!paramElements.size() || !_loadSettings(paramElements)) {
        qWarning() <<  "Unable to load camera parameters from camera definition";
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::_handleLocalization(QByteArray& bytes)
{
    QString errorMsg;
    int errorLine;
    QDomDocument doc;
    if(!doc.setContent(bytes, false, &errorMsg, &errorLine)) {
        qCritical() << "Unable to parse camera definition file on line:" << errorLine;
        qCritical() << errorMsg;
        return false;
    }
    //-- Find out where we are
    QLocale locale = QLocale::system();
#if defined (Q_OS_MAC)
    locale = QLocale(locale.name());
#endif
    QString localeName = locale.name().toLower().replace("-", "_");
    qCDebug(CameraDefinitionHandler) << "Current locale:" << localeName;
    if(localeName == "en_us") {
        // Nothing to do
        return true;
    }
    QDomNodeList locRoot = doc.elementsByTagName(kLocalization);
    if(!locRoot.size()) {
        // Nothing to do
        return true;
    }
    //-- Iterate locales
    QDomNode node = locRoot.item(0);
    QDomElement elem = node.toElement();
    QDomNodeList locales = elem.elementsByTagName(kLocale);
    for(int i = 0; i < locales.size(); i++) {
        QDomNode locale = locales.item(i);
        QString name;
        if(!read_attribute(locale, kName, name)) {
            qWarning() << "Localization entry is missing its name attribute";
            continue;
        }
        // If we found a direct match, deal with it now
        if(localeName == name.toLower().replace("-", "_")) {
            return _replaceLocaleStrings(locale, bytes);
        }
    }
    //-- No direct match. Pick first matching language (if any)
    localeName = localeName.left(3);
    for(int i = 0; i < locales.size(); i++) {
        QDomNode locale = locales.item(i);
        QString name;
        read_attribute(locale, kName, name);
        if(name.toLower().startsWith(localeName)) {
            return _replaceLocaleStrings(locale, bytes);
        }
    }
    //-- Could not find a language to use
    qWarning() <<  "No match for" << QLocale::system().name() << "in camera definition file";
    //-- Just use default, en_US
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::_replaceLocaleStrings(const QDomNode node, QByteArray& bytes)
{
    QDomElement stringElem = node.toElement();
    QDomNodeList strings = stringElem.elementsByTagName(kStrings);
    for(int i = 0; i < strings.size(); i++) {
        QDomNode stringNode = strings.item(i);
        QString original;
        QString translated;
        if(read_attribute(stringNode, kOriginal, original)) {
            if(read_attribute(stringNode, kTranslated, translated)) {
                QString o; o = "\"" + original + "\"";
                QString t; t = "\"" + translated + "\"";
                bytes.replace(o.toUtf8(), t.toUtf8());
                o = ">" + original + "<";
                t = ">" + translated + "<";
                bytes.replace(o.toUtf8(), t.toUtf8());
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::_loadConstants(const QDomNodeList nodeList)
{
    QDomNode node = nodeList.item(0);
    if(!read_attribute(node, kVersion, _version)) {
        return false;
    }
    if(!read_value(node, kModel, _modelName)) {
        return false;
    }
    if(!read_value(node, kVendor, _vendor)) {
        return false;
    }

    emit constantsUpdated();
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::_loadSettings(const QDomNodeList nodeList)
{
    QDomNode node = nodeList.item(0);
    QDomElement elem = node.toElement();
    QDomNodeList parameters = elem.elementsByTagName(kParameter);
    //-- Pre-process settings (maintain order and skip non-controls)
    for(int i = 0; i < parameters.size(); i++) {
        QDomNode parameterNode = parameters.item(i);
        QString name;
        if(read_attribute(parameterNode, kName, name)) {
            bool control = true;
            read_attribute(parameterNode, kControl, control);
            if(control) {
                _settings << name;
            }
        } else {
            qCritical() << "Parameter entry missing parameter name";
            return false;
        }
    }
    //-- Load parameters
    for(int i = 0; i < parameters.size(); i++) {
        QDomNode parameterNode = parameters.item(i);
        QString factName;
        read_attribute(parameterNode, kName, factName);
        QString type;
        if(!read_attribute(parameterNode, kType, type)) {
            qCritical() << QString("Parameter %1 missing parameter type").arg(factName);
            return false;
        }
        //-- Does it have a control?
        bool control = true;
        read_attribute(parameterNode, kControl, control);
        //-- Is it read only?
        bool readOnly = false;
        read_attribute(parameterNode, kReadOnly, readOnly);
        //-- Is it write only?
        bool writeOnly = false;
        read_attribute(parameterNode, kWriteOnly, writeOnly);
        //-- It can't be both
        if(readOnly && writeOnly) {
            qCritical() << QString("Parameter %1 cannot be both read only and write only").arg(factName);
        }
        //-- Param type
        bool unknownType;
        FactMetaData::ValueType_t factType = FactMetaData::stringToType(type, unknownType);
        if (unknownType) {
            qCritical() << QString("Unknown type for parameter %1").arg(factName);
            return false;
        }
        //-- By definition, custom types do not have control
        if(factType == FactMetaData::valueTypeCustom) {
            control = false;
        }
        //-- Description
        QString description;
        if(!read_value(parameterNode, kDescription, description)) {
            qCritical() << QString("Parameter %1 missing parameter description").arg(factName);
            return false;
        }
        //-- Check for updates
        QStringList updates = _loadUpdates(parameterNode);
        if(updates.size()) {
            qCDebug(CameraDefinitionHandler) << "Parameter" << factName << "requires updates for:" << updates;
            _requestUpdates[factName] = updates;
        }
        //-- Build metadata
        FactMetaData* metaData = new FactMetaData(factType, factName, nullptr);
        metaData->setShortDescription(description);
        metaData->setLongDescription(description);
        metaData->setHasControl(control);
        metaData->setReadOnly(readOnly);
        metaData->setWriteOnly(writeOnly);
        //-- Options (enums)
        QDomElement optionElem = parameterNode.toElement();
        QDomNodeList optionsRoot = optionElem.elementsByTagName(kOptions);
        if(optionsRoot.size()) {
            //-- Iterate options
            QDomNode node = optionsRoot.item(0);
            QDomElement elem = node.toElement();
            QDomNodeList options = elem.elementsByTagName(kOption);
            for(int i = 0; i < options.size(); i++) {
                QDomNode option = options.item(i);
                QString optName;
                QString optValue;
                QVariant optVariant;
                if(!_loadNameValue(option, factName, metaData, optName, optValue, optVariant)) {
                    delete metaData;
                    return false;
                }
                metaData->addEnumInfo(optName, optVariant);
                _originalOptNames[factName]  << optName;
                _originalOptValues[factName] << optVariant;
                //-- Check for exclusions
                QStringList exclusions = _loadExclusions(option);
                if(exclusions.size()) {
                    qCDebug(CameraDefinitionHandler) << "New exclusions:" << factName << optValue << exclusions;
                    QGCCameraOptionExclusion* pExc = new QGCCameraOptionExclusion(this, factName, optValue, exclusions);
                    QQmlEngine::setObjectOwnership(pExc, QQmlEngine::CppOwnership);
                    _valueExclusions.append(pExc);
                }
                //-- Check for range rules
                if(!_loadRanges(option, factName, optValue)) {
                    delete metaData;
                    return false;
                }
            }
        }
        QString defaultValue;
        if(read_attribute(parameterNode, kDefault, defaultValue)) {
            QVariant defaultVariant;
            QString  errorString;
            if (metaData->convertAndValidateRaw(defaultValue, false, defaultVariant, errorString)) {
                metaData->setRawDefaultValue(defaultVariant);
            } else {
                qWarning() << "Invalid default value for" << factName
                           << " type:"  << metaData->type()
                           << " value:" << defaultValue
                           << " error:" << errorString;
            }
        }
        //-- Set metadata and Fact

        {
            //-- Check for Min Value
            QString attr;
            if(read_attribute(parameterNode, kMin, attr)) {
                QVariant typedValue;
                QString  errorString;
                if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                    metaData->setRawMin(typedValue);
                } else {
                    qWarning() << "Invalid min value for" << factName
                               << " type:"  << metaData->type()
                               << " value:" << attr
                               << " error:" << errorString;
                }
            }
        }
        {
            //-- Check for Max Value
            QString attr;
            if(read_attribute(parameterNode, kMax, attr)) {
                QVariant typedValue;
                QString  errorString;
                if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                    metaData->setRawMax(typedValue);
                } else {
                    qWarning() << "Invalid max value for" << factName
                               << " type:"  << metaData->type()
                               << " value:" << attr
                               << " error:" << errorString;
                }
            }
        }
        {
            //-- Check for Step Value
            QString attr;
            if(read_attribute(parameterNode, kStep, attr)) {
                QVariant typedValue;
                QString  errorString;
                if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                    metaData->setRawIncrement(typedValue.toDouble());
                } else {
                    qWarning() << "Invalid step value for" << factName
                               << " type:"  << metaData->type()
                               << " value:" << attr
                               << " error:" << errorString;
                }
            }
        }
        {
            //-- Check for Decimal Places
            QString attr;
            if(read_attribute(parameterNode, kDecimalPlaces, attr)) {
                QVariant typedValue;
                QString  errorString;
                if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                    metaData->setDecimalPlaces(typedValue.toInt());
                } else {
                    qWarning() << "Invalid decimal places value for" << factName
                               << " type:"  << metaData->type()
                               << " value:" << attr
                               << " error:" << errorString;
                }
            }
        }
        {
            //-- Check for Units
            QString attr;
            if(read_attribute(parameterNode, kUnit, attr)) {
                metaData->setRawUnits(attr);
            }
        }
        qCDebug(CameraDefinitionHandler) << "New parameter:" << factName << (readOnly ? "ReadOnly" : "Writable") << (writeOnly ? "WriteOnly" : "Readable");

        ParsedFactType newFact;
        newFact.name = factName;
        newFact.type = factType;
        newFact.metaData = metaData;

        _parsedFacts.append(newFact);
    }

    if(_parsedFacts.size() > 0) {
        emit factsUpdated();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::_loadNameValue(QDomNode option, const QString factName, FactMetaData* metaData, QString& optName, QString& optValue, QVariant& optVariant)
{
    if(!read_attribute(option, kName, optName)) {
        qCritical() << QString("Malformed option for parameter %1").arg(factName);
        return false;
    }
    if(!read_attribute(option, kValue, optValue)) {
        qCritical() << QString("Malformed value for parameter %1").arg(factName);
        return false;
    }
    QString  errorString;
    if (!metaData->convertAndValidateRaw(optValue, false, optVariant, errorString)) {
        qWarning() << "Invalid option value, name:" << factName
                   << " type:"  << metaData->type()
                   << " value:" << optValue
                   << " error:" << errorString;
    }
    return true;
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraDefinitionFileHandler::_loadExclusions(QDomNode option)
{
    QStringList exclusionList;
    QDomElement optionElem = option.toElement();
    QDomNodeList excRoot = optionElem.elementsByTagName(kExclusions);
    if(excRoot.size()) {
        //-- Iterate exclusions
        QDomNode node = excRoot.item(0);
        QDomElement elem = node.toElement();
        QDomNodeList exclusions = elem.elementsByTagName(kExclusion);
        for(int i = 0; i < exclusions.size(); i++) {
            QString exclude = exclusions.item(i).toElement().text();
            if(!exclude.isEmpty()) {
                exclusionList << exclude;
            }
        }
    }
    return exclusionList;
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraDefinitionFileHandler::_loadUpdates(QDomNode option)
{
    QStringList updateList;
    QDomElement optionElem = option.toElement();
    QDomNodeList updateRoot = optionElem.elementsByTagName(kUpdates);
    if(updateRoot.size()) {
        //-- Iterate updates
        QDomNode node = updateRoot.item(0);
        QDomElement elem = node.toElement();
        QDomNodeList updates = elem.elementsByTagName(kUpdate);
        for(int i = 0; i < updates.size(); i++) {
            QString update = updates.item(i).toElement().text();
            if(!update.isEmpty()) {
                updateList << update;
            }
        }
    }
    return updateList;
}

//-----------------------------------------------------------------------------
bool
QGCCameraDefinitionFileHandler::_loadRanges(QDomNode option, const QString factName, QString paramValue)
{
    QDomElement optionElem = option.toElement();
    QDomNodeList rangeRoot = optionElem.elementsByTagName(kParameterranges);
    if(rangeRoot.size()) {
        QDomNode node = rangeRoot.item(0);
        QDomElement elem = node.toElement();
        QDomNodeList parameterRanges = elem.elementsByTagName(kParameterrange);
        //-- Iterate parameter ranges
        for(int i = 0; i < parameterRanges.size(); i++) {
            QString param;
            QString condition;
            QDomNode paramRange = parameterRanges.item(i);
            if(!read_attribute(paramRange, kParameter, param)) {
                qCritical() << QString("Malformed option range for parameter %1").arg(factName);
                return false;
            }
            read_attribute(paramRange, kCondition, condition);
            QDomElement pelem = paramRange.toElement();
            QDomNodeList rangeOptions = pelem.elementsByTagName(kRoption);
            QStringList  optNames;
            QStringList  optValues;
            //-- Iterate options
            for(int i = 0; i < rangeOptions.size(); i++) {
                QString optName;
                QString optValue;
                QDomNode roption = rangeOptions.item(i);
                if(!read_attribute(roption, kName, optName)) {
                    qCritical() << QString("Malformed roption for parameter %1").arg(factName);
                    return false;
                }
                if(!read_attribute(roption, kValue, optValue)) {
                    qCritical() << QString("Malformed rvalue for parameter %1").arg(factName);
                    return false;
                }
                optNames  << optName;
                optValues << optValue;
            }
            if(optNames.size()) {
                QGCCameraOptionRange* pRange = new QGCCameraOptionRange(this, factName, paramValue, param, condition, optNames, optValues);
                _optionRanges.append(pRange);
                qCDebug(CameraDefinitionHandler) << "New range limit:" << factName << paramValue << param << condition << optNames << optValues;
            }
        }
    }
    return true;
}
