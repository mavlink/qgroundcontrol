/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Abstract interface, represents one unmanned aerial vehicle
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UASINTERFACE_H_
#define _UASINTERFACE_H_

#include <QObject>
#include <QList>
#include <QAction>
#include <QColor>

#include "LinkInterface.h"
#include "ProtocolInterface.h"
#include "Waypoint.h"

/**
 * @brief Interface for all robots.
 *
 * This interface is abstract and thus cannot be instantiated. It serves only as type definition.
 * It represents an unmanned aerial vehicle, e.g. a micro air vehicle.
 **/
class UASInterface : public QObject {
    Q_OBJECT
public:
    UASInterface() :
            color(UASInterface::getNextColor())
    {
    }
    virtual ~UASInterface() {}

    /* MANAGEMENT */

    /** @brief The name of the robot **/
    virtual QString getUASName() = 0;
    //virtual QColor getColor() = 0;
    virtual int getUASID() = 0; ///< Get the ID of the connected UAS
    /** @brief The time interval the robot is switched on **/
    virtual quint64 getUptime() = 0;
    /** @brief Get the status flag for the communication **/
    virtual int getCommunicationStatus() = 0;

    /* COMMUNICATION FLAGS */

    enum CommStatus {
        /** Unit is disconnected, no failure state reached so far **/
        COMM_DISCONNECTED = 0,
        /** The communication is established **/
        COMM_CONNECTING = 1,
        /** The communication link is up **/
        COMM_CONNECTED = 2,
        /** The connection is closed **/
        COMM_DISCONNECTING = 3,
        COMM_FAIL = 4, ///< Communication link failed
    };



    /**
         * @brief Get the links associated with this robot
         *
         * @return List with all links this robot is associated with. Association is usually
         *         based on the fact that a message for this robot has been received through that
         *         interface. The LinkInterface can support multiple protocols.
         **/
    virtual QList<LinkInterface*>* getLinks() = 0;

    /**
     * @brief Get the color for this UAS
     *
     * This static function holds a color map that allows to draw a new color for each robot
     *
     * @return The next color in the color map. The map holds 20 colors and starts from the beginning
     *         if the colors are exceeded.
     */
    static QColor getNextColor()
    {
        /* Create color map */
        static QList<QColor> colors = QList<QColor>();
        static int nextColor = -1;

        if (nextColor == -1)
        {
            ///> Color map for plots, includes 20 colors
            ///> Map will start from beginning when the first 20 colors are exceeded
            colors.append(QColor(242,255,128));
            colors.append(QColor(230,126,23));
            colors.append(QColor(203,254,121));
            colors.append(QColor(231,72,28));
            colors.append(QColor(161,252,116));
            colors.append(QColor(232,33,47));
            colors.append(QColor(116,251,110));
            colors.append(QColor(234,38,107));
            colors.append(QColor(104,250,138));
            colors.append(QColor(235,43,165));
            colors.append(QColor(98,248,176));
            colors.append(QColor(236,48,221));
            colors.append(QColor(92,247,217));
            colors.append(QColor(200,54,238));
            colors.append(QColor(87,231,246));
            colors.append(QColor(151,59,239));
            colors.append(QColor(81,183,244));
            colors.append(QColor(104,64,240));
            colors.append(QColor(75,133,243));
            nextColor++;
        }
        return colors[nextColor++];
    }

    QColor getColor()
    {
        return color;
    }

public slots:

    /** @brief Launches the system/Liftof **/
    virtual void launch() = 0;
    /** @brief Set a new waypoint **/
    virtual void setWaypoint(Waypoint* wp) = 0;
    /** @brief Set this waypoint as next waypoint to fly to */
    virtual void setWaypointActive(int wp) = 0;
    /** @brief Order the robot to return home / to land on the runway **/
    virtual void home() = 0;
    /** @brief Halt the system */
    virtual void halt() = 0;
    /** @brief Start/continue the current robot action */
    virtual void go() = 0;
    /** @brief Set robot into auto mode
      *
      * @param autoMode true for autonomous operation, false for manual control
      */
    virtual void setAutoMode(bool autoMode) = 0;

    virtual void setMode(int mode) = 0;
    /** Stops the robot system. If it is an MAV, the robot starts the emergency landing procedure **/
    virtual void emergencySTOP() = 0;
    /** Kills the robot. All systems are immediately shut down (e.g. the main power line is cut). This might lead to a crash **/
    virtual bool emergencyKILL() = 0;
    /**
     * @brief Shut down the system's computers
     *
     * Works only if already landed and will cleanly shut down all onboard computers.
     */
    virtual void shutdown() = 0;
    /**
     * @brief Request the list of stored waypoints from the robot
     */
    virtual void requestWaypoints() = 0;
    /**
     * @brief Clear all existing waypoints on the robot
     */
    virtual void clearWaypointList() = 0;

    /**
     * @brief Add a link to the list of current links
     *
     * Adding the link to the robot's internal link list will allow him so send its own messages
     * over all registered links. Usually a link is added once a message for this particular robot
     * has been received over a link, but adding could also be done manually.
     * @warning Not all links should be added to all robots by default, because else all robots will
     *          attempt to send over all links, typically choking wireless serial links.
     */
    virtual void addLink(LinkInterface* link) = 0;

    /**
     * @brief Set the current robot as focused in the user interface
     */
    virtual void setSelected() = 0;
    // TODO EMAV REMOVE
    //virtual void sendMessage(LinkInterface* link, mavlink_message_t message) = 0;
    //virtual void sendMessage(mavlink_message_t message) = 0;

protected:
    QColor color;

signals:
    /** @brief The robot state has changed **/
    void statusChanged(int stateFlag);
    /** @brief The robot state has changed
     *
     * @param uas this robot
     * @param status short description of status, e.g. "connected"
     * @param description longer textual description. Should be however limited to a short text, e.g. 200 chars.
     */
    void statusChanged(UASInterface* uas, QString status, QString description);
    /** @brief Robot mode has changed */
    void modeChanged(int sysId, QString status, QString description);
    /** @brief A command has been issued **/
    void commandSent(int command);
    /** @brief The connection status has changed **/
    void connectionChanged(CommStatus connectionFlag);
    /** @brief The robot is connecting **/
    void connecting();
    /** @brief The robot is connected **/
    void connected();
    /** @brief The robot is disconnected **/
    void disconnected();
    /** @brief The robot is active **/
    void activated();
    /** @brief The robot is inactive **/
    void deactivated();
    /** @brief The robot is manually controlled **/
    void manualControl();
    /** @brief A value of the robot has changed.
      *
      * Typically this is used to send lowlevel information like the battery voltage to the plotting facilities of
      * the groundstation
      *
      * @param uasId ID of this system
      * @param name name of the value, e.g. "battery voltage"
      * @param value the value that changed
      * @param msec the timestamp of the message, in milliseconds
      */
    void valueChanged(int uasId, QString name, double value, quint64 msec);
    void valueChanged(UASInterface* uas, QString name, double value, quint64 msec);
    void voltageChanged(int uasId, double voltage);
    void waypointUpdated(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool active);
    void waypointSelected(int uasId, int id);
    void waypointReached(UASInterface* uas, int id);
    void autoModeChanged(bool autoMode);
    void detectionReceived(int uasId, QString patternPath, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, double confidence, bool detected);
    /**
     * @brief The battery status has been updated
     *
     * @param uas sending system
     * @param voltage battery voltage
     * @param percent remaining capacity in percent
     * @param seconds estimated remaining flight time in seconds
     */
    void batteryChanged(UASInterface* uas, double voltage, double percent, int seconds);
    void statusChanged(UASInterface* uas, QString status);
    void actuatorChanged(UASInterface*, int actId, double value);
    void thrustChanged(UASInterface*, double thrust);
    void heartbeat(UASInterface* uas);
    void attitudeChanged(UASInterface*, double roll, double pitch, double yaw, quint64 usec);
    void attitudeThrustSetPointChanged(UASInterface*, double rollDesired, double pitchDesired, double yawDesired, double thrustDesired, quint64 usec);
    void localPositionChanged(UASInterface*, double x, double y, double z, quint64 usec);
    void globalPositionChanged(UASInterface*, double lon, double lat, double alt, quint64 usec);
    void speedChanged(UASInterface*, double x, double y, double z, quint64 usec);
    void imageStarted(int imgid, int width, int height, int depth, int channels);
    void imageDataReceived(int imgid, const unsigned char* imageData, int length, int startIndex);
    /** @brief Emit the new system type */
    void systemTypeSet(UASInterface* uas, unsigned int type);
};

#endif // _UASINTERFACE_H_
