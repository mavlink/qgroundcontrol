/*=====================================================================

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
 *   @brief Parameter Object used to intefrace with an OpalRT Simulink Parameter
     \see OpalLink
     \see OpalRT::ParameterList
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef PARAMETER_H
#define PARAMETER_H

#include <QString>
#include <QDebug>

#include "mavlink_types.h"
#include "QGCParamID.h"
#include "OpalApi.h"
#include "OpalRT.h"
#include <cfloat>

namespace OpalRT
{
class Parameter
{
public:
//        Parameter(char *simulinkPath = "",
//                  char *simulinkName = "",
//                  uint8_t componentID = 0,
//                  QGCParamID paramID = QGCParamID(),
//                  unsigned short opalID = 0);
    Parameter(QString simulinkPath = QString(),
              QString simulinkName = QString(),
              uint8_t componentID = 0,
              QGCParamID paramID = QGCParamID(),
              unsigned short opalID = 0);
    Parameter(const Parameter& other);
    ~Parameter();

    const QGCParamID& getParamID() const {
        return *paramID;
    }
    void setOpalID(unsigned short opalID) {
        this->opalID = opalID;
    }
    const QString& getSimulinkPath() const {
        return *simulinkPath;
    }
    const QString& getSimulinkName() const {
        return *simulinkName;
    }
    uint8_t getComponentID() const {
        return componentID;
    }
    float getValue();
    void setValue(float value);

    bool operator==(const Parameter& other) const;
    operator QString() const;

protected:
    QString *simulinkPath;
    QString *simulinkName;
    uint8_t componentID;
    QGCParamID *paramID;
    unsigned short opalID;
};
}

#endif // PARAMETER_H
