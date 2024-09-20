/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

class TerrainFactGroup : public FactGroup
{
    Q_OBJECT

public:
    TerrainFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* blocksPending  READ blocksPending  CONSTANT)
    Q_PROPERTY(Fact* blocksLoaded   READ blocksLoaded   CONSTANT)

    Fact* blocksPending () { return &_blocksPendingFact; }
    Fact* blocksLoaded  () { return &_blocksLoadedFact; }

private:
    const QString _blocksPendingFactName =  QStringLiteral("blocksPending");
    const QString _blocksLoadedFactName =   QStringLiteral("blocksLoaded");

    Fact _blocksPendingFact;
    Fact _blocksLoadedFact;
};
