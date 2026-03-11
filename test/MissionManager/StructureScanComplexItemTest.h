#pragma once

#include "BaseClasses/MissionTest.h"

class StructureScanComplexItem;
class MultiSignalSpy;

class StructureScanComplexItemTest : public OfflineMissionTest
{
    Q_OBJECT

public:
    StructureScanComplexItemTest();

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testSaveLoad();
    void _testItemCount();

private:
    void _initItem();
    void _validateItem(StructureScanComplexItem* item);

    MultiSignalSpy* _multiSpy = nullptr;
    StructureScanComplexItem* _structureScanItem = nullptr;
    QList<QGeoCoordinate> _polyPoints;
};
