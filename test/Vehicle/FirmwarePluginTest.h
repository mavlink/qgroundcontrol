#pragma once

#include "UnitTest.h"

class FirmwarePluginTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _stableFirmwareNotifiedOncePerVersion_data();
    void _stableFirmwareNotifiedOncePerVersion();
};
