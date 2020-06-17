/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TransectStyleComplexItemTestBase.h"
#include "MultiSignalSpy.h"
#include "SurveyComplexItem.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"

#include <QGeoCoordinate>

/// Unit test for SurveyComplexItem
class SurveyComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT
    
public:
    SurveyComplexItemTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;
    
#if 1
private slots:
    void _testDirty(void);
    void _testGridAngle(void);
    void _testEntryLocation(void);
    void _testItemGeneration(void);
    void _testItemCount(void);
    void _testHoverCaptureItemGeneration(void);
#else
    // Handy mechanism to to a single test
private slots:
    void _testItemCount(void);
private:
    void _testDirty(void);
    void _testGridAngle(void);
    void _testEntryLocation(void);
    void _testItemGeneration(void);
    void _testHoverCaptureItemGeneration(void);
#endif

private:
    double          _clampGridAngle180(double gridAngle);
    QList<MAV_CMD>  _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void            _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate, const QList<MAV_CMD>& expectedCommands);

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

    MultiSignalSpy*         _multiSpy =             nullptr;
    SurveyComplexItem*      _surveyItem =           nullptr;
    QGCMapPolygon*          _mapPolygon =           nullptr;
    QList<QGeoCoordinate>   _polyVertices;

    static const int _expectedTransectCount = 2;
};
