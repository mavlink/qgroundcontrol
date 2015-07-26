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

#ifndef MOCKLINK_H
#define MOCKLINK_H

#include <QMap>
#include <QLoggingCategory>

#include "MockLinkMissionItemHandler.h"
#include "MockLinkFileServer.h"
#include "LinkManager.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(MockLinkLog)

/// @file
///     @brief Mock implementation of a Link.
///
///     @author Don Gagne <don@thegagnes.com>

class MockConfiguration : public LinkConfiguration
{
public:

    MockConfiguration(const QString& name) : LinkConfiguration(name) {}
    MockConfiguration(MockConfiguration* source) : LinkConfiguration(source) {}
    int  type() { return LinkConfiguration::TypeMock; }
    void copyFrom(LinkConfiguration* source) { LinkConfiguration::copyFrom(source); }
    void loadSettings(QSettings& settings, const QString& root) { Q_UNUSED(settings); Q_UNUSED(root); }
    void saveSettings(QSettings& settings, const QString& root) { Q_UNUSED(settings); Q_UNUSED(root); }
    void updateSettings() {}
};

class MockLink : public LinkInterface
{
    Q_OBJECT

public:
    // LinkConfiguration is optional for MockLink
    MockLink(MockConfiguration* config = NULL);
    ~MockLink(void);

    // Virtuals from LinkInterface
    virtual QString getName(void) const { return _name; }
    virtual void requestReset(void){ }
    virtual bool isConnected(void) const { return _connected; }
    virtual qint64 getConnectionSpeed(void) const { return 100000000; }
    virtual qint64 bytesAvailable(void) { return 0; }

    // MockLink methods
    MAV_AUTOPILOT getAutopilotType(void) { return _autopilotType; }
    void setAutopilotType(MAV_AUTOPILOT autopilot) { _autopilotType = autopilot; }
    void emitRemoteControlChannelRawChanged(int channel, uint16_t raw);
    
    /// Sends the specified mavlink message to QGC
    void respondWithMavlinkMessage(const mavlink_message_t& msg);
    
    MockLinkFileServer* getFileServer(void) { return _fileServer; }

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

    LinkConfiguration* getLinkConfiguration() { return _config; }

signals:
    /// @brief Used internally to move data to the thread.
    void _incomingBytes(const QByteArray bytes);

public slots:
    virtual void writeBytes(const char *bytes, qint64 cBytes);

protected slots:
    // FIXME: This should not be part of LinkInterface. It is an internal link implementation detail.
    virtual void readBytes(void);

private slots:
    void _run1HzTasks(void);
    void _run10HzTasks(void);
    void _run50HzTasks(void);

private:
    // From LinkInterface
    virtual bool _connect(void);
    virtual bool _disconnect(void);

    // QThread override
    virtual void run(void);

    // MockLink methods
    void _sendHeartBeat(void);
    void _handleIncomingBytes(const QByteArray bytes);
    void _handleIncomingNSHBytes(const char* bytes, int cBytes);
    void _handleIncomingMavlinkBytes(const uint8_t* bytes, int cBytes);
    void _loadParams(void);
    void _handleHeartBeat(const mavlink_message_t& msg);
    void _handleSetMode(const mavlink_message_t& msg);
    void _handleParamRequestList(const mavlink_message_t& msg);
    void _handleParamSet(const mavlink_message_t& msg);
    void _handleParamRequestRead(const mavlink_message_t& msg);
    void _handleMissionRequestList(const mavlink_message_t& msg);
    void _handleMissionRequest(const mavlink_message_t& msg);
    void _handleMissionItem(const mavlink_message_t& msg);
    void _handleFTP(const mavlink_message_t& msg);
    float _floatUnionForParam(int componentId, const QString& paramName);
    void _setParamFloatUnionIntoMap(int componentId, const QString& paramName, float paramFloat);

    MockLinkMissionItemHandler* _missionItemHandler;

    QString _name;
    bool    _connected;

    uint8_t _vehicleSystemId;
    uint8_t _vehicleComponentId;

    bool    _inNSH;
    bool    _mavlinkStarted;

    QMap<int, QMap<QString, QVariant> > _mapParamName2Value;
    QMap<QString, MAV_PARAM_TYPE>       _mapParamName2MavParamType;

    typedef QMap<uint16_t, mavlink_mission_item_t>   MissionList_t;
    MissionList_t   _missionItems;

    uint8_t     _mavBaseMode;
    uint32_t    _mavCustomMode;
    uint8_t     _mavState;

    MockConfiguration* _config;
    MAV_AUTOPILOT _autopilotType;
    
    MockLinkFileServer* _fileServer;
};

#endif
