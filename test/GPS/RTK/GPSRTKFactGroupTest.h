#pragma once

#include "UnitTest.h"

class GPSRTKFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testFactInitialValues();
    void testBasePositionFacts();
    void testGnssRelativeFacts();
    void testSurveyInFacts();
    void testResetToDefaults();
};
