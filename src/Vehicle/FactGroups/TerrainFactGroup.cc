#include "TerrainFactGroup.h"

TerrainFactGroup::TerrainFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/TerrainFactGroup.json"), parent)
{
    _addFact(&_blocksPendingFact);
    _addFact(&_blocksLoadedFact);
}
