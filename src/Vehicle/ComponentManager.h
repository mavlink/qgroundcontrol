/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>

#include "Vehicle.h"
#include "QGCApplication.h"


Q_DECLARE_LOGGING_CATEGORY(ComponentManagerLog)


class ComponentControl : public QObject
{
    Q_OBJECT

public:
    ComponentControl(const mavlink_component_information_t* compInfo, Vehicle* vehicle, int compId);
    ~ComponentControl();

    Vehicle* vehicle(void) { return _vehicle; }

    typedef struct {
        uint32_t                    firmware_version; //raw value as in MAVLink message
        uint32_t                    hardware_version; //raw value as in MAVLink message
        uint32_t                    capability_flags; //raw value as in MAVLink message
        QString                     firmwareVersion;
        QString                     hardwareVersion;
        QString                     vendorName;
        QString                     modelName;
        QString                     definitionFileUri;
    } ComponentInfo_t;

    typedef struct {
        QString                     group;
        FactMetaData::ValueType_t   factType;
        QString                     displayName;
        bool                        hide;
        QString                     shortDescription;
        QString                     longDescription;
        bool                        rebootRequired;
        bool                        readOnly;
        QString                     unit;
        bool                        defaultValueAvailable;
        int                         defaultValue; //TODO: these may also be float
        int                         minValue;
        int                         maxValue;
        int                         increment;
        int                         decimal;
        QMap<int,QString>           valuesMap;
    } ComponentParameterElement_t;

    bool componentInfoAvailable(void) {return _ok; }
    ComponentInfo_t& getComponentInfo(void) { return _componentInfo; }
    QMap<QString,ComponentParameterElement_t>& getComponentInfoParameterMap(void) { return _componentInfoParameterMap; }

protected:
    Vehicle*                        _vehicle;

    void _httpRequest(const QString& url);
    void _httpDownloadFinished();
    bool _loadComponentDefinitionFile(QByteArray& bytes);

private:
    int                             _compId;
    QNetworkAccessManager*          _netManager;

    bool                            _ok;

    ComponentInfo_t                             _componentInfo;
    QMap<QString, QStringList>                  _componentInfoGroupMap;
    QMap<QString, ComponentParameterElement_t>  _componentInfoParameterMap;
};


class ComponentManager : public QObject
{
    Q_OBJECT

public:
    ComponentManager(Vehicle* vehicle);
    ~ComponentManager();

    Vehicle* vehicle(void) { return _vehicle; }

    QList<int>& getComponentIdList(void) { return _componentIdList; }
    bool componentInfoAvailable(int compId) { return _componentControlMap.keys().contains(compId); }
    ComponentControl::ComponentInfo_t& getComponentInfoMap(int compId) {
        //if (!componentInfoAvailable(compId)) return nullptr;
        return _componentControlMap[compId]->getComponentInfo();
    }
    QMap<QString,ComponentControl::ComponentParameterElement_t>& getComponentInfoParameterMap(int compId) {
        //if (!componentInfoAvailable(compId)) return nullptr;
        return _componentControlMap[compId]->getComponentInfoParameterMap();
    }

    void stoptRequestComponentInfo(void);

protected:
    Vehicle*            _vehicle;

    void _startRequestComponentInfo(void);
    void _componentInfoRequestTimerTimeout(void);

    void _sendComponentInfoRequest(int compId);
    void _mavlinkMessageReceived(mavlink_message_t msg);
    void _handleHeartbeatMessage(mavlink_message_t& msg);
    void _handleComponentInfoMessage(mavlink_message_t& msg);

private:
    bool                    _logReplay;

    QList<int>              _componentIdList;

    QTimer                  _componentInfoRequestTimer;
    static const int        _componentInfoRequestRetryMax = 4;
    int                     _componentInfoRequestRetryCount;

    QMap<int, ComponentControl*> _componentControlMap;

    //TODO: SectionHeader in ParameterEditor.qml should adapt arrow to number of text rows
    //TODO: should we do as for the camera manager and do the request individually in component controller?
    //TODO: could/should we use the vehicle function for calling the CMD?
};


