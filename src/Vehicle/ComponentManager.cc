/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTime>
#include <QDateTime>
#include <QLocale>
#include <QQuaternion>

#include "MAVLinkProtocol.h"
#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "FirmwarePlugin.h"
#include "UAS.h"
#include "ParameterManager.h"
#include "ComponentManager.h"
#include "QGCApplication.h"
#include "QGCImageProvider.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCCorePlugin.h"
#include <QDomDocument>
#include <QDomNodeList>

QGC_LOGGING_CATEGORY(ComponentManagerLog, "ComponentManagerLog")


// Standard connected vehicle
ComponentManager::ComponentManager(Vehicle* vehicle)
    : QObject                           (vehicle)
    , _vehicle                          (vehicle)
    , _mavlink                          (nullptr)
    , _logReplay                        (vehicle->priorityLink() && vehicle->priorityLink()->isLogReplay())
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    //we connect directly to the Vehicle and not the UASInterface, it is thus more often called
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &ComponentManager::_mavlinkMessageReceived);

    _componentInfoRequestTimer.setSingleShot(true);
    _componentInfoRequestTimer.setInterval(3000);
    connect(&_componentInfoRequestTimer, &QTimer::timeout, this, &ComponentManager::_componentInfoRequestTimerTimeout);

    _startRequestComponentInfo();
}


ComponentManager::~ComponentManager()
{
}


void ComponentManager::_startRequestComponentInfo(void)
{
    _componentInfoMap.clear();
    _componentInfoAllReceived = false;
    _componentInfoRequestRetryCount = 0;

    _sendComponentInfoRequest(MAV_COMP_ID_ALL);
    _componentInfoRequestTimer.start();
}


void ComponentManager::_componentInfoRequestTimerTimeout(void)
{
    if (_componentInfoAllReceived) {
        return;
    }

    if (++_componentInfoRequestRetryCount <= _componentInfoRequestRetryMax) {
        _sendComponentInfoRequest(MAV_COMP_ID_ALL);
        _componentInfoRequestTimer.start();
        qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION retry request";
    } else {
        qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION requesting stopped, receivedcount: " << _componentInfoMap.size();
    }
}


void ComponentManager::_sendComponentInfoRequest(int compId)
{
    if (_logReplay) {
        return;
    }

    mavlink_message_t msg;
    mavlink_msg_command_long_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                       static_cast<uint8_t>(_mavlink->getComponentId()),
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
    if (msg.sysid != _vehicle->id()) return; // not for us, we also ignore broadcast sysid's

    switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeatMessage(msg);
        break;
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

    // register new components
    if (!_componentIdList.contains(msg.compid)){
        _componentIdList.append(msg.compid);
        qCDebug(ComponentManagerLog) << "!!!!" << "New conmponent registerd, component ID:" << msg.compid;
    }
}


// handler for received COMPONENT_INFORMATION messages
void ComponentManager::_handleComponentInfoMessage(mavlink_message_t& msg)
{
    bool componentAlreadyDone = false;

    if (!_componentInfoMap.contains(msg.compid)) {
        ComponentInfo_t ci;
        ci.ok = false;
        _componentInfoMap[msg.compid] = ci;
    } else {
        componentAlreadyDone = true;
    }

    // check if all done
    _componentInfoAllReceived = true;
    for(int i = 0; i < _componentIdList.size(); i++) {
        int compId = _componentIdList[i];
        if (compId == MAV_COMP_ID_ALL) continue;
        if (!_componentInfoMap.contains(compId)) _componentInfoAllReceived = false;  //there is one
    }

    if (componentAlreadyDone) return;

    //TODO: for the moment we allow only one to be received and handled, we probably want a ComponentManager,ComponentController thing
    // so we just stop anything further

    _componentInfoAllReceived = true;

    mavlink_component_information_t compInfo;
    mavlink_msg_component_information_decode(&msg, &compInfo);

    // Construct a string stopping at the first NUL (0) character, else copy the whole byte array
    QByteArray urlBytes(compInfo.component_definition_uri, MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_COMPONENT_DEFINITION_URI_LEN);
    QString url(urlBytes);

    _componentInfoMap[msg.compid].firmware_version = compInfo.firmware_version;
    _componentInfoMap[msg.compid].hardware_version = compInfo.hardware_version;
    _componentInfoMap[msg.compid].capability_flags = compInfo.capability_flags;
    _componentInfoMap[msg.compid].firmwareVersion = QString("%1.%2.%3.%4").arg(compInfo.firmware_version & 0xFF)
                                                                          .arg((compInfo.firmware_version >> 8) & 0xFF)
                                                                          .arg((compInfo.firmware_version >> 16) & 0xFF)
                                                                          .arg((compInfo.firmware_version >> 24) & 0xFF);
    _componentInfoMap[msg.compid].hardwareVersion = QString("%1.%2.%3.%4").arg(compInfo.hardware_version & 0xFF)
                                                                          .arg((compInfo.hardware_version >> 8) & 0xFF)
                                                                          .arg((compInfo.hardware_version >> 16) & 0xFF)
                                                                          .arg((compInfo.hardware_version >> 24) & 0xFF);
    QByteArray vBytes(reinterpret_cast<char*>(compInfo.vendor_name), MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_VENDOR_NAME_LEN);
    _componentInfoMap[msg.compid].vendorName = QString(vBytes);
    QByteArray mBytes(reinterpret_cast<char*>(compInfo.model_name), MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_MODEL_NAME_LEN);
    _componentInfoMap[msg.compid].modelName = QString(mBytes);
    _componentInfoMap[msg.compid].definitionFileUri += url;

    _httpRequest(url);

    QString what = (msg.compid == MAV_COMP_ID_ALL) ? "MAV_COMP_ID_ALL" : QString::number(msg.compid);
    qCDebug(ComponentManagerLog) << "!!!!" << "COMPONENT_INFORMATION received for component ID:" << what;
    qCDebug(ComponentManagerLog) << "!!!!" << "    definition file:" << url;
    qCDebug(ComponentManagerLog) << "!!!!" << "    firmwareVersion:" << _componentInfoMap[msg.compid].firmwareVersion;
    qCDebug(ComponentManagerLog) << "!!!!" << "    hardwareVersion:" << _componentInfoMap[msg.compid].hardwareVersion;
    qCDebug(ComponentManagerLog) << "!!!!" << "    vendorName:" << _componentInfoMap[msg.compid].vendorName;
    qCDebug(ComponentManagerLog) << "!!!!" << "    modelName:" << _componentInfoMap[msg.compid].modelName;
}


void ComponentManager::_httpRequest(const QString& url)
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
    connect(reply, &QNetworkReply::finished, this, &ComponentManager::_httpDownloadFinished);
    _netManager->setProxy(savedProxy);
}


void ComponentManager::_httpDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (!reply) {
        return;
    }

    qCDebug(ComponentManagerLog) << "!!!!" << "Downloading component definition file finished";

    int err = reply->error();
    int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if (err == QNetworkReply::NoError && http_code == 200) {
        data.append("\n");
    } else {
        data.clear();
        qWarning() << "!!!!" << QString("Component definition file download error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        return;
    }

    if (data.size()) {
        _loadComponentDefinitionFile(data);
    } else {
        qCDebug(ComponentManagerLog) << "!!!!" << "Empty component definition file";
    }
}


static bool read_attribute_str(QDomNode& node, const char* tagName, QString& result)
{
    QDomNamedNodeMap attrs = node.attributes();
    if (!attrs.count()) return false;
    QDomNode subNode = attrs.namedItem(tagName);
    if (subNode.isNull()) return false;
    result = subNode.nodeValue();
    return true;
}

static bool read_attribute_int(QDomNode& node, const char* tagName, int& result)
{
    QDomNamedNodeMap attrs = node.attributes();
    if (!attrs.count()) return false;
    QDomNode subNode = attrs.namedItem(tagName);
    if (subNode.isNull()) return false;
    result = subNode.nodeValue().toInt();
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


bool ComponentManager::_loadComponentDefinitionFile(QByteArray& bytes)
{
    //fake, get the component Id
    int compId = _componentInfoMap.keys()[0];
    qCDebug(ComponentManagerLog) << "!!!!" << "Parse component definition file for component ID:" << compId;

    QByteArray originalData(bytes);
    QString errorMsg;
    int errorLine;
    QDomDocument doc;

    //-- Read it
    if(!doc.setContent(bytes, false, &errorMsg, &errorLine)) {
        qCDebug(ComponentManagerLog) << "!!!!" << "Unable to parse component definition file on line:" << errorLine;
        qCDebug(ComponentManagerLog) << "!!!!" << errorMsg;
        return false;
    }

    //-- Does it have groups?
    QDomNodeList groupElements = doc.elementsByTagName("group");
    if (!groupElements.size()) {
        qCDebug(ComponentManagerLog) << "!!!!" << "Unable to load component group elements from component definition file";
        return false;
    }

    //-- Does it have parameters?
    QDomNodeList parameterElements = doc.elementsByTagName("parameter");
    if (!parameterElements.size()) {
        qCDebug(ComponentManagerLog) << "!!!!" << "Unable to load component parameter elements from component definition file";
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
            continue; // it must be a member of a group
        }
        if (!read_attribute_str(parameterNode, "dispname", p.displayName)) {
            p.displayName = factName;
        }
        if (!read_attribute_int(parameterNode, "default", p.defaultValue)) {
            p.defaultValue = 0;
        }
        read_value_str(parameterNode, "short_desc", p.shortDescription);
        read_value_str(parameterNode, "long_desc", p.longDescription);
        read_value_str(parameterNode, "unit", p.unit);
        if (!read_value_int(parameterNode, "min", p.minValue)) {
            p.minValue = p.defaultValue;
        }
        if (!read_value_int(parameterNode, "max", p.maxValue)) {
            p.maxValue = p.defaultValue;
        }
        if (p.minValue > p.maxValue) {
            continue; // this doesn't make sense, we allow min = max however!
        }
        if (p.defaultValue < p.minValue || p.defaultValue > p.maxValue) {
            continue;
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

    //-- Be happy
    //TODO: error handling if something not ok in parsing
    _componentInfoMap[compId].ok = true;
    _componentInfoGroupMap[compId] = groupMap;
    _componentInfoParameterMap[compId] = parameterMap;

    //-- If this is new, cache it
    // TODO: we currently don't do chaing, but just save it for inspection
    QString _cacheFile = "C:/Users/Olli/Documents/GitHub/test.xml";
    bool _cached = false;
    if (!_cached) {
        QFile file(_cacheFile);
        if (!file.open(QIODevice::WriteOnly)) {
            qCDebug(ComponentManagerLog) << "!!!!" << QString("Could not save cache file %1. Error: %2").arg(_cacheFile).arg(file.errorString());
        } else {
            file.write(originalData);
        }
    }

    return true;
}


bool ComponentManager::componentInfoAvailable(int componentId)
{
    return _componentInfoMap.contains(componentId) && _componentInfoMap[componentId].ok;
}



