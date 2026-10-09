#pragma once

#include <memory>

#include "Fixtures/RAIIFixtures.h"
#include "QmlUITestBase.h"

/// UI tests for the NTRIP sections of the RTK Corrections settings page.
///
/// Navigates to the page through the real settings left-nav and verifies the
/// enable/disable gating of the hand-written NTRIP controls: the Browse button
/// and Connect button follow the server host address, and the
/// accept-self-signed-certificate switch follows the use-TLS switch. Gating is
/// driven through the underlying Facts so the test never touches the network or
/// a real caster.
class NTRIPSettingsUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    NTRIPSettingsUITest() = default;

private slots:
    void init() override;
    void cleanup() override;
    void _testControlsGated();

private:
    bool _navigateToNtripPage();

    std::unique_ptr<TestFixtures::SettingsFixture> _settings;
};
