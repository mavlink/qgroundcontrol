#pragma once

#ifdef QGC_PORTABLE_TEST
#include <QtCore/QObject>
#include <QtTest/QTest>

class PortableTest : public QObject
{
    Q_OBJECT

private slots:

    void init() { QTest::failOnWarning(); }
};

#define QGC_REGISTER_PORTABLE_TEST(className, ...) QTEST_GUILESS_MAIN(className)
#else
#include "UnitTest.h"

using PortableTest = UnitTest;
#define QGC_REGISTER_PORTABLE_TEST(className, ...) UT_REGISTER_TEST_LIGHTWEIGHT(className, __VA_ARGS__)
#endif
