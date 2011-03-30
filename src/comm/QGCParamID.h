/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Stores the paramid used for mavlink
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef QGCPARAMID_H
#define QGCPARAMID_H

#include <QString>

#include "mavlink_types.h"

//namespace OpalRT
//{
//    class QGCParamID;
//}
//
//QDataStream& operator<<(QDataStream&, const OpalRT::QGCParamID&);

namespace OpalRT
{
/** Stores a param_id for the mavlink parameter packets.  This class adds the convenience
  of storing the id as a string (e.g., easy comparison).

  \todo Fix: warning: deprecated conversion from string constant to 'char*'
  */
class QGCParamID
{
//        friend QDataStream& operator<<(QDataStream& stream, const QGCParamID& paramid);
public:

    QGCParamID(const char[]);
    QGCParamID(const QString);
    QGCParamID() {}
    QGCParamID(const QGCParamID& other);

    bool operator<(const QGCParamID& other) const {
        return data<other.data;
    }
    bool operator==(const QGCParamID& other) const {
        return data == other.data;
    }
    operator QString() const {
        return data;
    }

    const QString getParamString() const {
        return static_cast<const QString>(data);
    }
    int8_t* toInt8_t() const {
        return (int8_t*)data.toAscii().data();
    }

protected:
    QString data;
};
}
#endif // QGCPARAMID_H
