/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomFactGroup.h"

GPSRTKFactGroup::GPSRTKFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Custom/CustomFact.json", parent)
    , _custom(0, _customFactName, FactMetaData::valueTypeBool)
{
    _addFact(&_custom, _customFactName);
}
