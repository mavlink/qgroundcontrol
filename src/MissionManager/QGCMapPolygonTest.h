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
#include "QGCMapPolygon.h"
#include "QmlObjectListModel.h"

class QGCMapPolygonTest : public UnitTest
{
    Q_OBJECT

public:
    QGCMapPolygonTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;

private slots:
    void _testDirty(void);
    void _testVertexManipulation(void);
    void _testKMLLoad(void);
    void _testSelectVertex(void);
    void _testSegmentSplit(void);

private:
    enum {
        polygonCountChangedIndex = 0,
        pathChangedIndex,
        polygonDirtyChangedIndex,
        clearedIndex,
        centerChangedIndex,
        maxPolygonSignalIndex
    };

    enum {
        polygonCountChangedMask =   1 << polygonCountChangedIndex,
        pathChangedMask =           1 << pathChangedIndex,
        polygonDirtyChangedMask =   1 << polygonDirtyChangedIndex,
        clearedMask =               1 << clearedIndex,
        centerChangedMask =         1 << centerChangedIndex,
    };

    static const size_t _cPolygonSignals = maxPolygonSignalIndex;
    const char*         _rgPolygonSignals[_cPolygonSignals];

    void countChanged(int count);
    void dirtyChanged(bool dirtyChanged);

    enum {
        modelCountChangedIndex = 0,
        modelDirtyChangedIndex,
        maxModelSignalIndex
    };

    enum {
        modelCountChangedMask = 1 << modelCountChangedIndex,
        modelDirtyChangedMask = 1 << modelDirtyChangedIndex,
    };

    static const size_t _cModelSignals = maxModelSignalIndex;
    const char*         _rgModelSignals[_cModelSignals];

    MultiSignalSpy*         _multiSpyPolygon;
    MultiSignalSpy*         _multiSpyModel;
    QGCMapPolygon*          _mapPolygon;
    QmlObjectListModel*     _pathModel;
    QList<QGeoCoordinate>   _polyPoints;
};
