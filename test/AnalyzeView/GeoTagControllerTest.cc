#include "GeoTagControllerTest.h"
#include "GeoTagController.h"
#include "GeoTagWorker.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void GeoTagControllerTest::_geoTagControllerTest()
{
    const QDir tempDir = QDir(QDir::tempPath());
    const QString imageDirPath = QDir::tempPath() + "/QGC_GEOTAG_TEST";
    const QString subDirName = QString("QGC_GEOTAG_TEST");
    if (!tempDir.exists(subDirName)) {
        QVERIFY(tempDir.mkdir(subDirName));
    } else {
        QDir subDir(QDir::tempPath() + "/" + subDirName);
        const QStringList files = subDir.entryList(QDir::Files);
        for (const QString &fileName : files) {
            QVERIFY(subDir.remove(fileName));
        }
    }

    QFile file(":/DSCN0010.jpg");
    for (int i = 0; i < 58; ++i) {
        QVERIFY(file.copy(imageDirPath + QStringLiteral("/geotag_temp_image_%1.jpg").arg(i)));
    }

    QDir imageDir = QDir(imageDirPath);
    if (!imageDir.exists("TAGGED")) {
        QVERIFY(imageDir.mkdir("TAGGED"));
    } else {
        QDir subDir(imageDirPath + "/TAGGED");
        const QStringList files = subDir.entryList(QDir::Files);
        for (const QString &fileName : files) {
            QVERIFY(subDir.remove(fileName));
        }
    }

    GeoTagController* const controller = new GeoTagController(this);

    const QFileInfo log = QFileInfo(":/SampleULog.ulg");
    controller->setLogFile(log.filePath());
    controller->setImageDirectory(imageDirPath + "/");
    controller->setSaveDirectory(controller->imageDirectory() + "/TAGGED");

    QVERIFY(!controller->logFile().isEmpty());
    QVERIFY(!controller->imageDirectory().isEmpty());
    QVERIFY(!controller->saveDirectory().isEmpty());

    QSignalSpy spyGeoTagControllerProgress(controller, &GeoTagController::progressChanged);

    controller->startTagging();

    QCOMPARE(spyGeoTagControllerProgress.wait(1000), true);

    QVERIFY(controller->progress() > 0);
    QCOMPARE(controller->errorMessage(), "");
}

void GeoTagControllerTest::_geoTagWorkerTest()
{
    const QDir tempDir = QDir(QDir::tempPath());
    const QString imageDirPath = QDir::tempPath() + "/QGC_GEOTAG_TEST";
    const QString subDirName = QString("QGC_GEOTAG_TEST");
    if (!tempDir.exists(subDirName)) {
        QVERIFY(tempDir.mkdir(subDirName));
    } else {
        QDir subDir(QDir::tempPath() + "/" + subDirName);
        const QStringList files = subDir.entryList(QDir::Files);
        for (const QString &fileName : files) {
            QVERIFY(subDir.remove(fileName));
        }
    }

    QFile file(":/DSCN0010.jpg");
    for (int i = 0; i < 58; ++i) {
        QVERIFY(file.copy(imageDirPath + QStringLiteral("/geotag_temp_image_%1.jpg").arg(i)));
    }

    QDir imageDir = QDir(imageDirPath);
    if (!imageDir.exists("TAGGED")) {
        QVERIFY(imageDir.mkdir("TAGGED"));
    } else {
        QDir subDir(imageDirPath + "/TAGGED");
        const QStringList files = subDir.entryList(QDir::Files);
        for (const QString &fileName : files) {
            QVERIFY(subDir.remove(fileName));
        }
    }

    const QFileInfo log = QFileInfo(":/SampleULog.ulg");
    GeoTagWorker* const worker = new GeoTagWorker(this);
    worker->setLogFile(log.filePath());
    worker->setImageDirectory(imageDirPath + "/");
    worker->setSaveDirectory(worker->imageDirectory() + "/TAGGED");

    QVERIFY(worker->process());
}
