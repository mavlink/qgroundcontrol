// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Host test for PortRegistry — the jni-free token->port map that fences port lifetime in JNI callbacks.
class PortRegistryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _allocateToken_isStrictlyIncreasingAndNonZero();
    void _lookup_unregisteredToken_isNull();
    void _registerThenLookup_resolvesPort();
    void _unregister_dropsLookup();
    void _clear_dropsAllLookups();
    void _register_overwritesStaleTokenMapping();
};
