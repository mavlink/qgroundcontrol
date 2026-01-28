#pragma once

#include "TestFixtures.h"

/// Unit tests for MAVLink signing functionality.
/// Tests signing initialization, callbacks, and message verification.
class SigningTest : public OfflineTest
{
    Q_OBJECT

public:
    SigningTest() = default;

private slots:
    void cleanup() override;

    // initSigning tests
    void _testInitSigningWithKey();
    void _testInitSigningWithEmptyKey();
    void _testInitSigningKeyHashing();
    void _testInitSigningWithByteArray();
    void _testInitSigningNoCallbackFails();
    void _testInitSigningDifferentChannels();
    void _testInitSigningReinitialize();

    // Callback tests
    void _testSecureCallbackAlwaysTrue();
    void _testInsecureCallbackRadioStatus();
    void _testInsecureCallbackOtherMessages();

    // checkSigningLinkId tests
    void _testCheckSigningLinkIdValid();
    void _testCheckSigningLinkIdNoSigning();

    // createSetupSigning tests
    void _testCreateSetupSigningTimestamp();
    void _testCreateSetupSigningTarget();
    void _testCreateSetupSigningSecretKey();
    void _testCreateSetupSigningNoSigning();

    // Signing state tests
    void _testSigningFlags();
    void _testSigningLinkId();
};
