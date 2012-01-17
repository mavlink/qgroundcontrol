/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Waypoint class
 *
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include <QStringList>
#include <QDebug>

#include "Waypoint.h"

Waypoint::Waypoint(quint16 _id, double _x, double _y, double _z, double _param1, double _param2, double _param3, double _param4,
                   bool _autocontinue, bool _current, MAV_FRAME _frame, MAV_CMD _action, const QString& _description)
    : id(_id),
      x(_x),
      y(_y),
      z(_z),
      yaw(_param4),
      frame(_frame),
      action(_action),
      autocontinue(_autocontinue),
      current(_current),
      orbit(_param3),
      param1(_param1),
      param2(_param2),
      name(QString("WP%1").arg(id, 2, 10, QChar('0'))),
      description(_description),
      reachedTime(0)
{
}

Waypoint::~Waypoint()
{    
}

bool Waypoint::isNavigationType()
{
    return (action < MAV_CMD_NAV_LAST);
}

void Waypoint::save(QTextStream &saveStream)
{
    QString position("%1\t%2\t%3");
    position = position.arg(x, 0, 'g', 18);
    position = position.arg(y, 0, 'g', 18);
    position = position.arg(z, 0, 'g', 18);
    QString parameters("%1\t%2\t%3\t%4");
    parameters = parameters.arg(param1, 0, 'g', 18).arg(param2, 0, 'g', 18).arg(orbit, 0, 'g', 18).arg(yaw, 0, 'g', 18);
    // FORMAT: <INDEX> <CURRENT WP> <COORD FRAME> <COMMAND> <PARAM1> <PARAM2> <PARAM3> <PARAM4> <PARAM5/X/LONGITUDE> <PARAM6/Y/LATITUDE> <PARAM7/Z/ALTITUDE> <AUTOCONTINUE> <DESCRIPTION>
    // as documented here: http://qgroundcontrol.org/waypoint_protocol
    saveStream << this->getId() << "\t" << this->getCurrent() << "\t" << this->getFrame() << "\t" << this->getAction() << "\t"  << parameters << "\t" << position  << "\t" << this->getAutoContinue() << "\r\n"; //"\t" << this->getDescription() << "\r\n";
}

bool Waypoint::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 12) {
        this->id = wpParams[0].toInt();
        this->current = (wpParams[1].toInt() == 1 ? true : false);
        this->frame = (MAV_FRAME) wpParams[2].toInt();
        this->action = (MAV_CMD) wpParams[3].toInt();
        this->param1 = wpParams[4].toDouble();
        this->param2 = wpParams[5].toDouble();
        this->orbit = wpParams[6].toDouble();
        this->yaw = wpParams[7].toDouble();
        this->x = wpParams[8].toDouble();
        this->y = wpParams[9].toDouble();
        this->z = wpParams[10].toDouble();
        this->autocontinue = (wpParams[11].toInt() == 1 ? true : false);
        //this->description = wpParams[12];
        return true;
    }
    return false;
}


void Waypoint::setId(quint16 id)
{
    this->id = id;
    this->name = QString("WP%1").arg(id, 2, 10, QChar('0'));
    emit changed(this);
}

void Waypoint::setX(double x)
{
    if (!isinf(x) && !isnan(x) && ((this->frame == MAV_FRAME_LOCAL_NED) || (this->frame == MAV_FRAME_LOCAL_ENU)))
    {
        this->x = x;
        emit changed(this);
    }
}

void Waypoint::setY(double y)
{
    if (!isinf(y) && !isnan(y) && ((this->frame == MAV_FRAME_LOCAL_NED) || (this->frame == MAV_FRAME_LOCAL_ENU)))
    {
        this->y = y;
        emit changed(this);
    }
}

void Waypoint::setZ(double z)
{
    if (!isinf(z) && !isnan(z) && ((this->frame == MAV_FRAME_LOCAL_NED) || (this->frame == MAV_FRAME_LOCAL_ENU)))
    {
        this->z = z;
        emit changed(this);
    }
}

void Waypoint::setLatitude(double lat)
{
    if (this->x != lat && ((this->frame == MAV_FRAME_GLOBAL) || (this->frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        this->x = lat;
        emit changed(this);
    }
}

void Waypoint::setLongitude(double lon)
{
    if (this->y != lon && ((this->frame == MAV_FRAME_GLOBAL) || (this->frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        this->y = lon;
        emit changed(this);
    }
}

void Waypoint::setAltitude(double altitude)
{
    if (this->z != altitude && ((this->frame == MAV_FRAME_GLOBAL) || (this->frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        this->z = altitude;
        emit changed(this);
    }
}

void Waypoint::setYaw(int yaw)
{
    if (this->yaw != yaw)
    {
        this->yaw = yaw;
        emit changed(this);
    }
}

void Waypoint::setYaw(double yaw)
{
    if (this->yaw != yaw)
    {
        this->yaw = yaw;
        emit changed(this);
    }
}

void Waypoint::setAction(int action)
{
    if (this->action != (MAV_CMD)action)
    {
        this->action = (MAV_CMD)action;
        emit changed(this);
    }
}

void Waypoint::setAction(MAV_CMD action)
{
    if (this->action != action) {
        this->action = action;
        emit changed(this);
    }
}

void Waypoint::setFrame(MAV_FRAME frame)
{
    if (this->frame != frame) {
        this->frame = frame;
        emit changed(this);
    }
}

void Waypoint::setAutocontinue(bool autoContinue)
{
    if (this->autocontinue != autoContinue) {
        this->autocontinue = autoContinue;
        emit changed(this);
    }
}

void Waypoint::setCurrent(bool current)
{
    if (this->current != current)
    {
        this->current = current;
        emit changed(this);
    }
}

void Waypoint::setAcceptanceRadius(double radius)
{
    if (this->param2 != radius)
    {
        this->param2 = radius;
        emit changed(this);
    }
}

void Waypoint::setParam1(double param1)
{
    //// // qDebug() << "SENDER:" << QObject::sender();
    //// // qDebug() << "PARAM1 SET REQ:" << param1;
    if (this->param1 != param1)
    {
        this->param1 = param1;
        emit changed(this);
    }
}

void Waypoint::setParam2(double param2)
{
    if (this->param2 != param2)
    {
        this->param2 = param2;
        emit changed(this);
    }
}

void Waypoint::setParam3(double param3)
{
    if (this->orbit != param3) {
        this->orbit = param3;
        emit changed(this);
    }
}

void Waypoint::setParam4(double param4)
{
    if (this->yaw != param4) {
        this->yaw = param4;
        emit changed(this);
    }
}

void Waypoint::setParam5(double param5)
{
    if (this->x != param5) {
        this->x = param5;
        emit changed(this);
    }
}

void Waypoint::setParam6(double param6)
{
    if (this->y != param6) {
        this->y = param6;
        emit changed(this);
    }
}

void Waypoint::setParam7(double param7)
{
    if (this->z != param7) {
        this->z = param7;
        emit changed(this);
    }
}

void Waypoint::setLoiterOrbit(double orbit)
{
    if (this->orbit != orbit) {
        this->orbit = orbit;
        emit changed(this);
    }
}

void Waypoint::setHoldTime(int holdTime)
{
    if (this->param1 != holdTime) {
        this->param1 = holdTime;
        emit changed(this);
    }
}

void Waypoint::setHoldTime(double holdTime)
{
    if (this->param1 != holdTime) {
        this->param1 = holdTime;
        emit changed(this);
    }
}

void Waypoint::setTurns(int turns)
{
    if (this->param1 != turns) {
        this->param1 = turns;
        emit changed(this);
    }
}
