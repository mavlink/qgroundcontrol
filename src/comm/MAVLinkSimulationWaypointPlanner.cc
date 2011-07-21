/*======================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009-2011 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

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

========================================================================*/

/**
*   @file
*   @brief a program to manage waypoints and exchange them with the ground station
*
*   @author Petri Tanskanen <petri.tanskanen@inf.ethz.ch>
*   @author Benjamin Knecht <bknecht@student.ethz.ch>
*   @author Christian Schluchter <schluchc@ee.ethz.ch>
*/

#include <cmath>

#include "MAVLinkSimulationWaypointPlanner.h"
#include "QGC.h"
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class PxMatrix3x3;


/**
 * @brief Pixhawk 3D vector class, can be cast to a local OpenCV CvMat.
 *
 */
class PxVector3
{
public:
    /** @brief standard constructor */
    PxVector3(void) {}
    /** @brief copy constructor */
    PxVector3(const PxVector3 &v) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = v.m_vec[i];
        }
    }
    /** @brief x,y,z constructor */
    PxVector3(const float _x, const float _y, const float _z) {
        m_vec[0] = _x;
        m_vec[1] = _y;
        m_vec[2] = _z;
    }
    /** @brief broadcast constructor */
    PxVector3(const float _f) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = _f;
        }
    }

private:
    /** @brief private constructor (not used here, for SSE compatibility) */
    PxVector3(const float (&_vec)[3]) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = _vec[i];
        }
    }

public:
    /** @brief assignment operator */
    void operator= (const PxVector3 &r) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = r.m_vec[i];
        }
    }
    /** @brief const element access */
    float operator[] (const int i) const {
        return m_vec[i];
    }
    /** @brief element access */
    float &operator[] (const int i) {
        return m_vec[i];
    }

    // === arithmetic operators ===
    /** @brief element-wise negation */
    friend PxVector3 operator- (const PxVector3 &v) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = -v.m_vec[i];
        }
        return ret;
    }
    friend PxVector3 operator+ (const PxVector3 &l, const PxVector3 &r) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] + r.m_vec[i];
        }
        return ret;
    }
    friend PxVector3 operator- (const PxVector3 &l, const PxVector3 &r) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] - r.m_vec[i];
        }
        return ret;
    }
    friend PxVector3 operator* (const PxVector3 &l, const PxVector3 &r) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] * r.m_vec[i];
        }
        return ret;
    }
    friend PxVector3 operator/ (const PxVector3 &l, const PxVector3 &r) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] / r.m_vec[i];
        }
        return ret;
    }

    friend void operator+= (PxVector3 &l, const PxVector3 &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] + r.m_vec[i];
        }
    }
    friend void operator-= (PxVector3 &l, const PxVector3 &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] - r.m_vec[i];
        }
    }
    friend void operator*= (PxVector3 &l, const PxVector3 &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] * r.m_vec[i];
        }
    }
    friend void operator/= (PxVector3 &l, const PxVector3 &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] / r.m_vec[i];
        }
    }

    friend PxVector3 operator+ (const PxVector3 &l, float f) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] + f;
        }
        return ret;
    }
    friend PxVector3 operator- (const PxVector3 &l, float f) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] - f;
        }
        return ret;
    }
    friend PxVector3 operator* (const PxVector3 &l, float f) {
        PxVector3 ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] * f;
        }
        return ret;
    }
    friend PxVector3 operator/ (const PxVector3 &l, float f) {
        PxVector3 ret;
        float inv = 1.f/f;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] * inv;
        }
        return ret;
    }

    friend void operator+= (PxVector3 &l, float f) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] + f;
        }
    }
    friend void operator-= (PxVector3 &l, float f) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] - f;
        }
    }
    friend void operator*= (PxVector3 &l, float f) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] * f;
        }
    }
    friend void operator/= (PxVector3 &l, float f) {
        float inv = 1.f/f;
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] * inv;
        }
    }

    // === vector operators ===
    /** @brief dot product */
    float	dot(const PxVector3 &v) const {
        return m_vec[0]*v.m_vec[0] + m_vec[1]*v.m_vec[1] + m_vec[2]*v.m_vec[2];
    }
    /** @brief length squared of the vector */
    float	lengthSquared(void) const {
        return m_vec[0]*m_vec[0] + m_vec[1]*m_vec[1] + m_vec[2]*m_vec[2];
    }
    /** @brief length of the vector */
    float	length(void) const {
        return sqrt(lengthSquared());
    }
    /** @brief cross product */
    PxVector3 cross(const PxVector3 &v) const {
        return PxVector3(m_vec[1]*v.m_vec[2] - m_vec[2]*v.m_vec[1], m_vec[2]*v.m_vec[0] - m_vec[0]*v.m_vec[2], m_vec[0]*v.m_vec[1] - m_vec[1]*v.m_vec[0]);
    }
    /** @brief normalizes the vector */
    PxVector3 &normalize(void) {
        const float l = 1.f / length();
        for (int i=0; i < 3; i++) {
            m_vec[i] *= l;
        }
        return *this;
    }

    friend class PxMatrix3x3;
protected:
    float m_vec[3];
};

/**
 * @brief Pixhawk 3D vector class in double precision, can be cast to a local OpenCV CvMat.
 *
 */
class PxVector3Double
{
public:
    /** @brief standard constructor */
    PxVector3Double(void) {}
    /** @brief copy constructor */
    PxVector3Double(const PxVector3Double &v) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = v.m_vec[i];
        }
    }
    /** @brief x,y,z constructor */
    PxVector3Double(const double _x, const double _y, const double _z) {
        m_vec[0] = _x;
        m_vec[1] = _y;
        m_vec[2] = _z;
    }
    /** @brief broadcast constructor */
    PxVector3Double(const double _f) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = _f;
        }
    }

private:
    /** @brief private constructor (not used here, for SSE compatibility) */
    PxVector3Double(const double (&_vec)[3]) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = _vec[i];
        }
    }

public:
    /** @brief assignment operator */
    void operator= (const PxVector3Double &r) {
        for (int i=0; i < 3; i++) {
            m_vec[i] = r.m_vec[i];
        }
    }
    /** @brief const element access */
    double operator[] (const int i) const {
        return m_vec[i];
    }
    /** @brief element access */
    double &operator[] (const int i) {
        return m_vec[i];
    }

    // === arithmetic operators ===
    /** @brief element-wise negation */
    friend PxVector3Double operator- (const PxVector3Double &v) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = -v.m_vec[i];
        }
        return ret;
    }
    friend PxVector3Double operator+ (const PxVector3Double &l, const PxVector3Double &r) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] + r.m_vec[i];
        }
        return ret;
    }
    friend PxVector3Double operator- (const PxVector3Double &l, const PxVector3Double &r) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] - r.m_vec[i];
        }
        return ret;
    }
    friend PxVector3Double operator* (const PxVector3Double &l, const PxVector3Double &r) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] * r.m_vec[i];
        }
        return ret;
    }
    friend PxVector3Double operator/ (const PxVector3Double &l, const PxVector3Double &r) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] / r.m_vec[i];
        }
        return ret;
    }

    friend void operator+= (PxVector3Double &l, const PxVector3Double &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] + r.m_vec[i];
        }
    }
    friend void operator-= (PxVector3Double &l, const PxVector3Double &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] - r.m_vec[i];
        }
    }
    friend void operator*= (PxVector3Double &l, const PxVector3Double &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] * r.m_vec[i];
        }
    }
    friend void operator/= (PxVector3Double &l, const PxVector3Double &r) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] / r.m_vec[i];
        }
    }

    friend PxVector3Double operator+ (const PxVector3Double &l, double f) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] + f;
        }
        return ret;
    }
    friend PxVector3Double operator- (const PxVector3Double &l, double f) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] - f;
        }
        return ret;
    }
    friend PxVector3Double operator* (const PxVector3Double &l, double f) {
        PxVector3Double ret;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] * f;
        }
        return ret;
    }
    friend PxVector3Double operator/ (const PxVector3Double &l, double f) {
        PxVector3Double ret;
        double inv = 1.f/f;
        for (int i=0; i < 3; i++) {
            ret.m_vec[i] = l.m_vec[i] * inv;
        }
        return ret;
    }

    friend void operator+= (PxVector3Double &l, double f) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] + f;
        }
    }
    friend void operator-= (PxVector3Double &l, double f) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] - f;
        }
    }
    friend void operator*= (PxVector3Double &l, double f) {
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] * f;
        }
    }
    friend void operator/= (PxVector3Double &l, double f) {
        double inv = 1.f/f;
        for (int i=0; i < 3; i++) {
            l.m_vec[i] = l.m_vec[i] * inv;
        }
    }

    // === vector operators ===
    /** @brief dot product */
    double	dot(const PxVector3Double &v) const {
        return m_vec[0]*v.m_vec[0] + m_vec[1]*v.m_vec[1] + m_vec[2]*v.m_vec[2];
    }
    /** @brief length squared of the vector */
    double	lengthSquared(void) const {
        return m_vec[0]*m_vec[0] + m_vec[1]*m_vec[1] + m_vec[2]*m_vec[2];
    }
    /** @brief length of the vector */
    double	length(void) const {
        return sqrt(lengthSquared());
    }
    /** @brief cross product */
    PxVector3Double cross(const PxVector3Double &v) const {
        return PxVector3Double(m_vec[1]*v.m_vec[2] - m_vec[2]*v.m_vec[1], m_vec[2]*v.m_vec[0] - m_vec[0]*v.m_vec[2], m_vec[0]*v.m_vec[1] - m_vec[1]*v.m_vec[0]);
    }
    /** @brief normalizes the vector */
    PxVector3Double &normalize(void) {
        const double l = 1.f / length();
        for (int i=0; i < 3; i++) {
            m_vec[i] *= l;
        }
        return *this;
    }

    friend class PxMatrix3x3;
protected:
    double m_vec[3];
};

MAVLinkSimulationWaypointPlanner::MAVLinkSimulationWaypointPlanner(MAVLinkSimulationLink *parent, int sysid) :
    QObject(parent),
    link(parent),
    idle(false),
    current_active_wp_id(-1),
    timestamp_lastoutside_orbit(0),
    timestamp_firstinside_orbit(0),
    waypoints(&waypoints1),
    waypoints_receive_buffer(&waypoints2),
    current_state(PX_WPP_IDLE),
    protocol_current_wp_id(0),
    protocol_current_count(0),
    protocol_current_partner_systemid(0),
    protocol_current_partner_compid(0),
    protocol_timestamp_lastaction(0),
    protocol_timeout(1000),
    timestamp_last_send_setpoint(0),
    systemid(sysid),
    compid(MAV_COMP_ID_WAYPOINTPLANNER),
    setpointDelay(10),
    yawTolerance(0.4f),
    verbose(true),
    debug(false),
    silent(false)
{
    connect(parent, SIGNAL(messageReceived(mavlink_message_t)), this, SLOT(handleMessage(mavlink_message_t)));
    qDebug() << "PLANNER FOR SYSTEM" << systemid << "INITIALIZED";
}



/*
*  @brief Sends an waypoint ack message
*/
void MAVLinkSimulationWaypointPlanner::send_waypoint_ack(uint8_t target_systemid, uint8_t target_compid, uint8_t type)
{
    mavlink_message_t msg;
    mavlink_waypoint_ack_t wpa;

    wpa.target_system = target_systemid;
    wpa.target_component = target_compid;
    wpa.type = type;

    mavlink_msg_waypoint_ack_encode(systemid, compid, &msg, &wpa);
    link->sendMAVLinkMessage(&msg);



    if (verbose) qDebug("Sent waypoint ack (%u) to ID %u\n", wpa.type, wpa.target_system);
}

/*
*  @brief Broadcasts the new target waypoint and directs the MAV to fly there
*
*  This function broadcasts its new active waypoint sequence number and
*  sends a message to the controller, advising it to fly to the coordinates
*  of the waypoint with a given orientation
*
*  @param seq The waypoint sequence number the MAV should fly to.
*/
void MAVLinkSimulationWaypointPlanner::send_waypoint_current(uint16_t seq)
{
    if(seq < waypoints->size()) {
        mavlink_waypoint_t *cur = waypoints->at(seq);

        mavlink_message_t msg;
        mavlink_waypoint_current_t wpc;

        wpc.seq = cur->seq;

        mavlink_msg_waypoint_current_encode(systemid, compid, &msg, &wpc);
        link->sendMAVLinkMessage(&msg);



        if (verbose) qDebug("Broadcasted new current waypoint %u\n", wpc.seq);
    }
}

/*
*  @brief Directs the MAV to fly to a position
*
*  Sends a message to the controller, advising it to fly to the coordinates
*  of the waypoint with a given orientation
*
*  @param seq The waypoint sequence number the MAV should fly to.
*/
void MAVLinkSimulationWaypointPlanner::send_setpoint(uint16_t seq)
{
    if(seq < waypoints->size()) {
        mavlink_waypoint_t *cur = waypoints->at(seq);

        mavlink_message_t msg;
        mavlink_local_position_setpoint_set_t PControlSetPoint;

        // send new set point to local IMU
        if (cur->frame == 1) {
            PControlSetPoint.target_system = systemid;
            PControlSetPoint.target_component = MAV_COMP_ID_IMU;
            PControlSetPoint.x = cur->x;
            PControlSetPoint.y = cur->y;
            PControlSetPoint.z = cur->z;
            PControlSetPoint.yaw = cur->param4;

            mavlink_msg_local_position_setpoint_set_encode(systemid, compid, &msg, &PControlSetPoint);
            link->sendMAVLinkMessage(&msg);


        } else {
            //if (verbose) qDebug("No new set point sent to IMU because the new waypoint %u had no local coordinates\n", cur->seq);
            PControlSetPoint.target_system = systemid;
            PControlSetPoint.target_component = MAV_COMP_ID_IMU;
            PControlSetPoint.x = cur->x;
            PControlSetPoint.y = cur->y;
            PControlSetPoint.z = cur->z;
            PControlSetPoint.yaw = cur->param4;

            mavlink_msg_local_position_setpoint_set_encode(systemid, compid, &msg, &PControlSetPoint);
            link->sendMAVLinkMessage(&msg);
            emit messageSent(msg);
        }

        uint64_t now = QGC::groundTimeUsecs()/1000;
        timestamp_last_send_setpoint = now;
    }
}

void MAVLinkSimulationWaypointPlanner::send_waypoint_count(uint8_t target_systemid, uint8_t target_compid, uint16_t count)
{
    mavlink_message_t msg;
    mavlink_waypoint_count_t wpc;

    wpc.target_system = target_systemid;
    wpc.target_component = target_compid;
    wpc.count = count;

    mavlink_msg_waypoint_count_encode(systemid, compid, &msg, &wpc);
    link->sendMAVLinkMessage(&msg);

    if (verbose) qDebug("Sent waypoint count (%u) to ID %u\n", wpc.count, wpc.target_system);


}

void MAVLinkSimulationWaypointPlanner::send_waypoint(uint8_t target_systemid, uint8_t target_compid, uint16_t seq)
{
    if (seq < waypoints->size()) {
        mavlink_message_t msg;
        mavlink_waypoint_t *wp = waypoints->at(seq);
        wp->target_system = target_systemid;
        wp->target_component = target_compid;

        if (verbose) qDebug("Sent waypoint %u (%u / %u / %u / %u / %u / %f / %f / %f / %u / %f / %f / %f / %f / %u)\n", wp->seq, wp->target_system, wp->target_component, wp->seq, wp->frame, wp->command, wp->param3, wp->param1, wp->param2, wp->current, wp->x, wp->y, wp->z, wp->param4, wp->autocontinue);

        mavlink_msg_waypoint_encode(systemid, compid, &msg, wp);
        link->sendMAVLinkMessage(&msg);
        if (verbose) qDebug("Sent waypoint %u to ID %u\n", wp->seq, wp->target_system);


    } else {
        if (verbose) qDebug("ERROR: index out of bounds\n");
    }
}

void MAVLinkSimulationWaypointPlanner::send_waypoint_request(uint8_t target_systemid, uint8_t target_compid, uint16_t seq)
{
    mavlink_message_t msg;
    mavlink_waypoint_request_t wpr;
    wpr.target_system = target_systemid;
    wpr.target_component = target_compid;
    wpr.seq = seq;
    mavlink_msg_waypoint_request_encode(systemid, compid, &msg, &wpr);
    link->sendMAVLinkMessage(&msg);
    if (verbose) qDebug("Sent waypoint request %u to ID %u\n", wpr.seq, wpr.target_system);


}

/*
*  @brief emits a message that a waypoint reached
*
*  This function broadcasts a message that a waypoint is reached.
*
*  @param seq The waypoint sequence number the MAV has reached.
*/
void MAVLinkSimulationWaypointPlanner::send_waypoint_reached(uint16_t seq)
{
    mavlink_message_t msg;
    mavlink_waypoint_reached_t wp_reached;

    wp_reached.seq = seq;

    mavlink_msg_waypoint_reached_encode(systemid, compid, &msg, &wp_reached);
    link->sendMAVLinkMessage(&msg);

    if (verbose) qDebug("Sent waypoint %u reached message\n", wp_reached.seq);


}

float MAVLinkSimulationWaypointPlanner::distanceToSegment(uint16_t seq, float x, float y, float z)
{
    if (seq < waypoints->size()) {
        mavlink_waypoint_t *cur = waypoints->at(seq);

        const PxVector3 A(cur->x, cur->y, cur->z);
        const PxVector3 C(x, y, z);

        // seq not the second last waypoint
        if ((uint16_t)(seq+1) < waypoints->size()) {
            mavlink_waypoint_t *next = waypoints->at(seq+1);
            const PxVector3 B(next->x, next->y, next->z);
            const float r = (B-A).dot(C-A) / (B-A).lengthSquared();
            if (r >= 0 && r <= 1) {
                const PxVector3 P(A + r*(B-A));
                return (P-C).length();
            } else if (r < 0.f) {
                return (C-A).length();
            } else {
                return (C-B).length();
            }
        } else {
            return (C-A).length();
        }
    }
    return -1.f;
}

float MAVLinkSimulationWaypointPlanner::distanceToPoint(uint16_t seq, float x, float y, float z)
{
    if (seq < waypoints->size()) {
        mavlink_waypoint_t *cur = waypoints->at(seq);

        const PxVector3 A(cur->x, cur->y, cur->z);
        const PxVector3 C(x, y, z);

        return (C-A).length();
    }
    return -1.f;
}

float MAVLinkSimulationWaypointPlanner::distanceToPoint(uint16_t seq, float x, float y)
{
    if (seq < waypoints->size()) {
        mavlink_waypoint_t *cur = waypoints->at(seq);

        const PxVector3 A(cur->x, cur->y, 0);
        const PxVector3 C(x, y, 0);

        return (C-A).length();
    }
    return -1.f;
}

void MAVLinkSimulationWaypointPlanner::handleMessage(const mavlink_message_t& msg)
{
    mavlink_handler(&msg);
}

void MAVLinkSimulationWaypointPlanner::mavlink_handler (const mavlink_message_t* msg)
{
    // Handle param messages
//        paramClient->handleMAVLinkPacket(msg);

    //check for timed-out operations

    //qDebug() << "MAV: %d WAYPOINTPLANNER GOT MESSAGE" << systemid;

    uint64_t now = QGC::groundTimeUsecs()/1000;
    if (now-protocol_timestamp_lastaction > protocol_timeout && current_state != PX_WPP_IDLE) {
        if (verbose) qDebug() << "Last operation (state=%u) timed out, changing state to PX_WPP_IDLE" << current_state;
        current_state = PX_WPP_IDLE;
        protocol_current_count = 0;
        protocol_current_partner_systemid = 0;
        protocol_current_partner_compid = 0;
        protocol_current_wp_id = -1;

        if(waypoints->size() == 0) {
            current_active_wp_id = -1;
        }
    }

    if(now-timestamp_last_send_setpoint > setpointDelay) {
        send_setpoint(current_active_wp_id);
    }

    switch(msg->msgid) {
    case MAVLINK_MSG_ID_ATTITUDE: {
        if(msg->sysid == systemid && current_active_wp_id < waypoints->size()) {
            mavlink_waypoint_t *wp = waypoints->at(current_active_wp_id);
            if(wp->frame == 1) {
                mavlink_attitude_t att;
                mavlink_msg_attitude_decode(msg, &att);
                float yaw_tolerance = yawTolerance;
                //compare current yaw
                if (att.yaw - yaw_tolerance >= 0.0f && att.yaw + yaw_tolerance < 2.f*M_PI) {
                    if (att.yaw - yaw_tolerance <= wp->param4 && att.yaw + yaw_tolerance >= wp->param4)
                        yawReached = true;
                } else if(att.yaw - yaw_tolerance < 0.0f) {
                    float lowerBound = 360.0f + att.yaw - yaw_tolerance;
                    if (lowerBound < wp->param4 || wp->param4 < att.yaw + yaw_tolerance)
                        yawReached = true;
                } else {
                    float upperBound = att.yaw + yaw_tolerance - 2.f*M_PI;
                    if (att.yaw - yaw_tolerance < wp->param4 || wp->param4 < upperBound)
                        yawReached = true;
                }

                // FIXME HACK: Ignore yaw:

                yawReached = true;
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_LOCAL_POSITION: {
        if(msg->sysid == systemid && current_active_wp_id < waypoints->size()) {
            mavlink_waypoint_t *wp = waypoints->at(current_active_wp_id);

            if(wp->frame == 1) {
                mavlink_local_position_t pos;
                mavlink_msg_local_position_decode(msg, &pos);
                //qDebug() << "Received new position: x:" << pos.x << "| y:" << pos.y << "| z:" << pos.z;

                posReached = false;

                // compare current position (given in message) with current waypoint
                float orbit = wp->param1;

                float dist;
                if (wp->param2 == 0) {
                    dist = distanceToSegment(current_active_wp_id, pos.x, pos.y, pos.z);
                } else {
                    dist = distanceToPoint(current_active_wp_id, pos.x, pos.y, pos.z);
                }

                if (dist >= 0.f && dist <= orbit && yawReached) {
                    posReached = true;
                }
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        if(msg->sysid == systemid && current_active_wp_id < waypoints->size()) {
            mavlink_waypoint_t *wp = waypoints->at(current_active_wp_id);

            if(wp->frame == 0) {
                mavlink_global_position_int_t pos;
                mavlink_msg_global_position_int_decode(msg, &pos);

                float x = static_cast<double>(pos.lat)/1E7;
                float y = static_cast<double>(pos.lon)/1E7;
                //float z = static_cast<double>(pos.alt)/1000;

                //qDebug() << "Received new position: x:" << x << "| y:" << y << "| z:" << z;

                posReached = false;
                yawReached = true;

                // FIXME big hack for simulation!
                //float oneDegreeOfLatMeters = 111131.745f;
                float orbit = 0.00008f;

                // compare current position (given in message) with current waypoint
                //float orbit = wp->param1;

                // Convert to degrees


                float dist;
                dist = distanceToPoint(current_active_wp_id, x, y);

                if (dist >= 0.f && dist <= orbit && yawReached) {
                    posReached = true;
                    qDebug() << "WP PLANNER: REACHED POSITION";
                }
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_ACTION: { // special action from ground station
        mavlink_action_t action;
        mavlink_msg_action_decode(msg, &action);
        if(action.target == systemid) {
            if (verbose) qDebug("Waypoint: received message with action %d\n", action.action);
//            switch (action.action) {
//				case MAV_ACTION_LAUNCH:
//					if (verbose) std::cerr << "Launch received" << std::endl;
//					current_active_wp_id = 0;
//					if (waypoints->size()>0)
//					{
//						setActive(waypoints[current_active_wp_id]);
//					}
//					else
//						if (verbose) std::cerr << "No launch, waypointList empty" << std::endl;
//					break;

//				case MAV_ACTION_CONTINUE:
//					if (verbose) std::c
//					err << "Continue received" << std::endl;
//					idle = false;
//					setActive(waypoints[current_active_wp_id]);
//					break;

//				case MAV_ACTION_HALT:
//					if (verbose) std::cerr << "Halt received" << std::endl;
//					idle = true;
//					break;

//				default:
//					if (verbose) std::cerr << "Unknown action received with id " << action.action << ", no action taken" << std::endl;
//					break;
//            }
        }
        break;
	}

    case MAVLINK_MSG_ID_WAYPOINT_ACK: {
        mavlink_waypoint_ack_t wpa;
        mavlink_msg_waypoint_ack_decode(msg, &wpa);

        if((msg->sysid == protocol_current_partner_systemid && msg->compid == protocol_current_partner_compid) && (wpa.target_system == systemid && wpa.target_component == compid)) {
            protocol_timestamp_lastaction = now;

            if (current_state == PX_WPP_SENDLIST || current_state == PX_WPP_SENDLIST_SENDWPS) {
                if (protocol_current_wp_id == waypoints->size()-1) {
                    if (verbose) qDebug("Received Ack after having sent last waypoint, going to state PX_WPP_IDLE\n");
                    current_state = PX_WPP_IDLE;
                    protocol_current_wp_id = 0;
                }
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_WAYPOINT_SET_CURRENT: {
        mavlink_waypoint_set_current_t wpc;
        mavlink_msg_waypoint_set_current_decode(msg, &wpc);

        if(wpc.target_system == systemid && wpc.target_component == compid) {
            protocol_timestamp_lastaction = now;

            if (current_state == PX_WPP_IDLE) {
                if (wpc.seq < waypoints->size()) {
                    if (verbose) qDebug("Received MAVLINK_MSG_ID_WAYPOINT_SET_CURRENT\n");
                    current_active_wp_id = wpc.seq;
                    uint32_t i;
                    for(i = 0; i < waypoints->size(); i++) {
                        if (i == current_active_wp_id) {
                            waypoints->at(i)->current = true;
                        } else {
                            waypoints->at(i)->current = false;
                        }
                    }
                    if (verbose) qDebug("New current waypoint %u\n", current_active_wp_id);
                    yawReached = false;
                    posReached = false;
                    send_waypoint_current(current_active_wp_id);
                    send_setpoint(current_active_wp_id);
                    timestamp_firstinside_orbit = 0;
                } else {
                    if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_SET_CURRENT: Index out of bounds\n");
                }
            }
        } else {
            qDebug() << "SYSTEM / COMPONENT ID MISMATCH: target sys:" << wpc.target_system << "this system:" << systemid << "target comp:" << wpc.target_component << "this comp:" << compid;
        }
        break;
    }

    case MAVLINK_MSG_ID_WAYPOINT_REQUEST_LIST: {
        mavlink_waypoint_request_list_t wprl;
        mavlink_msg_waypoint_request_list_decode(msg, &wprl);
        if(wprl.target_system == systemid && wprl.target_component == compid) {
            protocol_timestamp_lastaction = now;

            if (current_state == PX_WPP_IDLE || current_state == PX_WPP_SENDLIST) {
                if (waypoints->size() > 0) {
                    if (verbose && current_state == PX_WPP_IDLE) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_REQUEST_LIST from %u changing state to PX_WPP_SENDLIST\n", msg->sysid);
                    if (verbose && current_state == PX_WPP_SENDLIST) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_REQUEST_LIST again from %u staying in state PX_WPP_SENDLIST\n", msg->sysid);
                    current_state = PX_WPP_SENDLIST;
                    protocol_current_wp_id = 0;
                    protocol_current_partner_systemid = msg->sysid;
                    protocol_current_partner_compid = msg->compid;
                } else {
                    if (verbose) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_REQUEST_LIST from %u but have no waypoints, staying in \n", msg->sysid);
                }
                protocol_current_count = waypoints->size();
                send_waypoint_count(msg->sysid,msg->compid, protocol_current_count);
            } else {
                if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST_LIST because i'm doing something else already (state=%i).\n", current_state);
            }
        } else {
            if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST_LIST because not my systemid or compid.\n");
        }
        break;
    }

    case MAVLINK_MSG_ID_WAYPOINT_REQUEST: {
        mavlink_waypoint_request_t wpr;
        mavlink_msg_waypoint_request_decode(msg, &wpr);
        if(msg->sysid == protocol_current_partner_systemid && msg->compid == protocol_current_partner_compid && wpr.target_system == systemid && wpr.target_component == compid) {
            protocol_timestamp_lastaction = now;

            //ensure that we are in the correct state and that the first request has id 0 and the following requests have either the last id (re-send last waypoint) or last_id+1 (next waypoint)
            if ((current_state == PX_WPP_SENDLIST && wpr.seq == 0) || (current_state == PX_WPP_SENDLIST_SENDWPS && (wpr.seq == protocol_current_wp_id || wpr.seq == protocol_current_wp_id + 1) && wpr.seq < waypoints->size())) {
                if (verbose && current_state == PX_WPP_SENDLIST) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_REQUEST of waypoint %u from %u changing state to PX_WPP_SENDLIST_SENDWPS\n", wpr.seq, msg->sysid);
                if (verbose && current_state == PX_WPP_SENDLIST_SENDWPS && wpr.seq == protocol_current_wp_id + 1) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_REQUEST of waypoint %u from %u staying in state PX_WPP_SENDLIST_SENDWPS\n", wpr.seq, msg->sysid);
                if (verbose && current_state == PX_WPP_SENDLIST_SENDWPS && wpr.seq == protocol_current_wp_id) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_REQUEST of waypoint %u (again) from %u staying in state PX_WPP_SENDLIST_SENDWPS\n", wpr.seq, msg->sysid);

                current_state = PX_WPP_SENDLIST_SENDWPS;
                protocol_current_wp_id = wpr.seq;
                send_waypoint(protocol_current_partner_systemid, protocol_current_partner_compid, wpr.seq);
            } else {
                if (verbose) {
                    if (!(current_state == PX_WPP_SENDLIST || current_state == PX_WPP_SENDLIST_SENDWPS)) {
                        qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST because i'm doing something else already (state=%i).\n", current_state);
                        break;
                    } else if (current_state == PX_WPP_SENDLIST) {
                        if (wpr.seq != 0) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST because the first requested waypoint ID (%u) was not 0.\n", wpr.seq);
                    } else if (current_state == PX_WPP_SENDLIST_SENDWPS) {
                        if (wpr.seq != protocol_current_wp_id && wpr.seq != protocol_current_wp_id + 1) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST because the requested waypoint ID (%u) was not the expected (%u or %u).\n", wpr.seq, protocol_current_wp_id, protocol_current_wp_id+1);
                        else if (wpr.seq >= waypoints->size()) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST because the requested waypoint ID (%u) was out of bounds.\n", wpr.seq);
                    } else qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST - FIXME: missed error description\n");
                }
            }
        } else {
            //we we're target but already communicating with someone else
            if((wpr.target_system == systemid && wpr.target_component == compid) && !(msg->sysid == protocol_current_partner_systemid && msg->compid == protocol_current_partner_compid)) {
                if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_REQUEST from ID %u because i'm already talking to ID %u.\n", msg->sysid, protocol_current_partner_systemid);
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_WAYPOINT_COUNT: {
        mavlink_waypoint_count_t wpc;
        mavlink_msg_waypoint_count_decode(msg, &wpc);
        if(wpc.target_system == systemid && wpc.target_component == compid) {
            protocol_timestamp_lastaction = now;

            if (current_state == PX_WPP_IDLE || (current_state == PX_WPP_GETLIST && protocol_current_wp_id == 0)) {
                if (wpc.count > 0) {
                    if (verbose && current_state == PX_WPP_IDLE) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_COUNT (%u) from %u changing state to PX_WPP_GETLIST\n", wpc.count, msg->sysid);
                    if (verbose && current_state == PX_WPP_GETLIST) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_COUNT (%u) again from %u\n", wpc.count, msg->sysid);

                    current_state = PX_WPP_GETLIST;
                    protocol_current_wp_id = 0;
                    protocol_current_partner_systemid = msg->sysid;
                    protocol_current_partner_compid = msg->compid;
                    protocol_current_count = wpc.count;

                    qDebug("clearing receive buffer and readying for receiving waypoints\n");
                    while(waypoints_receive_buffer->size() > 0) {
                        delete waypoints_receive_buffer->back();
                        waypoints_receive_buffer->pop_back();
                    }

                    send_waypoint_request(protocol_current_partner_systemid, protocol_current_partner_compid, protocol_current_wp_id);
                } else {
                    if (verbose) qDebug("Ignoring MAVLINK_MSG_ID_WAYPOINT_COUNT from %u with count of %u\n", msg->sysid, wpc.count);
                }
            } else {
                if (verbose && !(current_state == PX_WPP_IDLE || current_state == PX_WPP_GETLIST)) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_COUNT because i'm doing something else already (state=%i).\n", current_state);
                else if (verbose && current_state == PX_WPP_GETLIST && protocol_current_wp_id != 0) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_COUNT because i'm already receiving waypoint %u.\n", protocol_current_wp_id);
                else qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_COUNT - FIXME: missed error description\n");
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_WAYPOINT: {
        mavlink_waypoint_t wp;
        mavlink_msg_waypoint_decode(msg, &wp);

        if((msg->sysid == protocol_current_partner_systemid && msg->compid == protocol_current_partner_compid) && (wp.target_system == systemid && wp.target_component == compid)) {
            protocol_timestamp_lastaction = now;

            //ensure that we are in the correct state and that the first waypoint has id 0 and the following waypoints have the correct ids
            if ((current_state == PX_WPP_GETLIST && wp.seq == 0) || (current_state == PX_WPP_GETLIST_GETWPS && wp.seq == protocol_current_wp_id && wp.seq < protocol_current_count)) {
                if (verbose && current_state == PX_WPP_GETLIST) qDebug("Got MAVLINK_MSG_ID_WAYPOINT %u from %u changing state to PX_WPP_GETLIST_GETWPS\n", wp.seq, msg->sysid);
                if (verbose && current_state == PX_WPP_GETLIST_GETWPS && wp.seq == protocol_current_wp_id) qDebug("Got MAVLINK_MSG_ID_WAYPOINT %u from %u\n", wp.seq, msg->sysid);
                if (verbose && current_state == PX_WPP_GETLIST_GETWPS && wp.seq-1 == protocol_current_wp_id) qDebug("Got MAVLINK_MSG_ID_WAYPOINT %u (again) from %u\n", wp.seq, msg->sysid);

                current_state = PX_WPP_GETLIST_GETWPS;
                protocol_current_wp_id = wp.seq + 1;
                mavlink_waypoint_t* newwp = new mavlink_waypoint_t;
                memcpy(newwp, &wp, sizeof(mavlink_waypoint_t));
                waypoints_receive_buffer->push_back(newwp);

                if(protocol_current_wp_id == protocol_current_count && current_state == PX_WPP_GETLIST_GETWPS) {
                    if (verbose) qDebug("Got all %u waypoints, changing state to PX_WPP_IDLE\n", protocol_current_count);

                    send_waypoint_ack(protocol_current_partner_systemid, protocol_current_partner_compid, 0);

                    if (current_active_wp_id > waypoints_receive_buffer->size()-1) {
                        current_active_wp_id = waypoints_receive_buffer->size() - 1;
                    }

                    // switch the waypoints list
                    std::vector<mavlink_waypoint_t*>* waypoints_temp = waypoints;
                    waypoints = waypoints_receive_buffer;
                    waypoints_receive_buffer = waypoints_temp;

                    //get the new current waypoint
                    uint32_t i;
                    for(i = 0; i < waypoints->size(); i++) {
                        if (waypoints->at(i)->current == 1) {
                            current_active_wp_id = i;
                            //if (verbose) qDebug("New current waypoint %u\n", current_active_wp_id);
                            yawReached = false;
                            posReached = false;
                            send_waypoint_current(current_active_wp_id);
                            send_setpoint(current_active_wp_id);
                            timestamp_firstinside_orbit = 0;
                            break;
                        }
                    }

                    if (i == waypoints->size()) {
                        current_active_wp_id = -1;
                        yawReached = false;
                        posReached = false;
                        timestamp_firstinside_orbit = 0;
                    }

                    current_state = PX_WPP_IDLE;
                } else {
                    send_waypoint_request(protocol_current_partner_systemid, protocol_current_partner_compid, protocol_current_wp_id);
                }
            } else {
                if (current_state == PX_WPP_IDLE) {
                    //we're done receiving waypoints, answer with ack.
                    send_waypoint_ack(protocol_current_partner_systemid, protocol_current_partner_compid, 0);
                    qDebug("Received MAVLINK_MSG_ID_WAYPOINT while state=PX_WPP_IDLE, answered with WAYPOINT_ACK.\n");
                }
                if (verbose) {
                    if (!(current_state == PX_WPP_GETLIST || current_state == PX_WPP_GETLIST_GETWPS)) {
                        qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT %u because i'm doing something else already (state=%i).\n", wp.seq, current_state);
                        break;
                    } else if (current_state == PX_WPP_GETLIST) {
                        if(!(wp.seq == 0)) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT because the first waypoint ID (%u) was not 0.\n", wp.seq);
                        else qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT %u - FIXME: missed error description\n", wp.seq);
                    } else if (current_state == PX_WPP_GETLIST_GETWPS) {
                        if (!(wp.seq == protocol_current_wp_id)) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT because the waypoint ID (%u) was not the expected %u.\n", wp.seq, protocol_current_wp_id);
                        else if (!(wp.seq < protocol_current_count)) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT because the waypoint ID (%u) was out of bounds.\n", wp.seq);
                        else qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT %u - FIXME: missed error description\n", wp.seq);
                    } else qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT %u - FIXME: missed error description\n", wp.seq);
                }
            }
        } else {
            // We're target but already communicating with someone else
            if((wp.target_system == systemid && wp.target_component == compid) && !(msg->sysid == protocol_current_partner_systemid && msg->compid == protocol_current_partner_compid) && current_state != PX_WPP_IDLE) {
                if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT %u from ID %u because i'm already talking to ID %u.\n", wp.seq, msg->sysid, protocol_current_partner_systemid);
            } else if(wp.target_system == systemid && wp.target_component == compid) {
                if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT %u from ID %u because i have no idea what to do with it\n", wp.seq, msg->sysid);
            }
        }
        break;
    }

    case MAVLINK_MSG_ID_WAYPOINT_CLEAR_ALL: {
        mavlink_waypoint_clear_all_t wpca;
        mavlink_msg_waypoint_clear_all_decode(msg, &wpca);

        if(wpca.target_system == systemid && wpca.target_component == compid && current_state == PX_WPP_IDLE) {
            protocol_timestamp_lastaction = now;

            if (verbose) qDebug("Got MAVLINK_MSG_ID_WAYPOINT_CLEAR_LIST from %u deleting all waypoints\n", msg->sysid);
            while(waypoints->size() > 0) {
                delete waypoints->back();
                waypoints->pop_back();
            }
            current_active_wp_id = -1;
        } else if (wpca.target_system == systemid && wpca.target_component == compid && current_state != PX_WPP_IDLE) {
            if (verbose) qDebug("Ignored MAVLINK_MSG_ID_WAYPOINT_CLEAR_LIST from %u because i'm doing something else already (state=%i).\n", msg->sysid, current_state);
        }
        break;
    }

    default: {
        if (debug) qDebug("Waypoint: received message of unknown type\n");
        break;
    }
    }

    //check if the current waypoint was reached
    if ((posReached && /*yawReached &&*/ !idle)) {
        if (current_active_wp_id < waypoints->size()) {
            mavlink_waypoint_t *cur_wp = waypoints->at(current_active_wp_id);

            if (timestamp_firstinside_orbit == 0) {
                // Announce that last waypoint was reached
                if (verbose) qDebug("*** Reached waypoint %u ***\n", cur_wp->seq);
                send_waypoint_reached(cur_wp->seq);
                timestamp_firstinside_orbit = now;
            }

            // check if the MAV was long enough inside the waypoint orbit
            //if (now-timestamp_lastoutside_orbit > (cur_wp->hold_time*1000))
            if(now-timestamp_firstinside_orbit >= cur_wp->param2*1000) {
                if (cur_wp->autocontinue) {
                    cur_wp->current = 0;
                    if (current_active_wp_id == waypoints->size() - 1 && waypoints->size() > 0) {
                        //the last waypoint was reached, if auto continue is
                        //activated restart the waypoint list from the beginning
                        current_active_wp_id = 0;
                    } else {
                        current_active_wp_id++;
                    }

                    // Fly to next waypoint
                    timestamp_firstinside_orbit = 0;
                    send_waypoint_current(current_active_wp_id);
                    send_setpoint(current_active_wp_id);
                    waypoints->at(current_active_wp_id)->current = true;
                    posReached = false;
                    //yawReached = false;
                    if (verbose) qDebug("Set new waypoint (%u)\n", current_active_wp_id);
                }
            }
        }
    } else {
        timestamp_lastoutside_orbit = now;
    }
}
