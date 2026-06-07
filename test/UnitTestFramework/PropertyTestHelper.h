#pragma once

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#define QVERIFY_PROPERTY_SET(obj, Signal, setter, getter, value)          \
    do {                                                                   \
        QSignalSpy _pth_spy((obj), (Signal));                              \
        (obj->*(setter))(value);                                           \
        QCOMPARE(_pth_spy.count(), 1);                                     \
        QCOMPARE((obj->*(getter))(), (value));                             \
    } while (false)

#define QVERIFY_PROPERTY_SET_N(obj, Signal, setter, getter, value, expectedCount) \
    do {                                                                           \
        QSignalSpy _pth_spy((obj), (Signal));                                      \
        (obj->*(setter))(value);                                                   \
        QCOMPARE(_pth_spy.count(), (expectedCount));                               \
        if ((expectedCount) > 0) {                                                 \
            QCOMPARE((obj->*(getter))(), (value));                                 \
        }                                                                          \
    } while (false)
