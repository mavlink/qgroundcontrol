/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "MultiSignalSpy.h"
#include "StructureScanComplexItem.h"

class StructureScanComplexItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    StructureScanComplexItemTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
private slots:
    void _testDirty(void);
    void _testSaveLoad(void);
    void _testItemCount(void);

private:
    void _initItem(void);
    void _validateItem(StructureScanComplexItem* item);

    enum {
        dirtyChangedIndex,
        maxSignalIndex
    };

    enum {
        dirtyChangedMask = 1 << dirtyChangedIndex
    };

    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];

    Vehicle*                    _offlineVehicle;
    MultiSignalSpy*             _multiSpy;
    StructureScanComplexItem*   _structureScanItem;
    QList<QGeoCoordinate>       _polyPoints;
};
