#pragma once

#include <QtTest/QTest>

/// Compare floating point values with configurable epsilon
#define QCOMPARE_FUZZY(actual, expected, epsilon)                                             \
    QVERIFY2(qAbs((actual) - (expected)) <= (epsilon),                                        \
             qPrintable(QString("Values differ: actual=%1, expected=%2, diff=%3, epsilon=%4") \
                            .arg(actual)                                                      \
                            .arg(expected)                                                    \
                            .arg(qAbs((actual) - (expected)))                                 \
                            .arg(epsilon)))
