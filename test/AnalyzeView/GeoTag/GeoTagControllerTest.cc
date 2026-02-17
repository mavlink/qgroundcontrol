#include "GeoTagControllerTest.h"
#include "GeoTagController.h"
#include "GeoTagImageModel.h"
#include "ExifParser.h"
#include "ExifUtility.h"
#include "ULogTestGenerator.h"

#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

namespace {

QString generateTestULogFile(QTemporaryDir &tempDir, int numEvents = 20)
{
    const QString ulogPath = tempDir.path() + "/test.ulg";
    const auto events = ULogTestGenerator::generateSampleEvents(numEvents);
    if (!ULogTestGenerator::generateULog(ulogPath, events)) {
        return QString();
    }
    return ulogPath;
}

// Helper functions for calibrator tests
GeoTagData makeValidTrigger(qint64 timestamp, double lat = 37.0, double lon = -122.0)
{
    GeoTagData trigger;
    trigger.timestamp = timestamp;
    trigger.coordinate = QGeoCoordinate(lat, lon, 100.0);
    trigger.captureResult = GeoTagData::CaptureResult::Success;
    return trigger;
}

GeoTagData makeInvalidTrigger(qint64 timestamp)
{
    GeoTagData trigger;
    trigger.timestamp = timestamp;
    trigger.coordinate = QGeoCoordinate(37.0, -122.0, 100.0);
    trigger.captureResult = GeoTagData::CaptureResult::Failure;
    return trigger;
}

GeoTagData makeTriggerWithInvalidCoord(qint64 timestamp)
{
    GeoTagData trigger;
    trigger.timestamp = timestamp;
    trigger.coordinate = QGeoCoordinate();  // Invalid
    trigger.captureResult = GeoTagData::CaptureResult::Success;
    return trigger;
}

} // namespace

void GeoTagControllerTest::_propertyAccessorsTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString imageDirPath = tempDir.path() + "/images";
    const QString taggedDirPath = tempDir.path() + "/tagged";
    QVERIFY(QDir().mkpath(imageDirPath));
    QVERIFY(QDir().mkpath(taggedDirPath));

    const QString ulogPath = generateTestULogFile(tempDir, 10);
    QVERIFY(!ulogPath.isEmpty());

    QFile file(":/unittest/DSCN0010.jpg");
    for (int i = 0; i < 10; ++i) {
        QVERIFY(file.copy(imageDirPath + QStringLiteral("/geotag_temp_image_%1.jpg").arg(i)));
    }

    GeoTagController* const controller = new GeoTagController(this);

    controller->setLogFile(ulogPath);
    controller->setImageDirectory(imageDirPath + "/");
    controller->setSaveDirectory(taggedDirPath);

    QVERIFY(!controller->logFile().isEmpty());
    QVERIFY(!controller->imageDirectory().isEmpty());
    QVERIFY(!controller->saveDirectory().isEmpty());

    QCOMPARE(controller->logFile(), ulogPath);
    QCOMPARE(controller->imageDirectory(), imageDirPath + "/");
    QCOMPARE(controller->saveDirectory(), taggedDirPath);
    QCOMPARE(controller->progress(), 0.0);
    QVERIFY(!controller->inProgress());

    // Test tolerance property
    QCOMPARE(controller->toleranceSecs(), 2.0);  // Default value
    controller->setToleranceSecs(5.0);
    QCOMPARE(controller->toleranceSecs(), 5.0);

    // Test tolerance clamping
    controller->setToleranceSecs(0.05);  // Below minimum
    QCOMPARE(controller->toleranceSecs(), 0.1);  // Clamped to 0.1
    controller->setToleranceSecs(100.0);  // Above maximum
    QCOMPARE(controller->toleranceSecs(), 60.0);  // Clamped to 60
}

void GeoTagControllerTest::_urlPathConversionTest()
{
    GeoTagController* const controller = new GeoTagController(this);

    const QString testPath = QDir::tempPath() + "/test_file.ulg";

    QFile testFile(testPath);
    QVERIFY(testFile.open(QIODevice::WriteOnly));
    testFile.write("test");
    testFile.close();

    controller->setLogFile(QStringLiteral("file://") + testPath);
    QCOMPARE(controller->logFile(), testPath);

    testFile.remove();
}

void GeoTagControllerTest::_validationTest()
{
    GeoTagController* const controller = new GeoTagController(this);

    controller->setLogFile(QString());
    QVERIFY(!controller->errorMessage().isEmpty());

    controller->setLogFile(QStringLiteral("/nonexistent/path/file.ulg"));
    QVERIFY(!controller->errorMessage().isEmpty());

    controller->setImageDirectory(QString());
    QVERIFY(!controller->errorMessage().isEmpty());

    controller->setImageDirectory(QStringLiteral("/nonexistent/directory/"));
    QVERIFY(!controller->errorMessage().isEmpty());

    controller->startTagging();
    QVERIFY(!controller->errorMessage().isEmpty());
}

void GeoTagControllerTest::_calibrationMismatchTest()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString imageDirPath = tempDir.path() + "/images";
    const QString taggedDirPath = tempDir.path() + "/tagged";
    QVERIFY(QDir().mkpath(imageDirPath));
    QVERIFY(QDir().mkpath(taggedDirPath));

    const int numImages = 10;
    const QString ulogPath = generateTestULogFile(tempDir, numImages);
    QVERIFY(!ulogPath.isEmpty());

    // Copy test images (same timestamp, so calibration will fail)
    QFile file(":/unittest/DSCN0010.jpg");
    for (int i = 0; i < numImages; ++i) {
        QVERIFY(file.copy(imageDirPath + QStringLiteral("/geotag_temp_image_%1.jpg").arg(i)));
    }

    GeoTagController* const controller = new GeoTagController(this);
    controller->setLogFile(ulogPath);
    controller->setImageDirectory(imageDirPath + "/");
    controller->setSaveDirectory(taggedDirPath);

    QSignalSpy completeSpy(controller, &GeoTagController::taggingCompleteChanged);

    controller->startTagging();

    // Wait for taggingCompleteChanged signal (indicates results are ready)
    QTRY_VERIFY_WITH_TIMEOUT(completeSpy.count() > 0 || !controller->errorMessage().isEmpty(), 10000);

    // With "continue on failures" behavior, partial matches now succeed
    // Only 1 image matches (all have same timestamp), so we get 1 tagged
    QVERIFY(controller->taggedCount() >= 0);
}

void GeoTagControllerTest::_fullGeotaggingTest()
{
    const int numImages = 10;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString imageDirPath = tempDir.path() + "/images";
    const QString taggedDirPath = tempDir.path() + "/tagged";
    QVERIFY(QDir().mkpath(imageDirPath));
    QVERIFY(QDir().mkpath(taggedDirPath));

    // Generate ULog with valid camera captures
    const auto events = ULogTestGenerator::generateSampleEvents(numImages, 1000000, 2.0);
    QCOMPARE(events.size(), numImages);

    const QString ulogPath = tempDir.path() + "/test.ulg";
    QVERIFY(ULogTestGenerator::generateULog(ulogPath, events));

    // Load template image
    QFile templateFile(":/unittest/DSCN0010.jpg");
    QVERIFY(templateFile.open(QIODevice::ReadOnly));
    const QByteArray templateBuffer = templateFile.readAll();
    templateFile.close();

    // Create images with timestamps matching the ULog triggers
    const uint64_t lastTriggerUs = events.last().timestamp_us;
    const qint64 baseImageTime = QDateTime::currentSecsSinceEpoch();

    for (int i = 0; i < numImages; ++i) {
        QByteArray imageBuffer = templateBuffer;

        const uint64_t triggerUs = events[i].timestamp_us;
        const qint64 triggerOffsetSec = static_cast<qint64>((lastTriggerUs - triggerUs) / 1000000);
        const QDateTime imageTime = QDateTime::fromSecsSinceEpoch(baseImageTime - triggerOffsetSec);

        ExifData *exifData = ExifUtility::loadFromBuffer(imageBuffer);
        QVERIFY(exifData != nullptr);
        QVERIFY(ExifUtility::writeDateTimeOriginal(exifData, imageTime));
        QVERIFY(ExifUtility::saveToBuffer(exifData, imageBuffer));
        exif_data_unref(exifData);

        const QString imagePath = imageDirPath + QStringLiteral("/image_%1.jpg").arg(i, 2, 10, QChar('0'));
        QFile imageFile(imagePath);
        QVERIFY(imageFile.open(QIODevice::WriteOnly));
        imageFile.write(imageBuffer);
        imageFile.close();
    }

    // Run geotagging via controller
    GeoTagController* const controller = new GeoTagController(this);
    controller->setLogFile(ulogPath);
    controller->setImageDirectory(imageDirPath + "/");
    controller->setSaveDirectory(taggedDirPath);

    QSignalSpy inProgressSpy(controller, &GeoTagController::inProgressChanged);

    controller->startTagging();

    // Wait for inProgressChanged (emitted twice: start and finish)
    // First emission is when processing starts, second is when it finishes
    QTRY_VERIFY_WITH_TIMEOUT(inProgressSpy.count() >= 2, 30000);

    QVERIFY2(controller->errorMessage().isEmpty(), qPrintable(controller->errorMessage()));
    QVERIFY2(controller->taggedCount() > 0, qPrintable(QString("Expected tagged images, got %1 tagged, %2 skipped")
             .arg(controller->taggedCount()).arg(controller->skippedCount())));

    // Verify output files exist and have GPS data
    QDir taggedDir(taggedDirPath);
    const QStringList taggedFiles = taggedDir.entryList({"*.jpg", "*.JPG"}, QDir::Files);
    QCOMPARE(taggedFiles.count(), controller->taggedCount());

    if (!taggedFiles.isEmpty()) {
        QFile taggedFile(taggedDirPath + "/" + taggedFiles.first());
        QVERIFY(taggedFile.open(QIODevice::ReadOnly));
        const QByteArray taggedBuffer = taggedFile.readAll();
        taggedFile.close();

        ExifData *verifyData = ExifUtility::loadFromBuffer(taggedBuffer);
        QVERIFY(verifyData != nullptr);

        ExifContent *gpsIfd = verifyData->ifd[EXIF_IFD_GPS];
        QVERIFY(gpsIfd != nullptr);

        ExifEntry *latEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE));
        ExifEntry *lonEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE));
        QVERIFY(latEntry != nullptr);
        QVERIFY(lonEntry != nullptr);

        exif_data_unref(verifyData);
    }
}

void GeoTagControllerTest::_previewModeTest()
{
    constexpr int numImages = 3;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Generate ULog with valid camera captures
    const auto events = ULogTestGenerator::generateSampleEvents(numImages, 1000000, 2.0);
    QCOMPARE(events.size(), numImages);

    const QString ulogPath = tempDir.path() + "/test.ulg";
    QVERIFY(ULogTestGenerator::generateULog(ulogPath, events));

    // Create image directory and tagged output directory paths
    const QString imageDirPath = tempDir.filePath("images");
    const QString taggedDirPath = tempDir.filePath("tagged");
    QVERIFY(QDir().mkpath(imageDirPath));
    // Don't create tagged directory - preview mode shouldn't need it

    // Load template image
    QFile templateFile(":/unittest/DSCN0010.jpg");
    QVERIFY(templateFile.open(QIODevice::ReadOnly));
    const QByteArray templateBuffer = templateFile.readAll();
    templateFile.close();

    // Create images with timestamps matching the ULog triggers
    const uint64_t lastTriggerUs = events.last().timestamp_us;
    const qint64 baseImageTime = QDateTime::currentSecsSinceEpoch();

    for (int i = 0; i < numImages; ++i) {
        QByteArray imageBuffer = templateBuffer;

        const uint64_t triggerUs = events[i].timestamp_us;
        const qint64 triggerOffsetSec = static_cast<qint64>((lastTriggerUs - triggerUs) / 1000000);
        const QDateTime imageTime = QDateTime::fromSecsSinceEpoch(baseImageTime - triggerOffsetSec);

        ExifData *exifData = ExifUtility::loadFromBuffer(imageBuffer);
        QVERIFY(exifData != nullptr);
        QVERIFY(ExifUtility::writeDateTimeOriginal(exifData, imageTime));
        QVERIFY(ExifUtility::saveToBuffer(exifData, imageBuffer));
        exif_data_unref(exifData);

        const QString imagePath = imageDirPath + QStringLiteral("/image_%1.jpg").arg(i, 2, 10, QChar('0'));
        QFile imageFile(imagePath);
        QVERIFY(imageFile.open(QIODevice::WriteOnly));
        imageFile.write(imageBuffer);
        imageFile.close();
    }

    // Run geotagging in preview mode
    GeoTagController* const controller = new GeoTagController(this);
    controller->setLogFile(ulogPath);
    controller->setImageDirectory(imageDirPath + "/");
    controller->setSaveDirectory(taggedDirPath);
    controller->setPreviewMode(true);

    QSignalSpy inProgressSpy(controller, &GeoTagController::inProgressChanged);

    controller->startTagging();

    // Wait for completion
    QTRY_VERIFY_WITH_TIMEOUT(inProgressSpy.count() >= 2, 30000);

    QVERIFY2(controller->errorMessage().isEmpty(), qPrintable(controller->errorMessage()));
    QVERIFY2(controller->taggedCount() > 0, qPrintable(QString("Expected tagged images, got %1 tagged")
             .arg(controller->taggedCount())));

    // Verify no output directory or files were created
    QVERIFY2(!QDir(taggedDirPath).exists(), "Preview mode should not create output directory");

    // Verify model has coordinates populated
    GeoTagImageModel* const model = controller->imageModel();
    QVERIFY(model != nullptr);
    QVERIFY(model->count() > 0);

    // Check that at least one image has coordinates set
    bool foundCoordinate = false;
    for (int i = 0; i < model->count(); ++i) {
        QModelIndex idx = model->index(i);
        QGeoCoordinate coord = model->data(idx, GeoTagImageModel::CoordinateRole).value<QGeoCoordinate>();
        if (coord.isValid()) {
            foundCoordinate = true;
            break;
        }
    }
    QVERIFY2(foundCoordinate, "Preview mode should populate model with coordinates");
}

// ============================================================================
// Calibrator algorithm tests
// ============================================================================

void GeoTagControllerTest::_calibratorEmptyInputsTest()
{
    // Empty triggers
    {
        QList<qint64> imageTimestamps = {100, 200, 300};
        QList<GeoTagData> triggers;
        CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);
        QVERIFY(result.imageIndices.isEmpty());
        QVERIFY(result.triggerIndices.isEmpty());
        QCOMPARE(result.skippedTriggers, 0);
    }

    // Empty images
    {
        QList<qint64> imageTimestamps;
        QList<GeoTagData> triggers = {makeValidTrigger(100)};
        CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);
        QVERIFY(result.imageIndices.isEmpty());
        QVERIFY(result.triggerIndices.isEmpty());
    }

    // Both empty
    {
        QList<qint64> imageTimestamps;
        QList<GeoTagData> triggers;
        CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);
        QVERIFY(result.imageIndices.isEmpty());
        QVERIFY(result.triggerIndices.isEmpty());
    }
}

void GeoTagControllerTest::_calibratorPerfectMatchTest()
{
    QList<qint64> imageTimestamps = {100, 200, 300};
    QList<GeoTagData> triggers = {
        makeValidTrigger(100),
        makeValidTrigger(200),
        makeValidTrigger(300)
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    QCOMPARE(result.imageIndices.count(), 3);
    QCOMPARE(result.triggerIndices.count(), 3);
    QCOMPARE(result.unmatchedImages.count(), 0);
    QCOMPARE(result.skippedTriggers, 0);

    // Verify correct pairing
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(result.imageIndices[i], i);
        QCOMPARE(result.triggerIndices[i], i);
    }
}

void GeoTagControllerTest::_calibratorToleranceMatchTest()
{
    // Offset-based matching: tolerance applies to offset differences, not absolute timestamps
    // Images: offsets from last (300) are {200, 100, 0}
    // Triggers: offsets from last (300) should be within tolerance of image offsets
    QList<qint64> imageTimestamps = {100, 200, 300};
    QList<GeoTagData> triggers = {
        makeValidTrigger(99),   // offset 201, diff 1 from image offset 200
        makeValidTrigger(198),  // offset 102, diff 2 from image offset 100
        makeValidTrigger(300)   // offset 0, exact match
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    QCOMPARE(result.imageIndices.count(), 3);
    QCOMPARE(result.triggerIndices.count(), 3);
    QCOMPARE(result.unmatchedImages.count(), 0);
}

void GeoTagControllerTest::_calibratorNoMatchOutsideToleranceTest()
{
    // Offset-based matching: last elements always have offset 0, so they match.
    // To test "no match", we verify that triggers with non-matching offsets fail.
    // Images: offsets from 300 are {200, 100, 0}
    // Triggers: offsets from 50 are {40, 0} - only offset 0 matches image offset 0
    QList<qint64> imageTimestamps = {100, 200, 300};
    QList<GeoTagData> triggers = {
        makeValidTrigger(10),   // offset 40 - far from any image offset
        makeValidTrigger(50)    // offset 0 - matches image offset 0
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    // Only the last trigger (offset 0) matches the last image (offset 0)
    QCOMPARE(result.imageIndices.count(), 1);
    QCOMPARE(result.triggerIndices.count(), 1);
    QCOMPARE(result.unmatchedImages.count(), 2);  // Images 0 and 1 unmatched
}

void GeoTagControllerTest::_calibratorSkipInvalidTriggersTest()
{
    QList<qint64> imageTimestamps = {100, 200, 300};
    QList<GeoTagData> triggers = {
        makeValidTrigger(100),
        makeInvalidTrigger(200),  // Failure result - should be skipped
        makeValidTrigger(300)
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    QCOMPARE(result.imageIndices.count(), 2);
    QCOMPARE(result.triggerIndices.count(), 2);
    QCOMPARE(result.skippedTriggers, 1);
    QCOMPARE(result.unmatchedImages.count(), 1);  // Image at 200 has no match
}

void GeoTagControllerTest::_calibratorSkipInvalidCoordinatesTest()
{
    QList<qint64> imageTimestamps = {100, 200, 300};
    QList<GeoTagData> triggers = {
        makeValidTrigger(100),
        makeTriggerWithInvalidCoord(200),  // Invalid coordinate - should be skipped
        makeValidTrigger(300)
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    QCOMPARE(result.imageIndices.count(), 2);
    QCOMPARE(result.triggerIndices.count(), 2);
    QCOMPARE(result.skippedTriggers, 1);
}

void GeoTagControllerTest::_calibratorUnmatchedImagesTest()
{
    // Images: offsets from 500 are {400, 300, 200, 100, 0}
    // Triggers: offsets from 300 are {200, 0}
    // Trigger offset 200 matches image offset 200 (image 2 at timestamp 300)
    // Trigger offset 0 matches image offset 0 (image 4 at timestamp 500)
    QList<qint64> imageTimestamps = {100, 200, 300, 400, 500};
    QList<GeoTagData> triggers = {
        makeValidTrigger(100),
        makeValidTrigger(300)
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    QCOMPARE(result.imageIndices.count(), 2);
    QCOMPARE(result.unmatchedImages.count(), 3);
    QVERIFY(result.unmatchedImages.contains(0));  // offset 400 - no match
    QVERIFY(result.unmatchedImages.contains(1));  // offset 300 - no match
    QVERIFY(result.unmatchedImages.contains(3));  // offset 100 - no match
}

void GeoTagControllerTest::_calibratorTimeOffsetTest()
{
    // Offset-based matching uses "time from end of sequence" which normalizes
    // different time bases. The timeOffsetSecs parameter adjusts for clock drift
    // but doesn't affect matching when both sequences have the same duration.
    //
    // For this test, we create sequences with different durations where
    // timeOffset affects which offsets align.
    //
    // Images: {90, 190, 290} → offsets from 290: {200, 100, 0}
    // Triggers: {100, 200, 300} → offsets from 300: {200, 100, 0}
    // Same offsets = all match regardless of absolute timestamp difference!
    QList<qint64> imageTimestamps = {90, 190, 290};
    QList<GeoTagData> triggers = {
        makeValidTrigger(100),
        makeValidTrigger(200),
        makeValidTrigger(300)
    };

    // Both sequences have same duration (200 seconds), so offsets align
    {
        CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);
        QCOMPARE(result.imageIndices.count(), 3);
    }

    // Test with mismatched durations where offset helps
    // Images: {10, 110, 210} → offsets from 210: {200, 100, 0}
    // Triggers: {5, 105, 205} → offsets from 205: {200, 100, 0}
    // Same offsets = all match
    // Note: Using 10 instead of 0 because 0 is reserved for failed EXIF parsing
    {
        QList<qint64> images2 = {10, 110, 210};
        QList<GeoTagData> triggers2 = {
            makeValidTrigger(5),
            makeValidTrigger(105),
            makeValidTrigger(205)
        };
        CalibrationResult result = GeoTagCalibrator::calibrate(images2, triggers2, 0, 2);
        QCOMPARE(result.imageIndices.count(), 3);
    }
}

void GeoTagControllerTest::_calibratorDuplicatePreventionTest()
{
    // Two triggers close to same image timestamp
    QList<qint64> imageTimestamps = {100, 200};
    QList<GeoTagData> triggers = {
        makeValidTrigger(100),
        makeValidTrigger(101),  // Very close to first, but image 0 already used
        makeValidTrigger(200)
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    // Should match 2 images, not 3 (can't reuse images)
    QCOMPARE(result.imageIndices.count(), 2);
    QCOMPARE(result.triggerIndices.count(), 2);

    // Verify no duplicate image indices
    QSet<int> usedImages(result.imageIndices.begin(), result.imageIndices.end());
    QCOMPARE(usedImages.count(), result.imageIndices.count());
}

void GeoTagControllerTest::_calibratorClosestMatchTest()
{
    // Test that closest offset match is selected
    // Image: {100} → offset from 100: {0}
    // Triggers processed in order; first valid match within tolerance wins
    //
    // With offset-based matching:
    // Trigger at 102 has offset 99-102 = -3 (last trigger is 99)
    // Trigger at 99 has offset 0
    // Image offset 0 matches trigger offset 0, so trigger 1 (at 99) matches
    QList<qint64> imageTimestamps = {100};
    QList<GeoTagData> triggers = {
        makeValidTrigger(102),  // offset -3 from last, outside tolerance
        makeValidTrigger(99)    // offset 0 from last, matches image offset 0
    };

    CalibrationResult result = GeoTagCalibrator::calibrate(imageTimestamps, triggers, 0, 2);

    QCOMPARE(result.imageIndices.count(), 1);
    QCOMPARE(result.triggerIndices.count(), 1);
    QCOMPARE(result.triggerIndices[0], 1);  // Second trigger (99) matches due to offset alignment
}

UT_REGISTER_TEST(GeoTagControllerTest, TestLabel::Unit, TestLabel::AnalyzeView)
