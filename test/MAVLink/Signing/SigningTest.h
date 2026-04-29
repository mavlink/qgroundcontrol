#pragma once

#include "UnitTest.h"

class SigningTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitSigning();
    void _testCheckSigningLinkId();
    void _testCreateSetupSigning();
    void _testCreateDisableSigning();
    void _testVerifySignature();
    void _testAcceptUnsignedCallbacks();
    void _testSetAcceptUnsignedCallback();
    void _testTryDetectKey();
    void _testTryDetectKeyUnsignedMessage();
    void _testTryDetectKeyHintCache();
    void _testAddRawKey();
    void _testMaxKeyLimit();
    void _testGenerateRandomHexKey();
    void _testRemoveKeyNonexistent();
    void _testKeyAtOutOfBounds();
    void _testSetMessageSigned();
    void _testSigningStatusString();
    void _testSigningStreamCount();
    void _testTimestampPersistence();
    void _testReplayProtection();
    void _testTryDetectKeySuspended();
    void _testTryDetectKeyCooldown();
    void _testChannelKeyName();
    void _testInitSigningWithPersistedTimestamp();
};
