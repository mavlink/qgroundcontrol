#include "APMAirframeComponentControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtGui/QGuiApplication>

#define private public
#include "APMAirframeComponentController.h"
#undef private

APMAirframeComponentControllerTest::APMAirframeComponentControllerTest()
{
    setWaitForParameters(true);
}

void APMAirframeComponentControllerTest::_downloadCompleteSlotsRestoreCursor()
{
    APMAirframeComponentController controller(this);

    // Failure path for github metadata download should restore wait cursor.
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    controller._githubJsonDownloadComplete(false, QString(), QStringLiteral("simulated download error"));
    QVERIFY(QGuiApplication::overrideCursor() == nullptr);

    // Success path with invalid JSON should also restore wait cursor.
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString invalidJsonFile = tempDir.path() + QStringLiteral("/invalid.json");
    QFile jsonFile(invalidJsonFile);
    QVERIFY(jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    jsonFile.write("{ invalid json");
    jsonFile.close();

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    controller._githubJsonDownloadComplete(true, invalidJsonFile, QString());
    QVERIFY(QGuiApplication::overrideCursor() == nullptr);

    // Success path with missing download_url should fail start and restore wait cursor.
    const QString missingDownloadUrlJsonFile = tempDir.path() + QStringLiteral("/missing_download_url.json");
    QFile missingUrlFile(missingDownloadUrlJsonFile);
    QVERIFY(missingUrlFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    missingUrlFile.write("{}");
    missingUrlFile.close();

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    controller._githubJsonDownloadComplete(true, missingDownloadUrlJsonFile, QString());
    QVERIFY(QGuiApplication::overrideCursor() == nullptr);

    // Parameter file download failure should restore wait cursor.
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    controller._paramFileDownloadComplete(false, QString(), QStringLiteral("simulated param download error"));
    QVERIFY(QGuiApplication::overrideCursor() == nullptr);
}

UT_REGISTER_TEST(APMAirframeComponentControllerTest, TestLabel::Integration, TestLabel::Vehicle)
