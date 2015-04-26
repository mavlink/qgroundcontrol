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

#ifndef MOCKUAS_H
#define MOCKUAS_H

#include "UASInterface.h"
#include "MockQGCUASParamManager.h"
#include "MockMavlinkInterface.h"

#include <limits>

/// @file
///     @brief This is a mock implementation of a UAS used for writing Unit Tests. Normal usage is to
///         call MockUASManager::setMockActiveUAS to set it to the active mock UAS>
///
///     @author Don Gagne <don@thegagnes.com>

class MockUAS : public UASInterface
{
    Q_OBJECT

signals:
    // The following UASInterface signals are supported
    //  void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    //  void remoteControlChannelRawChanged(int channelId, float raw);

public:
    // Implemented UASInterface overrides
    virtual int getSystemType(void) { return _systemType; }
    virtual int getUASID(void) const { return _systemId; }
    virtual bool isAirplane() { return false; }
    virtual QGCUASParamManagerInterface* getParamManager() { return &_paramManager; };

    // sendMessage is only supported if a mavlink plugin is installed.
    virtual void sendMessage(mavlink_message_t message);

public:
    // MockUAS methods
    MockUAS(void);

    // Use these methods to setup/control the mock UAS

    void setMockSystemType(int systemType) { _systemType = systemType; }
    void setMockSystemId(int systemId) { _systemId = systemId; }

    /// @return returns mock QGCUASParamManager associated with the UAS. This mock implementation
    /// allows you to simulate parameter input and validate parameter setting
    MockQGCUASParamManager* getMockQGCUASParamManager(void) { return &_paramManager; }

    /// @brief Sets the parameter map into the mock QGCUASParamManager and signals parameterChanged for
    /// each param
    void setMockParametersAndSignal(MockQGCUASParamManager::ParamMap_t& map);

    void emitRemoteControlChannelRawChanged(int channel, float raw) { emit remoteControlChannelRawChanged(channel, raw); }

    /// @brief Installs a mavlink plugin. Only a single mavlink plugin is supported at a time.
    void setMockMavlinkPlugin(MockMavlinkInterface* mavlinkPlugin) { _mavlinkPlugin = mavlinkPlugin; };

public:
    // Unimplemented UASInterface overrides
    virtual QString getUASName() const { Q_ASSERT(false); return _bogusString; };
    virtual const QString& getShortState() const { Q_ASSERT(false); return _bogusString; };
    virtual const QString& getShortMode() const { Q_ASSERT(false); return _bogusString; };
    virtual QString getShortModeTextFor(uint8_t base_mode, uint32_t custom_mode) const { Q_UNUSED(base_mode); Q_UNUSED(custom_mode); Q_ASSERT(false); return _bogusStaticString; };
    virtual quint64 getUptime() const { Q_ASSERT(false); return 0; };
    virtual double getLocalX() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getLocalY() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getLocalZ() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual bool localPositionKnown() const { Q_ASSERT(false); return false; };
    virtual double getLatitude() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getLongitude() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getAltitudeAMSL() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getAltitudeRelative() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual bool globalPositionKnown() const { Q_ASSERT(false); return false; };
    virtual double getRoll() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getPitch() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual double getYaw() const { Q_ASSERT(false); return std::numeric_limits<double>::quiet_NaN(); };
    virtual bool getSelected() const { Q_ASSERT(false); return false; };
    virtual bool isArmed() const { Q_ASSERT(false); return false; };
    virtual int getAirframe() const { Q_ASSERT(false); return 0; };
    virtual UASWaypointManager* getWaypointManager(void) { Q_ASSERT(false); return NULL; };
    virtual QList<LinkInterface*> getLinks() { Q_ASSERT(false); return QList<LinkInterface*>(); };
    virtual bool systemCanReverse() const { Q_ASSERT(false); return false; };
    virtual QString getSystemTypeName() { Q_ASSERT(false); return _bogusString; };
    virtual int getAutopilotType() { return MAV_AUTOPILOT_PX4; };
    virtual QGCUASFileManager* getFileManager() {Q_ASSERT(false); return NULL; }
    virtual void startCalibration(StartCalibrationType calType) { Q_UNUSED(calType); return; };
    virtual void stopCalibration() { return; };

    /** @brief Send a message over this link (to this or to all UAS on this link) */
    virtual void sendMessage(LinkInterface* link, mavlink_message_t message){ Q_UNUSED(link); Q_UNUSED(message); Q_ASSERT(false); }
    virtual QString getAutopilotTypeName() { Q_ASSERT(false); return _bogusString; };
    virtual void setAutopilotType(int apType) { Q_UNUSED(apType); Q_ASSERT(false); };
    virtual QMap<int, QString> getComponents() { Q_ASSERT(false); return _bogusMapIntQString; };
    virtual QList<QAction*> getActions() const { Q_ASSERT(false); return _bogusQListQActionPointer; };

public slots:
    // Unimplemented UASInterface overrides
    virtual void setUASName(const QString& name) { Q_UNUSED(name); Q_ASSERT(false); };
    virtual void executeCommand(MAV_CMD command) { Q_UNUSED(command); Q_ASSERT(false); };
    virtual void executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7, int component) { Q_UNUSED(command); Q_UNUSED(confirmation); Q_UNUSED(param1); Q_UNUSED(param2); Q_UNUSED(param3); Q_UNUSED(param4); Q_UNUSED(param5); Q_UNUSED(param6); Q_UNUSED(param7); Q_UNUSED(component); Q_ASSERT(false); };
    virtual void executeCommandAck(int num, bool success) { Q_UNUSED(num); Q_UNUSED(success); Q_ASSERT(false); };
    virtual void setAirframe(int airframe) { Q_UNUSED(airframe); Q_ASSERT(false); };
    virtual void launch() { Q_ASSERT(false); };
    virtual void home() { Q_ASSERT(false); };
    virtual void land() { Q_ASSERT(false); };
    virtual void pairRX(int rxType, int rxSubType) { Q_UNUSED(rxType); Q_UNUSED(rxSubType); Q_ASSERT(false); };
    virtual void halt() { Q_ASSERT(false); };
    virtual void go() { Q_ASSERT(false); };
    virtual void setMode(uint8_t newBaseMode, uint32_t newCustomMode) { Q_UNUSED(newBaseMode); Q_UNUSED(newCustomMode); Q_ASSERT(false); };
    virtual void emergencySTOP() { Q_ASSERT(false); };
    virtual bool emergencyKILL() { Q_ASSERT(false); return false; };
    virtual void shutdown() { Q_ASSERT(false); };
    virtual void setTargetPosition(float x, float y, float z, float yaw) { Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); Q_UNUSED(yaw); Q_ASSERT(false); };
    virtual void setLocalOriginAtCurrentGPSPosition() { Q_ASSERT(false); };
    virtual void setHomePosition(double lat, double lon, double alt) { Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); Q_ASSERT(false); };
    virtual void requestParameter(int component, const QString& parameter) { Q_UNUSED(component); Q_UNUSED(parameter); Q_ASSERT(false); };
    virtual void writeParametersToStorage() { Q_ASSERT(false); };
    virtual void readParametersFromStorage() { Q_ASSERT(false); };
    virtual void setParameter(const int component, const QString& id, const QVariant& value)
        { Q_UNUSED(component); Q_UNUSED(id); Q_UNUSED(value); Q_ASSERT(false); };
    virtual void addLink(LinkInterface* link) { Q_UNUSED(link); Q_ASSERT(false); };
    virtual void setSelected() { Q_ASSERT(false); }
    virtual void enableAllDataTransmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableRawSensorDataTransmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableExtendedSystemStatusTransmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableRCChannelDataTransmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableRawControllerDataTransmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enablePositionTransmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableExtra1Transmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableExtra2Transmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void enableExtra3Transmission(int rate) { Q_UNUSED(rate); Q_ASSERT(false); };
    virtual void setLocalPositionSetpoint(float x, float y, float z, float yaw)
        { Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); Q_UNUSED(yaw); Q_ASSERT(false); };
    virtual void setLocalPositionOffset(float x, float y, float z, float yaw) { Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); Q_UNUSED(yaw); Q_ASSERT(false); };
    virtual void setBatterySpecs(const QString& specs) { Q_UNUSED(specs); Q_ASSERT(false); };
    virtual QString getBatterySpecs() { Q_ASSERT(false); return _bogusString; };
    virtual void sendHilState(quint64 time_us, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, double lat, double lon, double alt, float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc)
        { Q_UNUSED(time_us); Q_UNUSED(roll); Q_UNUSED(pitch); Q_UNUSED(yaw); Q_UNUSED(rollspeed); Q_UNUSED(pitchspeed); Q_UNUSED(yawspeed); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); Q_UNUSED(vx); Q_UNUSED(vy); Q_UNUSED(vz); Q_UNUSED(ind_airspeed); Q_UNUSED(true_airspeed); Q_UNUSED(xacc); Q_UNUSED(yacc); Q_UNUSED(zacc); Q_ASSERT(false); };
    virtual void sendHilSensors(quint64 time_us, float xacc, float yacc, float zacc, float rollspeed, float pitchspeed, float yawspeed, float xmag, float ymag, float zmag, float abs_pressure, float diff_pressure, float pressure_alt, float temperature, quint32 fields_changed)
        { Q_UNUSED(time_us); Q_UNUSED(xacc); Q_UNUSED(yacc); Q_UNUSED(zacc); Q_UNUSED(rollspeed); Q_UNUSED(pitchspeed); Q_UNUSED(yawspeed); Q_UNUSED(xmag); Q_UNUSED(ymag); Q_UNUSED(zmag); Q_UNUSED(abs_pressure); Q_UNUSED(diff_pressure); Q_UNUSED(pressure_alt); Q_UNUSED(temperature); Q_UNUSED(fields_changed); Q_ASSERT(false); };
    virtual void sendHilGps(quint64 time_us, double lat, double lon, double alt, int fix_type, float eph, float epv, float vel, float vn, float ve, float vd, float cog, int satellites)
        { Q_UNUSED(time_us); Q_UNUSED(lat); Q_UNUSED(lon); Q_UNUSED(alt); Q_UNUSED(fix_type); Q_UNUSED(eph); Q_UNUSED(epv); Q_UNUSED(vel); Q_UNUSED(vn); Q_UNUSED(ve); Q_UNUSED(vd); Q_UNUSED(cog); Q_UNUSED(satellites); Q_ASSERT(false); };
    virtual void sendHilOpticalFlow(quint64 time_us, qint16 flow_x, qint16 flow_y, float flow_comp_m_x,
                            float flow_comp_m_y, quint8 quality, float ground_distance)
    { Q_UNUSED(time_us); Q_UNUSED(flow_x); Q_UNUSED(flow_y); Q_UNUSED(flow_comp_m_x); Q_UNUSED(flow_comp_m_y); Q_UNUSED(quality); Q_UNUSED(ground_distance); Q_ASSERT(false);};
    virtual void sendMapRCToParam(QString param_id, float scale, float value0, quint8 param_rc_channel_index, float valueMin, float valueMax)
    { Q_UNUSED(param_id); Q_UNUSED(scale); Q_UNUSED(value0); Q_UNUSED(param_rc_channel_index); Q_UNUSED(valueMin); Q_UNUSED(valueMax); }
    virtual void unsetRCToParameterMap() {}

    virtual bool isRotaryWing() { Q_ASSERT(false); return false; }
    virtual bool isFixedWing() { Q_ASSERT(false); return false; }

private:
    int                 _systemType;
    int                 _systemId;

    MockQGCUASParamManager _paramManager;

    MockMavlinkInterface* _mavlinkPlugin;   ///< Mock Mavlink plugin, NULL for none

    // Bogus variables used for return types of NYI methods
    QString             _bogusString;
    static QString      _bogusStaticString;
    QMap<int, QString>  _bogusMapIntQString;
    QList<QAction*>     _bogusQListQActionPointer;
    QList<int>          _bogusQListInt;
};

#endif
