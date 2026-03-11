#pragma once

#include "UnitTest.h"

class QGCCachedTileSetTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testConstructorSetsName();
    void _testDefaultPropertyValues();
    void _testSetMapTypeStr();
    void _testSetCoordinates();
    void _testSetZoomAndMetadata();
    void _testSetTotalTileCountEmitsSignal();
    void _testSetTotalTileSizeEmitsSignal();
    void _testSetSavedCountAndSizeEmitSignals();
    void _testSetUniqueCountAndSizeEmitSignals();
    void _testSetNameEmitsSignal();
    void _testSetStateEmitsSignals();
    void _testSetterNoSignalOnSameValue();
    void _testCompleteWhenDefaultSet();
    void _testCompleteWhenAllSaved();
    void _testSetSelectedEmitsSignal();
};
