#include "VideoSettingsTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>

#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <unistd.h>
#endif

#include "Fixtures/RAIIFixtures.h"
#include "VideoSettings.h"

void VideoSettingsTest::_testNetworkVideoUrlValidation_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QStringList>("schemes");
    QTest::addColumn<bool>("valid");

    QTest::newRow("http") << QStringLiteral("http://192.0.2.1:8080/video")
                          << QStringList{QStringLiteral("http"), QStringLiteral("https")} << true;
    QTest::newRow("https-query") << QStringLiteral("https://camera.example/video?quality=high")
                                 << QStringList{QStringLiteral("http"), QStringLiteral("https")} << true;
    QTest::newRow("websocket") << QStringLiteral("wss://camera.example/jpeg")
                               << QStringList{QStringLiteral("ws"), QStringLiteral("wss")} << true;
    QTest::newRow("missing-host") << QStringLiteral("https:///video")
                                  << QStringList{QStringLiteral("http"), QStringLiteral("https")} << false;
    QTest::newRow("wrong-scheme") << QStringLiteral("ftp://camera.example/video")
                                  << QStringList{QStringLiteral("http"), QStringLiteral("https")} << false;
    QTest::newRow("userinfo") << QStringLiteral("https://user:pass@camera.example/video")
                              << QStringList{QStringLiteral("http"), QStringLiteral("https")} << false;
    QTest::newRow("fragment") << QStringLiteral("wss://camera.example/jpeg#token")
                              << QStringList{QStringLiteral("ws"), QStringLiteral("wss")} << false;
    QTest::newRow("token-query") << QStringLiteral("https://camera.example/video?Access_Token=secret")
                                 << QStringList{QStringLiteral("http"), QStringLiteral("https")} << false;
}

void VideoSettingsTest::_testNetworkVideoUrlValidation()
{
    QFETCH(QString, url);
    QFETCH(QStringList, schemes);
    QFETCH(bool, valid);

    QString error;
    QCOMPARE(VideoSettings::validateNetworkVideoUrl(url, schemes, error), valid);
    QCOMPARE(error.isEmpty(), valid);
}

void VideoSettingsTest::_testAuthenticatedTransportPolicy()
{
#ifndef QGC_GST_STREAMING
    QSKIP("GStreamer not enabled");
#else
    VideoSettings settings;
    TestFixtures::SettingsFixture fixture;
    fixture.setFactValue(settings.videoSource(), QString::fromLatin1(VideoSettings::videoSourceHTTPMJPEG));
    fixture.setFactValue(settings.httpMjpegUrl(), QStringLiteral("http://192.0.2.1:8080/video"));
    fixture.setFactValue(settings.networkVideoAuthType(), VideoSettings::NetworkVideoAuthBasic);
    fixture.setFactValue(settings.networkVideoUsername(), QStringLiteral("viewer"));
    QCOMPARE(settings.setNetworkVideoSecret(QStringLiteral("test-password")), QString());

    QVERIFY(settings.networkVideoConfigurationError().contains(QStringLiteral("HTTPS")));

    settings.httpMjpegUrl()->setRawValue(QStringLiteral("https://camera.example/video"));
    QCOMPARE(settings.networkVideoConfigurationError(), QString());

    const QStringList invalidUsernames = {
        QStringLiteral("viewer:admin"),
        QString::fromUtf8("viewer\0admin", 12),
        QStringLiteral("viewer\radmin"),
        QStringLiteral("viewer\nadmin"),
    };
    for (const QString& username : invalidUsernames) {
        settings.networkVideoUsername()->setRawValue(username);
        QVERIFY(settings.networkVideoConfigurationError().contains(QStringLiteral("username")));
    }
#endif
}

void VideoSettingsTest::_testOriginValidation()
{
#ifndef QGC_HAS_WEBSOCKET_VIDEO
    QSKIP("Qt WebSockets unavailable");
#else
    VideoSettings settings;
    TestFixtures::SettingsFixture fixture;
    fixture.setFactValue(settings.videoSource(), QString::fromLatin1(VideoSettings::videoSourceWebSocketJPEG));
    fixture.setFactValue(settings.websocketJpegUrl(), QStringLiteral("ws://192.0.2.1:8080/video"));

    fixture.setFactValue(settings.networkVideoOrigin(), QStringLiteral("https://operator.example/path"));
    QVERIFY(settings.networkVideoConfigurationError().contains(QStringLiteral("Origin")));

    settings.networkVideoOrigin()->setRawValue(QStringLiteral("https://operator.example:8443"));
    QCOMPARE(settings.networkVideoConfigurationError(), QString());
#endif
}

void VideoSettingsTest::_testCredentialFilePlatformSupport()
{
    VideoSettings settings;
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    QVERIFY(settings.networkVideoCredentialFileSupported());
#else
    QVERIFY(!settings.networkVideoCredentialFileSupported());
#endif
}

void VideoSettingsTest::_testSecretFileValidation()
{
#if !defined(Q_OS_UNIX) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QSKIP("Owner-only credential file validation is Unix-specific");
#else
    VideoSettings settings;
    TestFixtures::SettingsFixture fixture;
    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write("file-secret\n"), 12);
    QVERIFY(file.flush());
    QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
    fixture.setFactValue(settings.networkVideoSecretFile(), file.fileName());

    QByteArray secret;
    QString error;
    QVERIFY2(settings.resolveNetworkVideoSecret(secret, error), qPrintable(error));
    QCOMPARE(secret, QByteArrayLiteral("file-secret"));

    QVERIFY(file.resize(0));
    QVERIFY(file.seek(0));
    QCOMPARE(file.write("file-secret\n\n"), 13);
    QVERIFY(file.flush());
    QVERIFY(!settings.resolveNetworkVideoSecret(secret, error));
    QVERIFY(error.contains(QStringLiteral("exactly one")));

    QVERIFY(file.resize(0));
    QVERIFY(file.seek(0));
    QCOMPARE(file.write("file-secret\n"), 12);
    QVERIFY(file.flush());
    QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup));
    QVERIFY(!settings.resolveNetworkVideoSecret(secret, error));
    QVERIFY(error.contains(QStringLiteral("permissions")));
#endif
}

void VideoSettingsTest::_testSecretFileRejectsAliases()
{
#if !defined(Q_OS_UNIX) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QSKIP("Owner-only credential file validation is Unix-specific");
#else
    VideoSettings settings;
    TestFixtures::SettingsFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString targetPath = directory.filePath(QStringLiteral("credential"));
    QFile target(targetPath);
    QVERIFY(target.open(QIODevice::WriteOnly));
    QCOMPARE(target.write("file-secret\n"), 12);
    target.close();
    QVERIFY(target.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));

    const QString symlinkPath = directory.filePath(QStringLiteral("credential-symlink"));
    const QByteArray encodedTarget = QFile::encodeName(targetPath);
    const QByteArray encodedSymlink = QFile::encodeName(symlinkPath);
    QCOMPARE(::symlink(encodedTarget.constData(), encodedSymlink.constData()), 0);
    fixture.setFactValue(settings.networkVideoSecretFile(), symlinkPath);

    QByteArray secret;
    QString error;
    QVERIFY(!settings.resolveNetworkVideoSecret(secret, error));
    QVERIFY(error.contains(QStringLiteral("symbolic")));

    const QString hardlinkPath = directory.filePath(QStringLiteral("credential-hardlink"));
    const QByteArray encodedHardlink = QFile::encodeName(hardlinkPath);
    QCOMPARE(::link(encodedTarget.constData(), encodedHardlink.constData()), 0);
    settings.networkVideoSecretFile()->setRawValue(targetPath);
    QVERIFY(!settings.resolveNetworkVideoSecret(secret, error));
    QVERIFY(error.contains(QStringLiteral("hard links")));
#endif
}

UT_REGISTER_TEST(VideoSettingsTest, TestLabel::Unit)
