/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCameraManagerTest.h"
#include "QGCCameraManager.h"

#include <QtTest/QTest>

void QGCCameraManagerTest::_testCameraList()
{
    const QList<CameraMetaData*> cameraList = QGCCameraManager::_parseCameraMetaData(QStringLiteral(":/json/CameraMetaData.json"));

    QVERIFY(!cameraList.isEmpty());

    qDeleteAll(cameraList);
}
