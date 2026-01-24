#include "GeoTagControllerTest.h"
#include "TestHelpers.h"
#include "QtTestExtensions.h"
#include "GeoTagController.h"
#include "GeoTagWorker.h"
#include "MultiSignalSpy.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Setup and Cleanup
// ============================================================================

void GeoTagControllerTest::init()
{
    UnitTest::init();

    // Create unique temp directories for this test run
    _tempDir = createTempDir();
    QVERIFY2(_tempDir, "Failed to create temp directory");

    _imageDirPath = _tempDir->path();
    _taggedDirPath = _imageDirPath + "/" + kTaggedDirName;

    QDir().mkpath(_taggedDirPath);
}

void GeoTagControllerTest::cleanup()
{
    // QTemporaryDir auto-cleans, but we clean our subdirectory explicitly
    _cleanupDirectory(_taggedDirPath);
    QDir().rmdir(_taggedDirPath);

    _tempDir = nullptr;  // Will be deleted by UnitTest::cleanup()

    UnitTest::cleanup();
}

void GeoTagControllerTest::_cleanupDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    const QStringList files = dir.entryList(QDir::Files);
    for (const QString &fileName : files) {
        dir.remove(fileName);
    }
}

bool GeoTagControllerTest::_setupTestImages(int imageCount)
{
    // Ensure tagged directory exists
    QDir imageDir(_imageDirPath);
    if (!imageDir.exists(kTaggedDirName)) {
        if (!imageDir.mkdir(kTaggedDirName)) {
            return false;
        }
    } else {
        _cleanupDirectory(_taggedDirPath);
    }

    QFile sourceFile(":/unittest/DSCN0010.jpg");
    for (int i = 0; i < imageCount; ++i) {
        const QString destPath = _imageDirPath + QStringLiteral("/geotag_temp_image_%1.jpg").arg(i);
        if (!sourceFile.copy(destPath)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Controller Tests
// ============================================================================

void GeoTagControllerTest::_controllerCreationTest()
{
    GeoTagController controller(this);

    // Initial state should be empty/default
    QVERIFY(controller.logFile().isEmpty());
    QVERIFY(controller.imageDirectory().isEmpty());
    QCOMPARE_EQ(controller.progress(), 0.0);
    QVERIFY(controller.errorMessage().isEmpty());
}

void GeoTagControllerTest::_controllerConfigurationTest()
{
    GeoTagController controller(this);

    QSignalSpy logFileSpy(&controller, &GeoTagController::logFileChanged);
    QSignalSpy imageDirSpy(&controller, &GeoTagController::imageDirectoryChanged);
    QSignalSpy saveDirSpy(&controller, &GeoTagController::saveDirectoryChanged);

    controller.setLogFile(":/unittest/SampleULog.ulg");
    controller.setImageDirectory(_imageDirPath);
    controller.setSaveDirectory(_taggedDirPath);

    QCOMPARE_EQ(logFileSpy.count(), 1);
    QCOMPARE_EQ(imageDirSpy.count(), 1);
    QCOMPARE_EQ(saveDirSpy.count(), 1);

    QGC_VERIFY_NOT_EMPTY(controller.logFile());
    QGC_VERIFY_NOT_EMPTY(controller.imageDirectory());
    QGC_VERIFY_NOT_EMPTY(controller.saveDirectory());
}

void GeoTagControllerTest::_controllerTaggingProgressTest()
{
    QVERIFY2(_setupTestImages(kDefaultImageCount), "Failed to set up test images");

    GeoTagController controller(this);
    controller.setLogFile(":/unittest/SampleULog.ulg");
    controller.setImageDirectory(_imageDirPath + "/");
    controller.setSaveDirectory(_taggedDirPath);

    MultiSignalSpy spy;
    QVERIFY(spy.init(&controller));

    controller.startTagging();

    QVERIFY2(spy.waitForSignal("progressChanged", TestHelpers::kDefaultTimeoutMs),
             "Timeout waiting for progress signal");
    QCOMPARE_GT(controller.progress(), 0.0);
    QGC_VERIFY_EMPTY(controller.errorMessage());
}

void GeoTagControllerTest::_controllerInvalidLogFileTest()
{
    QVERIFY2(_setupTestImages(5), "Failed to set up test images");

    GeoTagController controller(this);

    // Set invalid log file
    controller.setLogFile("/nonexistent/path/to/log.ulg");
    controller.setImageDirectory(_imageDirPath + "/");
    controller.setSaveDirectory(_taggedDirPath);

    QSignalSpy errorSpy(&controller, &GeoTagController::errorMessageChanged);
    QGC_VERIFY_SPY_VALID(errorSpy);

    controller.startTagging();

    // Should get an error signal
    QVERIFY(errorSpy.wait(TestHelpers::kDefaultTimeoutMs));
    QGC_VERIFY_NOT_EMPTY(controller.errorMessage());
}

void GeoTagControllerTest::_controllerInvalidImageDirTest()
{
    GeoTagController controller(this);

    controller.setLogFile(":/unittest/SampleULog.ulg");
    controller.setImageDirectory("/nonexistent/image/directory/");
    controller.setSaveDirectory(_taggedDirPath);

    QSignalSpy errorSpy(&controller, &GeoTagController::errorMessageChanged);
    QGC_VERIFY_SPY_VALID(errorSpy);

    controller.startTagging();

    // Should get an error signal for invalid directory
    QVERIFY(errorSpy.wait(TestHelpers::kDefaultTimeoutMs));
    QGC_VERIFY_NOT_EMPTY(controller.errorMessage());
}

// ============================================================================
// Worker Tests
// ============================================================================

void GeoTagControllerTest::_workerProcessTest()
{
    QVERIFY2(_setupTestImages(kDefaultImageCount), "Failed to set up test images");

    GeoTagWorker worker(this);
    worker.setLogFile(":/unittest/SampleULog.ulg");
    worker.setImageDirectory(_imageDirPath + "/");
    worker.setSaveDirectory(_taggedDirPath);

    QVERIFY2(worker.process(), "GeoTagWorker::process() failed");
}

void GeoTagControllerTest::_workerEmptyImageDirTest()
{
    // Ensure tagged directory exists
    QDir imageDir(_imageDirPath);
    if (!imageDir.exists(kTaggedDirName)) {
        QVERIFY(imageDir.mkdir(kTaggedDirName));
    }
    _cleanupDirectory(_taggedDirPath);

    GeoTagWorker worker(this);
    worker.setLogFile(":/unittest/SampleULog.ulg");
    worker.setImageDirectory(_imageDirPath + "/");
    worker.setSaveDirectory(_taggedDirPath);

    // Process should handle empty directory gracefully
    // (may return false or true with 0 images processed)
    worker.process();
}
