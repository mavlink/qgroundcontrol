#pragma once

#include "UnitTest.h"

class VideoManagerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void init() override;

    void _testQmlReadyBeforeBackendReady();
    void _testBackendReadyBeforeQmlReady();
    void _testBackendInitFailure();
#else
    void _testGStreamerDisabled();
#endif
};
