#pragma once

#include "UnitTest.h"

/// Tests for the credits of the area that an Esri map shows: the reader of Esri's lists of contributors, and what
/// QGCMapEngineManager answers a view from them
class EsriAttributionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _creditsOfAreaAndZoom_data();
    void _creditsOfAreaAndZoom();
    void _entriesNotUnderstoodAreLeftOut();
    void _answerIsEmptyUntilListArrives();
    void _listIsFetchedOnce();
    void _fetchWithoutList_data();
    void _fetchWithoutList();
    void _questionWithoutAnswerAsksNoServer_data();
    void _questionWithoutAnswerAsksNoServer();
    void _viewOfMapIsCreditedForItsArea_data();
    void _viewOfMapIsCreditedForItsArea();
};
