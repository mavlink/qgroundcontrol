#pragma once

#include "PortableTest.h"

class UomRegistrationCoreTest final : public PortableTest
{
    Q_OBJECT
private slots:
    void _cryptoContract();
    void _officialProfilesOnly();
    void _parser_data();
    void _parser();
    void _durableRecoveryAndScopeIsolation();
    void _schemaMigrationDoesNotInventAcknowledgements();
    void _profileMigrationPreservesIdentityAndHistory();
    void _encryptedWireAndPersistentLimit();
    void _cancellationHasNoHiddenSerialAndNoLateResult();
    void _memoryCredentialsAreExplicitAndRemovable();
    void _unavailableKeyringRetainsMemory();
};
