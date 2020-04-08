/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTime>

#include "MAVLinkProtocol.h"
#include "ParameterManager.h"
#include "ComponentManager.h"
#include "QGCApplication.h"
#include "QGCImageProvider.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCCorePlugin.h"
#include <QDomDocument>
#include <QDomNodeList>

QGC_LOGGING_CATEGORY(ComponentManagerLog, "ComponentManagerLog")


//-- DOM Helpers

static bool read_attribute_str(QDomNode& node, const char* tagName, QString& result)
{
    QDomNamedNodeMap attrs = node.attributes();
    if (!attrs.count()) return false;
    QDomNode n = attrs.namedItem(tagName);
    if (n.isNull()) return false;
    result = n.nodeValue();
    return true;
}


static bool read_attribute_int(QDomNode& node, const char* tagName, int& result)
{
    QDomNamedNodeMap attrs = node.attributes();
    if (!attrs.count()) return false;
    QDomNode n = attrs.namedItem(tagName);
    if (n.isNull()) return false;
    result = n.nodeValue().toInt();
    return true;
}


static bool read_attribute_bool(QDomNode& node, const char* tagName, bool& result)
{
    QDomNamedNodeMap attrs = node.attributes();
    if (!attrs.count()) return false;
    QDomNode n = attrs.namedItem(tagName);
    if (n.isNull()) return false;
    result = (n.nodeValue().toLower() == "true");
    return true;
}


static bool read_value_str(QDomNode& node, const char* tagName, QString& result)
{
    QDomElement e = node.firstChildElement(tagName);
    if (e.isNull()) return false;
    result = e.text();
    return true;
}


static bool read_value_int(QDomNode& node, const char* tagName, int& result)
{
    QDomElement e = node.firstChildElement(tagName);
    if (e.isNull()) return false;
    result = e.text().toInt();
    return true;
}


static bool read_value_bool(QDomNode& node, const char* tagName, bool& result)
{
    QDomElement e = node.firstChildElement(tagName);
    if (e.isNull()) return false;
    result = (e.text().toLower() == "true");
    return true;
}


//-- Component Controller

ComponentControl::ComponentControl(const mavlink_component_information_t* compInfo, Vehicle* vehicle, int compId)
    : QObject       (vehicle)
    , _vehicle      (vehicle)
    , _compId       (compId)
{
    _netManager = nullptr;

    // Fill component information
    // construct a string by stopping at the first NUL (0) character, else copy the whole byte array
    _componentInfo.firmware_version = compInfo->firmware_version;
    _componentInfo.hardware_version = compInfo->hardware_version;
    _componentInfo.capability_flags = compInfo->capability_flags;
    _componentInfo.firmwareVersion = QString("%1.%2.%3.%4").arg(compInfo->firmware_version & 0xFF)
                                                           .arg((compInfo->firmware_version >> 8) & 0xFF)
                                                           .arg((compInfo->firmware_version >> 16) & 0xFF)
                                                           .arg((compInfo->firmware_version >> 24) & 0xFF);
    _componentInfo.hardwareVersion = QString("%1.%2.%3.%4").arg(compInfo->hardware_version & 0xFF)
                                                           .arg((compInfo->hardware_version >> 8) & 0xFF)
                                                           .arg((compInfo->hardware_version >> 16) & 0xFF)
                                                           .arg((compInfo->hardware_version >> 24) & 0xFF);
    QByteArray vBytes(reinterpret_cast<const char*>(compInfo->vendor_name), MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_VENDOR_NAME_LEN);
    _componentInfo.vendorName = QString(vBytes);
    QByteArray mBytes(reinterpret_cast<const char*>(compInfo->model_name), MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_MODEL_NAME_LEN);
    _componentInfo.modelName = QString(mBytes);
    QByteArray uBytes(compInfo->component_definition_uri, MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_COMPONENT_DEFINITION_URI_LEN);
    _componentInfo.definitionFileUri += QString(uBytes);

    qCDebug(ComponentManagerLog) << "$$$$" << "Component Controller created for component ID: " << compId;
    qCDebug(ComponentManagerLog) << "$$$$" << "    definition file:" << _componentInfo.definitionFileUri;
    qCDebug(ComponentManagerLog) << "$$$$" << "    firmwareVersion:" << _componentInfo.firmwareVersion;
    qCDebug(ComponentManagerLog) << "$$$$" << "    hardwareVersion:" << _componentInfo.hardwareVersion;
    qCDebug(ComponentManagerLog) << "$$$$" << "    vendorName:" << _componentInfo.vendorName;
    qCDebug(ComponentManagerLog) << "$$$$" << "    modelName:" << _componentInfo.modelName;

    _ok = false;

    if (_componentInfo.definitionFileUri.length() > 0) {
        qCDebug(ComponentManagerLog) << "$$$$" << "Start downloading definition file";
        _httpRequest(_componentInfo.definitionFileUri);
    }
}


ComponentControl::~ComponentControl()
{
    if (_netManager) {
        delete _netManager;
    }
}


void ComponentControl::_httpRequest(const QString& url)
{
    if (!_netManager) {
        _netManager = new QNetworkAccessManager(this);
    }
    QNetworkProxy savedProxy = _netManager->proxy();
    QNetworkProxy tempProxy;
    tempProxy.setType(QNetworkProxy::DefaultProxy);
    _netManager->setProxy(tempProxy);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply* reply = _netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &ComponentControl::_httpDownloadFinished);
    _netManager->setProxy(savedProxy);
}


void ComponentControl::_httpDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (!reply) {
        return;
    }

    int err = reply->error();
    int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if (err == QNetworkReply::NoError && http_code == 200) {
        data.append("\n");
    } else {
        data.clear();
        qWarning() << "$$$$" << QString("Error: Definition file download error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        return;
    }

    if (!data.size()) {
        qCDebug(ComponentManagerLog) << "$$$$" << "Error: Definition file is empty";
        return;
    }

    qCDebug(ComponentManagerLog) << "$$$$" << "Downloading definition file DONE";

    _loadComponentDefinitionFile(data);
}


bool ComponentControl::_loadComponentDefinitionFile(QByteArray& bytes)
{
    qCDebug(ComponentManagerLog) << "$$$$" << "Parse definition file for component ID:" << _compId;

    QByteArray originalData(bytes);
    QString errorMsg;
    int errorLine;
    QDomDocument doc;

    //-- Read it
    if(!doc.setContent(bytes, false, &errorMsg, &errorLine)) {
        qCDebug(ComponentManagerLog) << "$$$$" << "Error: Unable to parse definition file, error on line:" << errorLine;
        qCDebug(ComponentManagerLog) << "$$$$" << errorMsg;
        return false;
    }

    //-- Does it have groups?
    QDomNodeList groupElements = doc.elementsByTagName("group");
    if (!groupElements.size()) {
        qCDebug(ComponentManagerLog) << "$$$$" << "Error: Unable to load group elements from definition file";
        return false;
    }

    //-- Does it have parameters?
    QDomNodeList parameterElements = doc.elementsByTagName("parameter");
    if (!parameterElements.size()) {
        qCDebug(ComponentManagerLog) << "$$$$" << "Error: Unable to load parameter elements from definition file";
        return false;
    }

    //-- Load groups
    QMap<QString, QStringList> groupMap;

    for (int i = 0; i < groupElements.size(); i++) {
        QDomNode groupNode = groupElements.item(i);
        QString groupName;
        if (read_attribute_str(groupNode, "name", groupName)) {
            if (!groupNode.isElement()) {
                continue; //somthing is wrong here
            }
            QStringList groupMapParameterList;
            QDomNodeList groupParameterElements = groupNode.toElement().elementsByTagName("parameter");
            for (int i = 0; i < groupParameterElements.size(); i++) {
                QDomNode parameterNode = groupParameterElements.item(i);
                QString factName;
                if (read_attribute_str(parameterNode, "name", factName)) {
                    groupMapParameterList.append(factName);
                }
            }
            if (groupMapParameterList.size()) groupMap[groupName].append(groupMapParameterList);
        }
    }

    //-- Load parameters
    QMap<QString, ComponentParameterElement_t> parameterMap; //for the moment we just store the short description

    for (int i = 0; i < parameterElements.size(); i++) {
        QDomNode parameterNode = parameterElements.item(i);
        ComponentParameterElement_t p;
        QString factName;
        if (!read_attribute_str(parameterNode, "name", factName)) {
            continue; // we must have a fact name
        }
        QString factTypeStr;
        if (!read_attribute_str(parameterNode, "type", factTypeStr)) {
            continue; // we must have a fact type
        } else {
            bool unknownType;
            FactMetaData::ValueType_t factType = FactMetaData::stringToType(factTypeStr, unknownType);
            if (unknownType || (factType == FactMetaData::valueTypeCustom)) {
                continue; // we must have a fact type
            }
            p.factType = factType;
        }
        for (QString groupName: groupMap.keys()) {
            if (groupMap[groupName].contains(factName)) p.group = groupName;
        }
        if (!p.group.length()) {
            continue; // it must be member of a group
        }
        if (!read_attribute_str(parameterNode, "dispname", p.displayName)) {
            p.displayName = factName;
        }
        if (!read_attribute_bool(parameterNode, "hide", p.hide)) {
            p.hide = false;
        }
        read_value_str(parameterNode, "short_desc", p.shortDescription);
        read_value_str(parameterNode, "long_desc", p.longDescription);
        if (!read_value_bool(parameterNode, "reboot_required", p.rebootRequired)) {
            p.rebootRequired = false;
        }
        if (!read_value_bool(parameterNode, "read_only", p.readOnly)) {
            p.readOnly = false;
        }
        read_value_str(parameterNode, "unit", p.unit);
        if (read_value_int(parameterNode, "default", p.defaultValue)) {
            p.defaultValueAvailable = true;
        } else {
            p.defaultValue = 0;
            p.defaultValueAvailable = false;
        }
        if (!read_value_int(parameterNode, "min", p.minValue)) {
            p.minValue = p.defaultValue;
        }
        if (!read_value_int(parameterNode, "max", p.maxValue)) {
            p.maxValue = p.defaultValue;
        }
        if (p.minValue > p.maxValue) {
            continue; // min > max doesn't make sense, we allow min = max however!
        }
        if (p.defaultValue < p.minValue || p.defaultValue > p.maxValue) {
            continue; // default must be in range
        }
        if (!read_value_int(parameterNode, "increment", p.increment)) {
            p.increment = 1;
        }
        if (!read_value_int(parameterNode, "decimal", p.decimal)) {
            p.decimal = 0;
        }
        QDomElement valuesElement = parameterNode.firstChildElement("values");
        if (!valuesElement.isNull()) {
            QDomNodeList valueElementList = valuesElement.elementsByTagName("value");
            for (int i = 0; i < valueElementList.size(); i++) {
                QDomNode valueNode = valueElementList.item(i);
                int valueCode;
                if (!read_attribute_int(valueNode, "code", valueCode)) continue;
                QString valueName = valueNode.toElement().text();
                if (!valueName.length()) continue;
                p.valuesMap[valueCode] = valueName;
            }
            if (!p.valuesMap.size()) continue;
            if (p.minValue != 0) continue;
            //if (p.maxValue != p.valuesMap.size()-1) continue;
            if  (p.maxValue > p.valuesMap.size()-1) p.maxValue = p.valuesMap.size()-1;
            if (p.increment != 1) continue;
        }

        parameterMap[factName] = p;
    }

    //TODO: error handling if something not ok in parsing

    qCDebug(ComponentManagerLog) << "$$$$" << "Parsing definition file DONE";

    //-- Be happy
    _ok = true;
    _componentInfoGroupMap = groupMap;
    _componentInfoParameterMap = parameterMap;

    //-- If this is new, cache it
    // TODO: we currently don't do caching
    QString _cacheFile = QString("C:/Users/Olli/Documents/GitHub/test-%1.xml").arg(_compId); //OW !!!
    bool _cached = false;
    if (!_cached) {
        QFile file(_cacheFile);
        if (!file.open(QIODevice::WriteOnly)) {
            qCDebug(ComponentManagerLog) << "$$$$" << QString("Could not save cache file %1. Error: %2").arg(_cacheFile).arg(file.errorString());
        } else {
            file.write(originalData);
        }
    }

    return true;
}


//-- Component Manager

ComponentManager::ComponentManager(Vehicle* vehicle)
    : QObject       (vehicle)
    , _vehicle      (vehicle)
    , _logReplay    (vehicle->priorityLink() && vehicle->priorityLink()->isLogReplay())
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    //we connect directly to the Vehicle and not the UASInterface, it is thus more often called
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &ComponentManager::_mavlinkMessageReceived);

    _componentInfoRequestTimer.setSingleShot(true);
    _componentInfoRequestTimer.setInterval(3000);
    connect(&_componentInfoRequestTimer, &QTimer::timeout, this, &ComponentManager::_componentInfoRequestTimerTimeout);

    //_startRequestComponentInfo(); //we start the request after a component was seen
}


ComponentManager::~ComponentManager()
{
}


void ComponentManager::_startRequestComponentInfo(void)
{
    qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION request started";

    _componentInfoRequestRetryCount = 0;
    _componentControlMap.clear();

    _sendComponentInfoRequest(MAV_COMP_ID_ALL);
    _componentInfoRequestTimer.start();
}


void ComponentManager::stoptRequestComponentInfo(void) //this is public so it can be called from externally
{
    qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION request stoped";

    _componentInfoRequestRetryCount = _componentInfoRequestRetryMax;
    _componentInfoRequestTimer.stop();
}


void ComponentManager::_componentInfoRequestTimerTimeout(void)
{
    if (++_componentInfoRequestRetryCount <= _componentInfoRequestRetryMax) {
        qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION retry request";
        _sendComponentInfoRequest(MAV_COMP_ID_ALL);
        _componentInfoRequestTimer.start();
    } else {
        qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION request stopped, receivedcount: " << _componentControlMap.size();
    }
}


void ComponentManager::_sendComponentInfoRequest(int compId)
{
    if (_logReplay) {
        return;
    }

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_message_t msg;
    mavlink_msg_command_long_pack_chan(static_cast<uint8_t>(mavlink->getSystemId()),
                                       static_cast<uint8_t>(mavlink->getComponentId()),
                                       _vehicle->priorityLink()->mavlinkChannel(),
                                       &msg,
                                       static_cast<uint8_t>(_vehicle->id()),                        // target system
                                       static_cast<uint8_t>(compId),                                // target component
                                       static_cast<uint16_t>(MAV_CMD_REQUEST_MESSAGE),              // command id
                                       0,                                                           // 0=first transmission of command
                                       static_cast<double>(MAVLINK_MSG_ID_COMPONENT_INFORMATION),   // The MAVLink message ID of the requested message.
                                       0,0,0,0,0,
                                       0);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);

    qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION request for component ID:" << compId;
}


// handler for received MAVLink messages
void ComponentManager::_mavlinkMessageReceived(mavlink_message_t msg)
{
    // not for us, we also ignore broadcast sysid's
    if (msg.sysid != _vehicle->id()) return;

    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        _handleHeartbeatMessage(msg);
        return;
    }

    // only allow previously registered components, or broadcast messages
    if ((msg.compid != MAV_COMP_ID_ALL) && !_componentIdList.contains(msg.compid)) return;

    switch (msg.msgid) {
    case MAVLINK_MSG_ID_COMPONENT_INFORMATION:
        _handleComponentInfoMessage(msg);
        break;
    default:
        break;
    }
}


// handler for received HEARTBEAT message
void ComponentManager::_handleHeartbeatMessage(mavlink_message_t& msg)
{
    if (msg.compid == MAV_COMP_ID_ALL) return; // this should never happen, but play it safe

    // register new components, and start requests when the first component was seen
    if (!_componentIdList.contains(msg.compid)) {
        _componentIdList.append(msg.compid);
        qCDebug(ComponentManagerLog) << "!!!!" << "New conmponent registerd, component ID:" << msg.compid;
        if (_componentIdList.size() == 1) _startRequestComponentInfo();
    }
}


// handler for received COMPONENT_INFORMATION messages
void ComponentManager::_handleComponentInfoMessage(mavlink_message_t& msg)
{
    if (msg.compid == MAV_COMP_ID_ALL) return; // this should never happen, but play it safe

    //TODO: do we really want to do the if(_componentIdList.contains(msg.compid)), to allow only from known components? Makes sense to  me.

    if (_componentIdList.contains(msg.compid) && !_componentControlMap.keys().contains(msg.compid)) {
        qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION received for component ID: " << msg.compid;

        mavlink_component_information_t compInfo;
        mavlink_msg_component_information_decode(&msg, &compInfo);

        //TODO: we probably want to pass this through firmwarePlugin()
        ComponentControl* pComponent = new ComponentControl(&compInfo, _vehicle, msg.compid);
        if (pComponent) {
            _componentControlMap[msg.compid] = pComponent;
        }
    }
}



