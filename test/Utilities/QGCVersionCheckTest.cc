#include "QGCVersionCheckTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Fact.h"
#include "QGCVersionCheck.h"
#include "SettingsManager.h"

void QGCVersionCheckTest::_newerVersionDetected_data()
{
    QTest::addColumn<QString>("current");
    QTest::addColumn<QString>("latest");
    QTest::addColumn<bool>("expected");

    QTest::newRow("newer patch") << "v5.1.2" << "v5.1.3" << true;
    QTest::newRow("newer minor") << "v5.1.9" << "v5.2.0" << true;
    QTest::newRow("newer major") << "v4.9.9" << "v5.0.0" << true;
    QTest::newRow("numeric not lexical") << "v5.9.0" << "v5.10.0" << true;
    QTest::newRow("equal") << "v5.1.2" << "v5.1.2" << false;
    QTest::newRow("older") << "v5.2.0" << "v5.1.9" << false;
    QTest::newRow("suffixed current") << "v5.1.2-45-gabcdef" << "v5.1.3" << true;
    QTest::newRow("unparseable latest") << "v5.1.2" << "garbage" << false;
    QTest::newRow("unparseable current") << "garbage" << "v5.1.3" << false;
    QTest::newRow("missing v prefix") << "5.1.2" << "v5.1.3" << false;
    QTest::newRow("leading garbage") << "v5.1.2" << "xv5.1.3" << false;
    QTest::newRow("leading whitespace") << "v5.1.2" << " v5.1.3" << true;
    QTest::newRow("empty latest") << "v5.1.2" << "" << false;
}

void QGCVersionCheckTest::_newerVersionDetected()
{
    QFETCH(QString, current);
    QFETCH(QString, latest);
    QFETCH(bool, expected);

    QCOMPARE(QGCVersionCheck::isNewerVersion(current, latest), expected);
}

void QGCVersionCheckTest::_notifyOncePerVersion_data()
{
    QTest::addColumn<QString>("latest");
    QTest::addColumn<QString>("lastNotified");
    QTest::addColumn<bool>("expected");

    QTest::newRow("never notified") << "v5.2.0" << "" << true;
    QTest::newRow("already notified") << "v5.2.0" << "v5.2.0" << false;
    QTest::newRow("newer than notified") << "v5.3.0" << "v5.2.0" << true;
    QTest::newRow("older than notified") << "v5.1.0" << "v5.2.0" << false;
    QTest::newRow("corrupt setting") << "v5.2.0" << "junk" << true;
    QTest::newRow("unparseable latest") << "junk" << "" << false;
}

void QGCVersionCheckTest::_notifyOncePerVersion()
{
    QFETCH(QString, latest);
    QFETCH(QString, lastNotified);
    QFETCH(bool, expected);

    QCOMPARE(QGCVersionCheck::shouldNotify(latest, lastNotified), expected);
}

void QGCVersionCheckTest::_downloadedVersionFile_data()
{
    QTest::addColumn<bool>("downloaded");
    QTest::addColumn<QByteArray>("versionFile");
    QTest::addColumn<QString>("newStableVersion");  // empty: no newer version is announced

    QTest::newRow("newer version") << true << QByteArrayLiteral("v9.9.9\n") << QStringLiteral("v9.9.9");
    QTest::newRow("only the first line counts")
        << true << QByteArrayLiteral("v9.9.9\r\nv10.0.0\r\n") << QStringLiteral("v9.9.9");
    QTest::newRow("no line end") << true << QByteArrayLiteral("v9.9.9") << QStringLiteral("v9.9.9");
    QTest::newRow("running version") << true << QByteArrayLiteral("v5.1.2\n") << QString();
    QTest::newRow("older version") << true << QByteArrayLiteral("v5.1.1\n") << QString();
    QTest::newRow("not a version") << true << QByteArrayLiteral("<html>Not Found</html>\n") << QString();
    QTest::newRow("empty file") << true << QByteArray() << QString();
    QTest::newRow("download failed") << false << QByteArrayLiteral("v9.9.9\n") << QString();
}

void QGCVersionCheckTest::_downloadedVersionFile()
{
    QFETCH(bool, downloaded);
    QFETCH(QByteArray, versionFile);
    QFETCH(QString, newStableVersion);

    const QString originalVersion = QCoreApplication::applicationVersion();
    QCoreApplication::setApplicationVersion(QStringLiteral("v5.1.2"));
    Fact* const lastNotifiedFact = SettingsManager::instance()->appSettings()->lastNotifiedStableVersion();
    const QVariant originalLastNotified = lastNotifiedFact->rawValue();
    lastNotifiedFact->setRawValue(QString());
    const auto restore = qScopeGuard([originalVersion, lastNotifiedFact, originalLastNotified]() {
        QCoreApplication::setApplicationVersion(originalVersion);
        lastNotifiedFact->setRawValue(originalLastNotified);
    });

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString versionFilePath = tempDir.filePath(QStringLiteral("QGC.version.txt"));
    QFile file(versionFilePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(versionFile), versionFile.size());
    file.close();

    QGCVersionCheck versionCheck;
    QSignalSpy changedSpy(&versionCheck, &QGCVersionCheck::newStableVersionChanged);

    const bool announced = !newStableVersion.isEmpty();
    if (announced) {
        expectAppMessage(QRegularExpression(QStringLiteral("There is a newer version of")));
    }
    if (downloaded) {
        versionCheck._downloadComplete(true, versionFilePath, QString());
    } else {
        versionCheck._downloadComplete(false, QString(), QStringLiteral("simulated download error"));
    }
    if (announced) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(versionCheck.newStableVersion(), newStableVersion);
    QCOMPARE(changedSpy.count(), announced ? 1 : 0);
    QCOMPARE(lastNotifiedFact->rawValue().toString(), newStableVersion);
}

UT_REGISTER_TEST(QGCVersionCheckTest, TestLabel::Unit, TestLabel::Utilities)
