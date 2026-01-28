#pragma once

#include "TestFixtures.h"

/// Unit tests for LinkConfiguration base class functionality.
/// Uses UDPConfiguration as the concrete implementation for testing.
class LinkConfigurationTest : public OfflineTest
{
    Q_OBJECT

public:
    LinkConfigurationTest() = default;

private slots:
    // Property tests
    void _namePropertyTest();
    void _dynamicPropertyTest();
    void _autoConnectPropertyTest();
    void _highLatencyPropertyTest();

    // Factory tests
    void _createSettingsTest();
    void _duplicateSettingsTest();

    // Copy tests
    void _copyFromTest();
    void _copyConstructorTest();

    // Settings root
    void _settingsRootTest();
};
