#pragma once

#include "UnitTest.h"

/// Base of the GStreamer test classes. Every test runs against a fully initialized GStreamer, as the app does once
/// VideoBackend is up, and skips when GStreamer can't be initialized.
class GStreamerTestBase : public UnitTest
{
    Q_OBJECT

protected slots:
#ifdef QGC_GST_STREAMING
    void initTestCase() override;
    void init() override;
#endif

private slots:
#ifndef QGC_GST_STREAMING
    void _testGStreamerDisabled();
#endif
};
