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
 *   @brief Implementation of class OpalRT::Parameter
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */
#include "Parameter.h"
using namespace OpalRT;

//Parameter::Parameter(char *simulinkPath, char *simulinkName, uint8_t componentID,
//                     QGCParamID paramID, unsigned short opalID)
//                         : simulinkPath(new QString(simulinkPath)),
//                         simulinkName(new QString(simulinkName)),
//                         componentID(componentID),
//                         paramID(new QGCParamID(paramID)),
//                         opalID(opalID)
//
//{
//}
Parameter::Parameter(QString simulinkPath, QString simulinkName, uint8_t componentID,
                     QGCParamID paramID, unsigned short opalID)
    : simulinkPath(new QString(simulinkPath)),
      simulinkName(new QString(simulinkName)),
      componentID(componentID),
      paramID(new QGCParamID(paramID)),
      opalID(opalID)

{
}
Parameter::Parameter(const Parameter &other)
    : componentID(other.componentID),
      opalID(other.opalID)
{
    simulinkPath = new QString(*other.simulinkPath);
    simulinkName = new QString(*other.simulinkName);
    paramID = new QGCParamID(*other.paramID);
}

Parameter::~Parameter()
{
    delete simulinkPath;
    delete simulinkName;
    delete paramID;
}

bool Parameter::operator ==(const Parameter& other) const
{
    return
        (*simulinkPath) == *(other.simulinkPath)
        && *simulinkName == *(other.simulinkName)
        && componentID == other.componentID
        && *paramID == *(other.paramID)
        && opalID == other.opalID;

}

float Parameter::getValue()
{
    unsigned short allocatedParams = 1;
    unsigned short numParams;
    unsigned short numValues = 1;
    unsigned short returnedNumValues;
    double value;

    int returnVal = OpalGetParameters(allocatedParams, &numParams, &opalID,
                                      numValues, &returnedNumValues, &value);

    if (returnVal != EOK) {
        OpalRT::OpalErrorMsg::displayLastErrorMsg();
        return FLT_MAX;
    }

    return static_cast<float>(value);
}

void Parameter::setValue(float val)
{
    unsigned short allocatedParams = 1;
    unsigned short numParams;
    unsigned short numValues = 1;
    unsigned short returnedNumValues;
    double value = static_cast<double>(val);

    int returnVal = OpalSetParameters(allocatedParams, &numParams, &opalID,
                                      numValues, &returnedNumValues, &value);
    if (returnVal != EOK) {
        //qDebug() << __FILE__ << ":" << __LINE__ << ": Error numer: " << QString::number(returnVal);
        OpalErrorMsg::displayLastErrorMsg();
    }
}

Parameter::operator QString() const
{
    return *simulinkPath + *simulinkName + " " + QString::number(componentID)
           + " " + *paramID + " " + QString::number(opalID);
}
