#pragma once

#include "UnitTest.h"

class Fact;

class AppSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _preferredFirmwareClassEnumFiltered();
    void _offlineEditingFirmwareClassEnumFiltered();

private:
    void _verifyFirmwareClassEnumFiltered(Fact *fact);
};
