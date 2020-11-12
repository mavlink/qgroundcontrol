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
#include "QGCMapPolyline.h"
#include "QmlObjectListModel.h"

class QGCMapPolylineTest : public UnitTest
{
    Q_OBJECT

public:
    QGCMapPolylineTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;

private slots:
    void _testDirty(void);
    void _testVertexManipulation(void);
//    void _testKMLLoad(void);
    void _testSelectVertex(void);

private:
    enum {
        countChangedIndex = 0,
        pathChangedIndex,
        dirtyChangedIndex,
        clearedIndex,
        maxSignalIndex
    };

    enum {
        countChangedMask =  1 << countChangedIndex,
        pathChangedMask =   1 << pathChangedIndex,
        dirtyChangedMask =  1 << dirtyChangedIndex,
        clearedMask =       1 << clearedIndex,
    };

    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];

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

    MultiSignalSpy*         _multiSpyPolyline;
    MultiSignalSpy*         _multiSpyModel;
    QGCMapPolyline*         _mapPolyline;
    QmlObjectListModel*     _pathModel;
    QList<QGeoCoordinate>   _linePoints;
};
