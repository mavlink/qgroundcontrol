/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComplexMissionItem.h"

const char* ComplexMissionItem::jsonComplexItemTypeKey = "complexItemType";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, QObject* parent)
    : VisualMissionItem(vehicle, parent)
{

}

const ComplexMissionItem& ComplexMissionItem::operator=(const ComplexMissionItem& other)
{
    VisualMissionItem::operator=(other);

    return *this;
}
