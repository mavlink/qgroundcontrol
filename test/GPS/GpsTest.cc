/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GpsTest.h"
#include "GPSProvider.h"
#include "RTCMMavlink.h"

#include <QtTest/QTest>

void GpsTest::_testGpsRTCM()
{
    RTCMMavlink *rtcm = new RTCMMavlink(this);

    const int fakeMsgLengths[3] = {30, 170, 240};
    uint8_t* const fakeData = new uint8_t[fakeMsgLengths[2]];
    for (int i = 0; i < 3; ++i) {
        const QByteArray message(reinterpret_cast<const char*>(fakeData), fakeMsgLengths[i]);
        rtcm->RTCMDataUpdate(message);
    }

    delete[] fakeData;
}
