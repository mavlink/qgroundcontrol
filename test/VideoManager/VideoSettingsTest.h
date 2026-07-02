#pragma once

#include "UnitTest.h"

class VideoSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testNetworkVideoUrlValidation();
    void _testNetworkVideoUrlValidation_data();
    void _testAuthenticatedTransportPolicy();
    void _testOriginValidation();
    void _testSecretFileValidation();
    void _testSecretFileRejectsAliases();
};
