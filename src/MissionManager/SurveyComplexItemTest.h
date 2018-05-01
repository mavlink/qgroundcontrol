/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"
#include "SurveyComplexItem.h"

#include <QGeoCoordinate>

/// Unit test for SurveyComplexItem
class SurveyComplexItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    SurveyComplexItemTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
private slots:
    void _testDirty(void);
    void _testCameraValueChanged(void);
    void _testCameraTrigger(void);
    void _testGridAngle(void);
    void _testEntryLocation(void);
    void _testItemCount(void);

private:
    double _clampGridAngle180(double gridAngle);
    void _setPolygon(void);

    enum {
        gridPointsChangedIndex = 0,
        cameraShotsChangedIndex,
        coveredAreaChangedIndex,
        cameraValueChangedIndex,
        gridTypeChangedIndex,
        timeBetweenShotsChangedIndex,
        cameraOrientationFixedChangedIndex,
        refly90DegreesChangedIndex,
        dirtyChangedIndex,
        maxSignalIndex
    };

    enum {
        gridPointsChangedMask =             1 << gridPointsChangedIndex,
        cameraShotsChangedMask =            1 << cameraShotsChangedIndex,
        coveredAreaChangedMask =            1 << coveredAreaChangedIndex,
        cameraValueChangedMask =            1 << cameraValueChangedIndex,
        gridTypeChangedMask =               1 << gridTypeChangedIndex,
        timeBetweenShotsChangedMask =       1 << timeBetweenShotsChangedIndex,
        cameraOrientationFixedChangedMask = 1 << cameraOrientationFixedChangedIndex,
        refly90DegreesChangedMask =         1 << refly90DegreesChangedIndex,
        dirtyChangedMask =                  1 << dirtyChangedIndex
    };

    static const size_t _cSurveySignals = maxSignalIndex;
    const char*         _rgSurveySignals[_cSurveySignals];

    Vehicle*                _offlineVehicle;
    MultiSignalSpy*         _multiSpy;
    SurveyComplexItem*      _surveyItem;
    QGCMapPolygon*          _mapPolygon;
    QList<QGeoCoordinate>   _polyPoints;
};
