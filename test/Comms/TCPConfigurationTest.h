#pragma once

#include "TestFixtures.h"

/// Unit tests for TCPConfiguration link settings.
class TCPConfigurationTest : public OfflineTest
{
    Q_OBJECT

public:
    TCPConfigurationTest() = default;

private slots:
    // Property tests
    void _hostPropertyTest();
    void _portPropertyTest();
    void _defaultValuesTest();

    // Configuration tests
    void _copyFromTest();
    void _copyConstructorTest();
    void _typeTest();
    void _settingsTest();
};
