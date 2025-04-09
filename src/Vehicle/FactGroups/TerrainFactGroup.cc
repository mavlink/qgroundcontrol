/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainFactGroup.h"

TerrainFactGroup::TerrainFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/TerrainFactGroup.json"), parent)
{
    _addFact(&_blocksPendingFact);
    _addFact(&_blocksLoadedFact);
}
