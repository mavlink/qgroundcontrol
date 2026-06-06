#pragma once

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

/// Verify that calling setter emits the notify signal exactly once and that
/// the getter returns the new value. Failures report at the call site.
///
/// @param obj    Pointer to object under test (QObject-derived)
/// @param Signal Pointer-to-member signal, e.g. &MyClass::fooChanged
/// @param setter Pointer-to-member setter,  e.g. &MyClass::setFoo
/// @param getter Pointer-to-member getter,  e.g. &MyClass::foo
/// @param value  New value to pass to the setter
#define QVERIFY_PROPERTY_SET(obj, Signal, setter, getter, value)          \
    do {                                                                   \
        QSignalSpy _pth_spy((obj), (Signal));                              \
        (obj->*(setter))(value);                                           \
        QCOMPARE(_pth_spy.count(), 1);                                     \
        QCOMPARE((obj->*(getter))(), (value));                             \
    } while (false)

/// Variant with a custom expected emission count (e.g. 0 for no-op sets).
///
/// @param obj           Pointer to object under test
/// @param Signal        Pointer-to-member signal
/// @param setter        Pointer-to-member setter
/// @param getter        Pointer-to-member getter
/// @param value         New value to pass to the setter
/// @param expectedCount Expected signal emission count after the setter call
#define QVERIFY_PROPERTY_SET_N(obj, Signal, setter, getter, value, expectedCount) \
    do {                                                                           \
        QSignalSpy _pth_spy((obj), (Signal));                                      \
        (obj->*(setter))(value);                                                   \
        QCOMPARE(_pth_spy.count(), (expectedCount));                               \
        if ((expectedCount) > 0) {                                                 \
            QCOMPARE((obj->*(getter))(), (value));                                 \
        }                                                                          \
    } while (false)
