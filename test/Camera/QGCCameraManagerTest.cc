#include "QGCCameraManagerTest.h"

#include "CameraMetaData.h"
#include "QGCCameraManager.h"

void QGCCameraManagerTest::_testCameraList()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();
    QVERIFY(!cameraList.isEmpty());
    qDeleteAll(cameraList);
}

UT_REGISTER_TEST(QGCCameraManagerTest, TestLabel::Integration, TestLabel::Vehicle)
