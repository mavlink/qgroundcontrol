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
    void _testGridAngle(void);
    void _testEntryLocation(void);
    void _testItemCount(void);

private:

    double _clampGridAngle180(double gridAngle);
    void _setPolygon(void);

    // SurveyComplexItem signals

    enum {
        surveyVisualTransectPointsChangedIndex = 0,
        surveyCameraShotsChangedIndex,
        surveyCoveredAreaChangedIndex,
        surveyTimeBetweenShotsChangedIndex,
        surveyRefly90DegreesChangedIndex,
        surveyDirtyChangedIndex,
        surveyMaxSignalIndex
    };

    enum {
        surveyVisualTransectPointsChangedMask = 1 << surveyVisualTransectPointsChangedIndex,
        surveyCameraShotsChangedMask =          1 << surveyCameraShotsChangedIndex,
        surveyCoveredAreaChangedMask =          1 << surveyCoveredAreaChangedIndex,
        surveyTimeBetweenShotsChangedMask =     1 << surveyTimeBetweenShotsChangedIndex,
        surveyRefly90DegreesChangedMask =       1 << surveyRefly90DegreesChangedIndex,
        surveyDirtyChangedMask =                1 << surveyDirtyChangedIndex
    };

    static const size_t _cSurveySignals = surveyMaxSignalIndex;
    const char*         _rgSurveySignals[_cSurveySignals];

    Vehicle*                _offlineVehicle;
    MultiSignalSpy*         _multiSpy;
    SurveyComplexItem*      _surveyItem;
    QGCMapPolygon*          _mapPolygon;
    QList<QGeoCoordinate>   _polyPoints;
};
