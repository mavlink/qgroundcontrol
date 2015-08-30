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

#ifndef Vehicle_H
#define Vehicle_H

#include <QObject>
#include <QGeoCoordinate>

#include "LinkInterface.h"
#include "QGCMAVLink.h"
#include "UAS.h"

class FirmwarePlugin;
class AutoPilotPlugin;

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

class Vehicle : public QObject
{
    Q_OBJECT
    
public:
    Vehicle(LinkInterface* link, int vehicleId, MAV_AUTOPILOT firmwareType);
    
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(AutoPilotPlugin* autopilot MEMBER _autopilotPlugin CONSTANT)
    
    Q_PROPERTY(QGeoCoordinate coordinate MEMBER _geoCoordinate NOTIFY coordinateChanged)
    
    Q_PROPERTY(double heading MEMBER _heading NOTIFY headingChanged)
    
    // Property accesors
    int id(void) { return _id; }
    MAV_AUTOPILOT firmwareType(void) { return _firmwareType; }
    
    /// Sends this specified message to all links accociated with this vehicle
    void sendMessage(mavlink_message_t message);
    
    /// Provides access to uas from vehicle. Temporary workaround until UAS is fully phased out.
    UAS* uas(void) { return _uas; }
    
    /// Provides access to uas from vehicle. Temporary workaround until AutoPilotPlugin is fully phased out.
    AutoPilotPlugin* autopilotPlugin(void) { return _autopilotPlugin; }
    
    QList<LinkInterface*> links(void);
    
public slots:
    void setLatitude(double latitude);
    void setLongitude(double longitude);
    
signals:
    void allLinksDisconnected(void);
    void coordinateChanged(QGeoCoordinate coordinate);
    void headingChanged(double heading);
    
    /// Used internally to move sendMessage call to main thread
    void _sendMessageOnThread(mavlink_message_t message);
    
private slots:
    void _mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message);
    void _linkDisconnected(LinkInterface* link);
    void _sendMessage(mavlink_message_t message);
    void _setYaw(double yaw);
    
private:
    bool _containsLink(LinkInterface* link);
    void _addLink(LinkInterface* link);

    int             _id;            ///< Mavlink system id
    MAV_AUTOPILOT   _firmwareType;
    
    FirmwarePlugin*     _firmwarePlugin;
    AutoPilotPlugin*    _autopilotPlugin;
    
    /// List of all links associated with this vehicle. We keep SharedLinkInterface objects
    /// which are QSharedPointer's in order to maintain reference counts across threads.
    /// This way Link deletion works correctly.
    QList<SharedLinkInterface> _links;
    
    UAS* _uas;
    
    QGeoCoordinate  _geoCoordinate;
    
    double _heading;
};

#endif
