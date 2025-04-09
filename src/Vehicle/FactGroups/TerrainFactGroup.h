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
    Q_PROPERTY(Fact *blocksPending  READ blocksPending  CONSTANT)
    Q_PROPERTY(Fact *blocksLoaded   READ blocksLoaded   CONSTANT)

public:
    explicit TerrainFactGroup(QObject *parent = nullptr);

    Fact *blocksPending() { return &_blocksPendingFact; }
    Fact *blocksLoaded() { return &_blocksLoadedFact; }

private:
    Fact _blocksPendingFact = Fact(0, QStringLiteral("blocksPending"), FactMetaData::valueTypeDouble);
    Fact _blocksLoadedFact = Fact(0, QStringLiteral("blocksLoaded"), FactMetaData::valueTypeDouble);
};
