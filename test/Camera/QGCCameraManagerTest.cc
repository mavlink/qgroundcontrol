#include "QGCCameraManagerTest.h"
#include "QGCCameraManager.h"
#include "CameraMetaData.h"

#include <QtTest/QTest>

void QGCCameraManagerTest::_testCameraList()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    QVERIFY(!cameraList.isEmpty());

    qDeleteAll(cameraList);
}
