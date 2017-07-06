/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCCameraControl.h"
#include "QGCCameraIO.h"
#include <QDomDocument>
#include <QDomNodeList>

QGC_LOGGING_CATEGORY(CameraControlLog, "CameraControlLog")
QGC_LOGGING_CATEGORY(CameraControlLogVerbose, "CameraControlLogVerbose")

static const char* kDefnition       = "definition";
static const char* kParameters      = "parameters";
static const char* kParameter       = "parameter";
static const char* kVersion         = "version";
static const char* kModel           = "model";
static const char* kVendor          = "vendor";
static const char* kLocalization    = "localization";
static const char* kLocale          = "locale";
static const char* kStrings         = "strings";
static const char* kName            = "name";
static const char* kValue           = "value";
static const char* kControl         = "control";
static const char* kOptions         = "options";
static const char* kOption          = "option";
static const char* kType            = "type";
static const char* kDefault         = "default";
static const char* kDescription     = "description";
static const char* kExclusions      = "exclusions";
static const char* kExclusion       = "exclude";
static const char* kRoption         = "roption";
static const char* kCondition       = "condition";
static const char* kParameterranges = "parameterranges";
static const char* kParameterrange  = "parameterrange";
static const char* kOriginal        = "original";
static const char* kTranslated      = "translated";

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
QGCCameraControl::QGCCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : FactGroup(0, parent)
    , _vehicle(vehicle)
    , _compID(compID)
    , _version(0)
    , _cameraMode(CAMERA_MODE_UNDEFINED)
{
    memcpy(&_info, &info, sizeof(mavlink_camera_information_t));
    if(1 /*!_info.definition_uri[0]*/) {
      //-- Process camera definition file
        _loadCameraDefinitionFile("/Users/gus/github/Yuneec/camera_repeater/yuneec_udp/def_files/e90.xml");
    }

    if(isBasic()) {
        _requestCameraSettings();
    } else {
        _requestAllParameters();
        QTimer::singleShot(2000, this, &QGCCameraControl::_requestCameraSettings);
    }

    emit infoChanged();
}

//-----------------------------------------------------------------------------
QGCCameraControl::~QGCCameraControl()
{
    //-- Clear param IO queue (if any)
    foreach(QString paramName, _paramIO.keys()) {
        if(_paramIO[paramName]) {
            delete _paramIO[paramName];
        }
    }
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::firmwareVersion()
{
    int major = (_info.firmware_version >> 24) & 0xFF;
    int minor = (_info.firmware_version >> 16) & 0xFF;
    int build = _info.firmware_version & 0xFFFF;
    QString ver;
    ver.sprintf("%d.%d.%d", major, minor, build);
    return ver;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setCameraMode(CameraMode mode)
{
    qCDebug(CameraControlLog) << "setCameraMode(" << mode << ")";
    if(mode == CAMERA_MODE_VIDEO) {
        setVideoMode();
    } else if(mode == CAMERA_MODE_PHOTO) {
        setPhotoMode();
    } else {
        qCDebug(CameraControlLog) << "setCameraMode() Invalid mode:" << mode;
        return;
    }
    _cameraMode = mode;
    emit cameraModeChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::toggleMode()
{
    if(cameraMode() == CAMERA_MODE_PHOTO) {
        setVideoMode();
    } else if(cameraMode() == CAMERA_MODE_VIDEO) {
        setPhotoMode();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::takePhoto()
{

}

//-----------------------------------------------------------------------------
void
QGCCameraControl::startVideo()
{

}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stopVideo()
{

}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setVideoMode()
{
    if(_vehicle && _cameraMode != CAMERA_MODE_VIDEO) {
        qCDebug(CameraControlLog) << "setVideoMode()";
        //-- Use basic MAVLink message
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_MODE,                // Command id
            true,                                   // ShowError
            0,                                      // Camera ID (0 for all, 1 for first, 2 for second, etc.) ==> TODO: Remove
            CAMERA_MODE_VIDEO);                     // Camera mode (0: photo, 1: video)
        _cameraMode = CAMERA_MODE_VIDEO;
        emit cameraModeChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoMode()
{
    if(_vehicle && _cameraMode != CAMERA_MODE_PHOTO) {
        qCDebug(CameraControlLog) << "setPhotoMode()";
        //-- Use basic MAVLink message
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_MODE,                // Command id
            true,                                   // ShowError
            0,                                      // Camera ID (0 for all, 1 for first, 2 for second, etc.) ==> TODO: Remove
            CAMERA_MODE_PHOTO);                     // Camera mode (0: photo, 1: video)
        _cameraMode = CAMERA_MODE_PHOTO;
        emit cameraModeChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::resetSettings()
{
    qCDebug(CameraControlLog) << "resetSettings()";
    if(_vehicle) {
        //-- Use basic MAVLink message
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_RESET_CAMERA_SETTINGS,          // Command id
            true,                                   // ShowError
            0,                                      // Camera ID (0 for all, 1 for first, 2 for second, etc.) ==> TODO: Remove
            1);                                     // Do Reset
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::formatCard(int id)
{
    qCDebug(CameraControlLog) << "formatCard()";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_STORAGE_FORMAT,                 // Command id
            true,                                   // ShowError
            id,                                     // Storage ID (1 for first, 2 for second, etc.)
            1);                                     // Do Format
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::factChanged(Fact* pFact)
{
    _updateActiveList();
    _updateRanges(pFact);
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadCameraDefinitionFile(const QString& file)
{
    QFile xmlFile(file);
    if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open camera definition file" << file << xmlFile.errorString();
        return false;
    }
    QByteArray bytes = xmlFile.readAll();
    xmlFile.close();
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
        qWarning() <<  "Unable to load camera constants from camera definition file" << file;
        return false;
    }
    //-- Load camera parameters
    QDomNodeList paramElements = doc.elementsByTagName(kParameters);
    if(!paramElements.size() || !_loadSettings(paramElements)) {
        qWarning() <<  "Unable to load camera parameters from camera definition file" << file;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadConstants(const QDomNodeList nodeList)
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
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadSettings(const QDomNodeList nodeList)
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
        bool unknownType;
        FactMetaData::ValueType_t factType = FactMetaData::stringToType(type, unknownType);
        if (unknownType) {
            qCritical() << QString("Unknown type for parameter %1").arg(factName);
            return false;
        }
        QString description;
        if(!read_value(parameterNode, kDescription, description)) {
            qCritical() << QString("Parameter %1 missing parameter description").arg(factName);
            return false;
        }
        //-- Build metadata
        FactMetaData* metaData = new FactMetaData(factType, factName, this);
        metaData->setShortDescription(description);
        metaData->setLongDescription(description);
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
                    qCDebug(CameraControlLogVerbose) << "New exclusions:" << factName << optValue << exclusions;
                    QGCCameraOptionExclusion* pExc = new QGCCameraOptionExclusion(this, factName, optValue, exclusions);
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
        if (_nameToFactMetaDataMap.contains(factName)) {
            qWarning() << QStringLiteral("Duplicate fact name:") << factName;
            delete metaData;
        } else {
            _nameToFactMetaDataMap[factName] = metaData;
            Fact* pFact = new Fact(_compID, factName, factType, this);
            pFact->setMetaData(metaData);
            pFact->setRawValue(metaData->rawDefaultValue(), true);
            QGCCameraParamIO* pIO = new QGCCameraParamIO(this, pFact, _vehicle);
            _paramIO[factName] = pIO;
            _addFact(pFact, factName);
        }
    }
    if(_nameToFactMetaDataMap.size() > 0) {
        _addFactGroup(this, "camera");
        _processRanges();
        _activeSettings = _settings;
        emit activeSettingsChanged();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_handleLocalization(QByteArray& bytes)
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
#if defined (__macos__)
    locale = QLocale(locale.name());
#endif
    QString localeName = locale.name().toLower().replace("-", "_");
    qCDebug(CameraControlLog) << "Current locale:" << localeName;
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
QGCCameraControl::_replaceLocaleStrings(const QDomNode node, QByteArray& bytes)
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
void
QGCCameraControl::_requestAllParameters()
{
    //-- Reset receive list
    foreach(QString pramName, _paramIO.keys()) {
        _paramIO[pramName]->setParamRequest();
    }
    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_message_t msg;
    mavlink_msg_param_ext_request_list_pack_chan(
        mavlink->getSystemId(),
        mavlink->getComponentId(),
        _vehicle->priorityLink()->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        compID());
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
    qCDebug(CameraControlLogVerbose) << "Request all parameters";
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::_getParamName(const char* param_id)
{
    QByteArray bytes(param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
    QString parameterName(bytes);
    return parameterName;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleParamAck(const mavlink_param_ext_ack_t& ack)
{
    QString paramName = _getParamName(ack.param_id);
    if(!_paramIO.contains(paramName)) {
        qCWarning(CameraControlLog) << "Received PARAM_EXT_ACK for unknown param:" << paramName;
        return;
    }
    _paramIO[paramName]->handleParamAck(ack);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleParamValue(const mavlink_param_ext_value_t& value)
{
    QString paramName = _getParamName(value.param_id);
    if(!_paramIO.contains(paramName)) {
        qCWarning(CameraControlLog) << "Received PARAM_EXT_VALUE for unknown param:" << paramName;
        return;
    }
    _paramIO[paramName]->handleParamValue(value);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_updateActiveList()
{
    //-- Clear out excluded parameters based on exclusion rules
    QStringList exclusionList;
    foreach(QGCCameraOptionExclusion* param, _valueExclusions) {
        Fact* pFact = getFact(param->param);
        if(pFact) {
            QString option = pFact->rawValueString();
            if(param->value == option) {
                exclusionList << param->exclusions;
            }
        }
    }
    qCDebug(CameraControlLogVerbose) << "Excluding" << exclusionList;
    _activeSettings.clear();
    foreach(QString key, _settings) {
        if(!exclusionList.contains(key)) {
            _activeSettings.append(key);
        }
    }
    emit activeSettingsChanged();
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_processConditionTest(const QString conditionTest)
{
    enum {
        TEST_NONE,
        TEST_EQUAL,
        TEST_NOT_EQUAL,
        TEST_GREATER,
        TEST_SMALLER
    };
    qCDebug(CameraControlLogVerbose) << "_processConditionTest(" << conditionTest << ")";
    int op = TEST_NONE;
    QStringList test;
    if(conditionTest.contains("!=")) {
        test = conditionTest.split("!=", QString::SkipEmptyParts);
        op = TEST_NOT_EQUAL;
    } else if(conditionTest.contains("=")) {
        test = conditionTest.split("=", QString::SkipEmptyParts);
        op = TEST_EQUAL;
    } else if(conditionTest.contains(">")) {
        test = conditionTest.split(">", QString::SkipEmptyParts);
        op = TEST_GREATER;
    } else if(conditionTest.contains("<")) {
        test = conditionTest.split("<", QString::SkipEmptyParts);
        op = TEST_SMALLER;
    }
    if(test.size() == 2) {
        Fact* pFact = getFact(test[0]);
        if(pFact) {
            switch(op) {
            case TEST_EQUAL:
                return pFact->rawValueString() == test[1];
            case TEST_NOT_EQUAL:
                return pFact->rawValueString() != test[1];
            case TEST_GREATER:
                return pFact->rawValueString() > test[1];
            case TEST_SMALLER:
                return pFact->rawValueString() < test[1];
            case TEST_NONE:
                break;
            }
        } else {
            qWarning() << "Invalid condition parameter:" << test[0] << "in" << conditionTest;
            return false;
        }
    }
    qWarning() << "Invalid condition" << conditionTest;
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_processCondition(const QString condition)
{
    qCDebug(CameraControlLogVerbose) << "_processCondition(" << condition << ")";
    bool result = true;
    bool andOp  = true;
    if(!condition.isEmpty()) {
        QStringList scond = condition.split(" ", QString::SkipEmptyParts);
        while(scond.size()) {
            QString test = scond.first();
            scond.removeFirst();
            if(andOp) {
                result = result && _processConditionTest(test);
            } else {
                result = result || _processConditionTest(test);
            }
            if(!scond.size()) {
                return result;
            }
            andOp = scond.first().toUpper() == "AND";
            scond.removeFirst();
        }
    }
    return result;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_updateRanges(Fact* pFact)
{
    QStringList changedList;
    QStringList resetList;
    qCDebug(CameraControlLogVerbose) << "_updateRanges(" << pFact->name() << ")";
    //-- Iterate range sets
    foreach(QGCCameraOptionRange* pRange, _optionRanges) {
        //-- If this fact or one of its conditions is part of this range set
        if(!changedList.contains(pRange->targetParam) && (pRange->param == pFact->name() || pRange->condition.contains(pFact->name()))) {
            Fact* pRFact = getFact(pRange->param);          //-- This parameter
            Fact* pTFact = getFact(pRange->targetParam);    //-- The target parameter (the one its range is to change)
            if(pRFact && pTFact) {
                QString option = pRFact->rawValueString();  //-- This parameter value
                //-- If this value (and condition) triggers a change in the target range
                if(pRange->value == option && _processCondition(pRange->condition)) {
                    //-- Set limited range set
                    pTFact->setEnumInfo(pRange->optNames, pRange->optVariants);
                    emit pTFact->enumStringsChanged();
                    emit pTFact->enumValuesChanged();
                    qCDebug(CameraControlLogVerbose) << "Limited:" << pRange->targetParam << pRange->optNames;
                    changedList << pRange->targetParam;
                } else {
                    if(!resetList.contains(pRange->targetParam)) {
                        //-- Restore full option set
                        pTFact->setEnumInfo(_originalOptNames[pRange->targetParam], _originalOptValues[pRange->targetParam]);
                        emit pTFact->enumStringsChanged();
                        emit pTFact->enumValuesChanged();
                        qCDebug(CameraControlLogVerbose) << "Full:" << pRange->targetParam << _originalOptNames[pRange->targetParam];
                        resetList << pRange->targetParam;
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestCameraSettings()
{
    qCDebug(CameraControlLog) << "_requestCameraSettings()";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_REQUEST_CAMERA_SETTINGS,        // command id
            false,                                  // showError
            0,                                      // Camera ID (0 for all, 1 for first, 2 for second, etc.) ==> TODO: Remove
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleSettings(const mavlink_camera_settings_t& settings)
{
    _cameraMode = (CameraMode)settings.mode_id;
    emit cameraModeChanged();
    qCDebug(CameraControlLog) << "handleSettings() Mode:" << _cameraMode;
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraControl::_loadExclusions(QDomNode option)
{
    QStringList exclusionList;
    QDomElement optionElem = option.toElement();
    QDomNodeList excRoot = optionElem.elementsByTagName(kExclusions);
    if(excRoot.size()) {
        //-- Iterate options
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
bool
QGCCameraControl::_loadRanges(QDomNode option, const QString factName, QString paramValue)
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
            QMap<QString, QVariant> rangeList;
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
                qCDebug(CameraControlLogVerbose) << "New range limit:" << factName << paramValue << param << condition << optNames << optValues;
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_processRanges()
{
    //-- After all parameter are loaded, process parameter ranges
    foreach(QGCCameraOptionRange* pRange, _optionRanges) {
        Fact* pRFact = getFact(pRange->targetParam);
        if(pRFact) {
            for(int i = 0; i < pRange->optNames.size(); i++) {
                QVariant optVariant;
                QString  errorString;
                if (!pRFact->metaData()->convertAndValidateRaw(pRange->optValues[i], false, optVariant, errorString)) {
                    qWarning() << "Invalid roption value, name:" << pRange->targetParam
                               << " type:"  << pRFact->metaData()->type()
                               << " value:" << pRange->optValues[i]
                               << " error:" << errorString;
                } else {
                    pRange->optVariants << optVariant;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadNameValue(QDomNode option, const QString factName, FactMetaData* metaData, QString& optName, QString& optValue, QVariant& optVariant)
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
