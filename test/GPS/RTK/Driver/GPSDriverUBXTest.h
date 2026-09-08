#pragma once

#include "UnitTest.h"

class GPSDriverUBXTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _surveyRestart_data();
    void _surveyRestart();
    void _readFailure_data();
    void _readFailure();
};
