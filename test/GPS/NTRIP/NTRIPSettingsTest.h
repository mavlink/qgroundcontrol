#pragma once

#include "UnitTest.h"

/// The saved NTRIP casters: what is kept of the stored list, saving and deleting one, and making one the caster of
/// the connection settings, with the certificate trusted for it.
class NTRIPSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _savedCastersLoad_data();
    void _savedCastersLoad();
    void _savedCastersSaveAndRemove();
    void _savedCasterBecomesConnection();
    void _savedCasterKeepsItsOwnCertificate_data();
    void _savedCasterKeepsItsOwnCertificate();
    void _pinNamesAnIPv6CasterInBrackets_data();
    void _pinNamesAnIPv6CasterInBrackets();
};
