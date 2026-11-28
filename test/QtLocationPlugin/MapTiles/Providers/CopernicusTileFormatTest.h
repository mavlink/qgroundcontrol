#pragma once

#include "UnitTest.h"

class CopernicusTileFormatTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSerializedAnswerIsReadAsTile();
    void _testAnswerThatIsNoTileGivesNone_data();
    void _testAnswerThatIsNoTileGivesNone();
};
