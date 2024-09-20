/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPOperator.h"

UTMSPOperator::UTMSPOperator()
{

}

std::string UTMSPOperator::operatorID()
{
    //TODO--> Get the operator ID from QGC UI
    std::string operatorID = "test.123";
    return operatorID;
}

std::string UTMSPOperator::operatorClass()
{
    //TODO--> Get the operator class
    std::string operatorClass = "";
    return operatorClass;
}
