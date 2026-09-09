#pragma once

#include "UnitTest.h"

class GPSDiagnosticsUITest : public UnitTest
{
    Q_OBJECT

private slots:
    void _mountpointNarrowLayout();
    void _positionSelection();
    void _recordingAndExport();
    void _observationDetails();
    void _configurationReadback();
};
