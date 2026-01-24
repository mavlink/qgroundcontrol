#pragma once

#include "TestFixtures.h"

class PlanMasterController;
class StructureScanComplexItem;
class MultiSignalSpy;

/// Unit test for StructureScanComplexItem.
/// Uses OfflineTest since it works with offline PlanMasterController.
class StructureScanComplexItemTest : public OfflineTest
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

    PlanMasterController* _masterController = nullptr;
    MultiSignalSpy* _multiSpy = nullptr;
    StructureScanComplexItem* _structureScanItem = nullptr;
    QList<QGeoCoordinate> _polyPoints;
};
