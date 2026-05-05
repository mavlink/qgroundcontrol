#include "OnboardLogDownloadTest.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <array>
#include <chrono>
#include <limits>

#include "ArduCopterFirmwarePlugin.h"
#include "FTPManager.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "FtpDownloadSession.h"
#include "FtpListingParser.h"
#include "FtpListingSession.h"
#include "FtpTransport.h"
#include "LogProtocolDownloadBatchSession.h"
#include "LogProtocolDownloadSession.h"
#include "LogProtocolTransport.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MockLinkFTP.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "OnboardLogController.h"
#include "OnboardLogEntry.h"
#include "OnboardLogFileName.h"
#include "OnboardLogModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void OnboardLogDownloadTest::_downloadTest()
{
    // VehicleTest::init() already connects the mock link
    OnboardLogController controller;
    MultiSignalSpy multiSpyLogDownloadController;
    QVERIFY(multiSpyLogDownloadController.init(&controller));
    controller.refresh();
    QVERIFY(multiSpyLogDownloadController.waitForSignal("busyChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController.clearAllSignals();
    if (controller._active && controller._active->requestingList()) {
        QVERIFY(multiSpyLogDownloadController.waitForSignal("busyChanged", TestTimeout::longMs()));
        QVERIFY(controller._active);
        QCOMPARE(controller._active->requestingList(), false);
    }
    multiSpyLogDownloadController.clearAllSignals();
    OnboardLogModel* const model = controller.sourceModel();
    QVERIFY(model);
    model->value<OnboardLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller.download(downloadTo);
    QVERIFY(multiSpyLogDownloadController.waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController.clearAllSignals();
    if (controller.downloadingLogs()) {
        QVERIFY(multiSpyLogDownloadController.waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
        QCOMPARE(controller.downloadingLogs(), false);
    }
    QCOMPARE(controller.batchProgress(), 1.);
    multiSpyLogDownloadController.clearAllSignals();
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void) QFile::remove(downloadFile);
}

UT_REGISTER_TEST(OnboardLogDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
