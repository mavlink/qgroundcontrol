#pragma once

#ifdef QGC_GPS_STANDALONE_TEST
#include <QtTest/QTest>
class RTCMFramerTest : public QObject
#else
#include "UnitTest.h"
class RTCMFramerTest : public UnitTest
#endif
{
    Q_OBJECT

private slots:
    void _frameViewAndReset();
    void _implicitAdvance_data();
    void _implicitAdvance();
};
