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
//#include <QVariantList>

//#include "QGCMAVLink.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"


Q_DECLARE_LOGGING_CATEGORY(ComponentManagerLog)


class ComponentManager : public QObject
{
    Q_OBJECT

public:
    ComponentManager(Vehicle* vehicle);
    ~ComponentManager();

    Vehicle* vehicle(void) { return _vehicle; }

    typedef struct {
        bool                        ok;
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
        QString                     shortDescription;
        QString                     longDescription;
        QString                     unit;
        int                         defaultValue;
        int                         minValue;
        int                         maxValue;
        int                         increment;
        int                         decimal;
        QMap<int,QString>           valuesMap;
    } ComponentParameterElement_t;

    QList<int>& getComponentIdList(void) { return _componentIdList; }
    bool componentInfoAvailable(int compId);
    QMap<int,ComponentInfo_t>& getComponentInfoMap(void) { return _componentInfoMap; }
    QMap<int,QMap<QString,ComponentParameterElement_t>>& getComponentInfoParameterMap(void) { return _componentInfoParameterMap; }

protected:
    Vehicle*            _vehicle;
    MAVLinkProtocol*    _mavlink;

    void _startRequestComponentInfo(void);
    void _componentInfoRequestTimerTimeout(void);

    void _sendComponentInfoRequest(int compId);
    void _mavlinkMessageReceived(mavlink_message_t msg);
    void _handleHeartbeatMessage(mavlink_message_t& msg);
    void _handleComponentInfoMessage(mavlink_message_t& msg);

    void _httpRequest(const QString& url);
    void _httpDownloadFinished();
    bool _loadComponentDefinitionFile(QByteArray& bytes);

private:
    bool                    _logReplay;

    QList<int>              _componentIdList;

    QTimer                  _componentInfoRequestTimer;
    static const int        _componentInfoRequestRetryMax = 4;
    int                     _componentInfoRequestRetryCount;
    bool                    _componentInfoAllReceived;
    QNetworkAccessManager*  _netManager = nullptr;

    QMap<int, ComponentInfo_t>                              _componentInfoMap;          // key = componentID
    QMap<int, QMap<QString, QStringList> >                  _componentInfoGroupMap;     // key = componentID
    QMap<int, QMap<QString, ComponentParameterElement_t> >  _componentInfoParameterMap; // key = componentID

    //TODO: SectionHeader in ParameterEditor.qml should adapt arrow for text row number
};
