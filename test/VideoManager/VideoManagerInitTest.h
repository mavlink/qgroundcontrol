#pragma once

#include "UnitTest.h"

class VideoManagerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testQmlReadyBeforeBackendReady();
    void _testBackendReadyBeforeQmlReady();
    void _testBackendInitFailure();
};
