#pragma once

#ifdef QGC_GPS_STANDALONE_TEST
#include <QtTest/QTest>
class RTCMParserTest : public QObject
#else
#include "UnitTest.h"
class RTCMParserTest : public UnitTest
#endif
{
    Q_OBJECT

private slots:
    void _frameAccess_data();
    void _frameAccess();
    void _partialAndReset();
    void _whitelist();
    void _crcCompatibility();
};
