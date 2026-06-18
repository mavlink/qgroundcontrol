// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "PortRegistryTest.h"

#include "AndroidSerialPortRegistry.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(PortRegistryTest, TestLabel::Unit, TestLabel::Comms)

namespace {
// The registry only stores/returns the pointer (never dereferences), so opaque non-null values stand in.
AndroidSerialPort* fakePort(quintptr id)
{
    return reinterpret_cast<AndroidSerialPort*>(id);
}
}  // namespace

void PortRegistryTest::_allocateToken_isStrictlyIncreasingAndNonZero()
{
    const PortRegistry::Token first = PortRegistry::allocateToken();
    const PortRegistry::Token second = PortRegistry::allocateToken();
    const PortRegistry::Token third = PortRegistry::allocateToken();

    QVERIFY(first != 0);
    QVERIFY(second > first);
    QVERIFY(third > second);
}

void PortRegistryTest::_lookup_unregisteredToken_isNull()
{
    PortRegistry::LookupGuard guard(PortRegistry::allocateToken());
    QVERIFY(!guard);
    QCOMPARE(guard.port(), nullptr);
}

void PortRegistryTest::_registerThenLookup_resolvesPort()
{
    const PortRegistry::Token token = PortRegistry::allocateToken();
    AndroidSerialPort* const port = fakePort(0x1000);
    PortRegistry::registerPort(token, port);

    {
        PortRegistry::LookupGuard guard(token);
        QVERIFY(guard);
        QCOMPARE(guard.port(), port);
    }

    PortRegistry::unregisterPort(token);
}

void PortRegistryTest::_unregister_dropsLookup()
{
    const PortRegistry::Token token = PortRegistry::allocateToken();
    PortRegistry::registerPort(token, fakePort(0x2000));
    PortRegistry::unregisterPort(token);

    PortRegistry::LookupGuard guard(token);
    QVERIFY(!guard);
}

void PortRegistryTest::_clear_dropsAllLookups()
{
    const PortRegistry::Token a = PortRegistry::allocateToken();
    const PortRegistry::Token b = PortRegistry::allocateToken();
    PortRegistry::registerPort(a, fakePort(0x3000));
    PortRegistry::registerPort(b, fakePort(0x4000));

    PortRegistry::clear();

    {
        PortRegistry::LookupGuard guardA(a);
        QVERIFY(!guardA);
    }
    {
        PortRegistry::LookupGuard guardB(b);
        QVERIFY(!guardB);
    }
}

void PortRegistryTest::_register_overwritesStaleTokenMapping()
{
    const PortRegistry::Token token = PortRegistry::allocateToken();
    PortRegistry::registerPort(token, fakePort(0x5000));
    AndroidSerialPort* const replacement = fakePort(0x6000);
    PortRegistry::registerPort(token, replacement);

    {
        PortRegistry::LookupGuard guard(token);
        QCOMPARE(guard.port(), replacement);
    }

    PortRegistry::unregisterPort(token);
}
