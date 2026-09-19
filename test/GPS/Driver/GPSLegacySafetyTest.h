#pragma once

#include "PortableTest.h"

class GPSLegacySafetyTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _fixedBaseSurveyStatus_data();
    void _fixedBaseSurveyStatus();
    void _ashtechDefaultBaseSettings();
    void _ashtechSurveyReceipt_data();
    void _ashtechSurveyReceipt();
    void _ashtechSurveyReconfiguration_data();
    void _ashtechSurveyReconfiguration();
    void _femtoPositionOutput_data();
    void _femtoPositionOutput();
};
