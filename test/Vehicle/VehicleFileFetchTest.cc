#include "VehicleFileFetchTest.h"

#include <memory>
#include <utility>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>

#include "FTPManager.h"
#include "FileFixtures.h"
#include "LocalHttpTestServer.h"
#include "MockLink.h"
#include "MockLinkFTP.h"
#include "NTRIP/Support/NTRIPTlsTestFixtures.h"
#include "Vehicle.h"
#include "VehicleFileFetch.h"

namespace {

/// The content of the MockLinkFTP size file of @a size
QByteArray fetchTestSizeFileContent(int size)
{
    QByteArray content(size, 0);
    for (int i = 0; i < size; i++) {
        content[i] = static_cast<char>(i % 255);
    }
    return content;
}

QString fetchTestSizeFileUri(int size)
{
    return QStringLiteral("mftp://%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(size);
}

}  // namespace

// Either way, the file arrives in the directory under the name asked for, the fetch reports its start and its
// progress, and it calls back once with the file
void VehicleFileFetchTest::_testFetch_data()
{
    QTest::addColumn<bool>("ftp");

    QTest::newRow("MAVLink FTP") << true;
    QTest::newRow("HTTP") << false;
}

void VehicleFileFetchTest::_testFetch()
{
    QFETCH(bool, ftp);

    _connectMockLinkNoInitialConnectSequence();
    constexpr int fileSize = 3 * 1024;
    TestFixtures::LocalHttpTestServer server;
    QString uri = fetchTestSizeFileUri(fileSize);
    if (!ftp) {
        QVERIFY(server.listen());
        server.installHttpResponder(fetchTestSizeFileContent(fileSize));
        uri = server.url(QStringLiteral("/file.bin"));
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QObject owner;
    Result result;
    VehicleFileFetch* const fetch = VehicleFileFetch::start(
        &owner, _vehicle, uri, {.directory = directory.path(), .fileName = QStringLiteral("fetched.bin")},
        [&result](const QString& file, const QString& errorMsg) {
            result.calls++;
            result.file = file;
            result.error = errorMsg;
        });
    QVERIFY(fetch);
    QSignalSpy startedSpy(fetch, &VehicleFileFetch::started);
    QSignalSpy progressSpy(fetch, &VehicleFileFetch::progressChanged);

    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.error, QString());
    QCOMPARE(result.file, QDir(directory.path()).filePath(QStringLiteral("fetched.bin")));
    QCOMPARE(TestFixtures::readFile(result.file), fetchTestSizeFileContent(fileSize));
    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(!progressSpy.isEmpty());
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList{QStringLiteral("fetched.bin")});
}

// A fetch that fails leaves no file, also over HTTP where the server sends a body with its error
void VehicleFileFetchTest::_testFailedFetchLeavesNothing_data()
{
    QTest::addColumn<bool>("ftp");

    QTest::newRow("MAVLink FTP, no such file") << true;
    QTest::newRow("HTTP, not found") << false;
}

void VehicleFileFetchTest::_testFailedFetchLeavesNothing()
{
    QFETCH(bool, ftp);

    _connectMockLinkNoInitialConnectSequence();
    TestFixtures::LocalHttpTestServer server;
    QString uri = QStringLiteral("mftp:///no-such-file");
    if (!ftp) {
        QVERIFY(server.listen());
        server.installHttpResponder(QByteArrayLiteral("There is no such file"), 404);
        uri = server.url(QStringLiteral("/no-such-file"));
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Download error")));
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QObject owner;
    Result result;
    QVERIFY(VehicleFileFetch::start(&owner, _vehicle, uri, {.directory = directory.path()},
                                    [&result](const QString& file, const QString& errorMsg) {
                                        result.calls++;
                                        result.file = file;
                                        result.error = errorMsg;
                                    }));

    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    if (!ftp) {
        verifyExpectedLogMessage();
    }
    QVERIFY(!result.error.isEmpty());
    QCOMPARE(result.file, QString());
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList());
}

// A file larger than the limit fails the fetch, whether its size is announced or not, and none of it is kept
void VehicleFileFetchTest::_testFileOverTheLimit_data()
{
    QTest::addColumn<bool>("ftp");
    QTest::addColumn<bool>("lengthAnnounced");

    QTest::newRow("MAVLink FTP") << true << true;
    QTest::newRow("HTTP") << false << true;
    QTest::newRow("HTTP, of no announced length") << false << false;
}

void VehicleFileFetchTest::_testFileOverTheLimit()
{
    QFETCH(bool, ftp);
    QFETCH(bool, lengthAnnounced);

    _connectMockLinkNoInitialConnectSequence();
    constexpr int fileSize = 4 * 1024;
    TestFixtures::LocalHttpTestServer server;
    QString uri = fetchTestSizeFileUri(fileSize);
    if (!ftp) {
        QVERIFY(server.listen());
        const QByteArray body = fetchTestSizeFileContent(fileSize);
        if (lengthAnnounced) {
            server.installHttpResponder(body);
        } else {
            server.installRawResponder(QByteArrayLiteral("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n") + body);
        }
        uri = server.url(QStringLiteral("/file.bin"));
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                         QRegularExpression(QStringLiteral("The file is larger than")));
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QObject owner;
    Result result;
    QVERIFY(VehicleFileFetch::start(&owner, _vehicle, uri, {.directory = directory.path(), .maxBytes = fileSize - 1},
                                    [&result](const QString& file, const QString& errorMsg) {
                                        result.calls++;
                                        result.file = file;
                                        result.error = errorMsg;
                                    }));

    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    if (!ftp) {
        verifyExpectedLogMessage();
    }
    QVERIFY2(result.error.contains(QStringLiteral("larger than")), qPrintable(result.error));
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList());
}

// cancel() and the end of the owner end a transfer under way: it stops, nothing of it is kept, and nothing is called
void VehicleFileFetchTest::_testFetchEndedWithoutCallback_data()
{
    QTest::addColumn<bool>("ftp");
    QTest::addColumn<bool>("ownerDeleted");

    QTest::newRow("MAVLink FTP, cancelled") << true << false;
    QTest::newRow("MAVLink FTP, owner deleted") << true << true;
    QTest::newRow("HTTP, cancelled") << false << false;
    QTest::newRow("HTTP, owner deleted") << false << true;
}

void VehicleFileFetchTest::_testFetchEndedWithoutCallback()
{
    QFETCH(bool, ftp);
    QFETCH(bool, ownerDeleted);

    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    // Accepts a connection and never answers it
    QTcpServer server;
    QString uri = fetchTestSizeFileUri(100 * 1024);
    if (ftp) {
        mockFtp->setBurstReadDelayMs(100);
    } else {
        QVERIFY(server.listen(QHostAddress::LocalHost));
        uri = QStringLiteral("http://127.0.0.1:%1/file.bin").arg(server.serverPort());
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    Result result;
    auto owner = std::make_unique<QObject>();
    const QPointer<VehicleFileFetch> fetch =
        VehicleFileFetch::start(owner.get(), _vehicle, uri, {.directory = directory.path()},
                                [&result](const QString& file, const QString& errorMsg) {
                                    result.calls++;
                                    result.file = file;
                                    result.error = errorMsg;
                                });
    QVERIFY(fetch);

    QTcpSocket* connection = nullptr;
    if (ftp) {
        QSignalSpy progressSpy(fetch, &VehicleFileFetch::progressChanged);
        QVERIFY(UnitTest::waitForSignal(progressSpy, TestTimeout::longMs(), QStringLiteral("progressChanged")));
    } else {
        QVERIFY(server.waitForNewConnection(TestTimeout::mediumMs()));
        connection = server.nextPendingConnection();
        QVERIFY(connection);
    }
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files).count(), 1);

    if (ownerDeleted) {
        owner.reset();
    } else {
        fetch->cancel();
    }
    mockFtp->setBurstReadDelayMs(0);

    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList());
    QCOMPARE(_vehicle->ftpManager()->requestCount(), 0);
    if (connection) {
        QTRY_COMPARE_WITH_TIMEOUT(connection->state(), QAbstractSocket::UnconnectedState, TestTimeout::shortMs());
    }
    QTRY_VERIFY_WITH_TIMEOUT(fetch.isNull(), TestTimeout::shortMs());
    QCOMPARE(result.calls, 0);
}

// Fetches over MAVLink FTP from one vehicle wait for each other, and each gets its own file
void VehicleFileFetchTest::_testFetchesOfOneVehicleQueue()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setSingleSessionEnforced(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QObject owner;
    Result first;
    Result second;
    for (auto [size, result] : {std::pair{3 * 1024, &first}, std::pair{2 * 1024, &second}}) {
        QVERIFY(VehicleFileFetch::start(&owner, _vehicle, fetchTestSizeFileUri(size), {.directory = directory.path()},
                                        [result](const QString& file, const QString& errorMsg) {
                                            result->calls++;
                                            result->file = file;
                                            result->error = errorMsg;
                                        }));
    }

    QVERIFY_TRUE_WAIT((first.calls == 1) && (second.calls == 1), TestTimeout::longMs());
    QCOMPARE(first.error, QString());
    QCOMPARE(second.error, QString());
    QCOMPARE(TestFixtures::readFile(first.file), fetchTestSizeFileContent(3 * 1024));
    QCOMPARE(TestFixtures::readFile(second.file), fetchTestSizeFileContent(2 * 1024));
}

// A fetch that cannot start says so at once, and calls nothing
void VehicleFileFetchTest::_testRefused_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<bool>("vehicle");
    QTest::addColumn<QString>("warning");

    QTest::newRow("no directory") << QString() << true << QStringLiteral("the directory is not set or does not exist");
    QTest::newRow("missing directory") << QStringLiteral("missing") << true
                                       << QStringLiteral("the directory is not set or does not exist");
    QTest::newRow("MAVLink FTP without a vehicle")
        << QStringLiteral(".") << false << QStringLiteral("without a vehicle");
}

void VehicleFileFetchTest::_testRefused()
{
    QFETCH(QString, directory);
    QFETCH(bool, vehicle);
    QFETCH(QString, warning);

    _connectMockLinkNoInitialConnectSequence();
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString directoryPath = directory.isEmpty() ? QString() : QDir(root.path()).filePath(directory);

    QObject owner;
    Result result;
    expectLogMessage("Vehicle.VehicleFileFetch", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(!VehicleFileFetch::start(&owner, vehicle ? _vehicle : nullptr, fetchTestSizeFileUri(1024),
                                     {.directory = directoryPath},
                                     [&result](const QString&, const QString&) { result.calls++; }));
    verifyExpectedLogMessage();
    QCOMPARE(_vehicle->ftpManager()->requestCount(), 0);
    QVERIFY(owner.children().isEmpty());
    QCOMPARE(result.calls, 0);
}

// A URI that a vehicle gives must not make QGC read its own disk: what is neither MAVLink FTP nor HTTP(S) is refused
// with one warning, and nothing of the file it names is read
void VehicleFileFetchTest::_testUriOtherThanFtpOrHttpRefused_data()
{
    QTest::addColumn<QString>("kind");

    QTest::newRow("absolute path") << QStringLiteral("absolute path");
    QTest::newRow("relative path that exists") << QStringLiteral("relative path");
    QTest::newRow("file URL") << QStringLiteral("file URL");
    QTest::newRow("Qt resource") << QStringLiteral("resource");
    QTest::newRow("Qt resource URL") << QStringLiteral("resource URL");
    QTest::newRow("other scheme") << QStringLiteral("other scheme");
}

void VehicleFileFetchTest::_testUriOtherThanFtpOrHttpRefused()
{
    QFETCH(QString, kind);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QTemporaryDir sourceDirectory;
    QVERIFY(sourceDirectory.isValid());
    // A file that would be fetched, were it read
    const QString source = sourceDirectory.filePath(QStringLiteral("definition.xml"));
    QVERIFY(TestFixtures::writeFile(source, QByteArrayLiteral("<definition/>")));

    QString uri = QStringLiteral("ftp://127.0.0.1/definition.xml");
    if (kind == QStringLiteral("absolute path")) {
        uri = source;
    }
    // Relative to the working directory, and without "./" before it
    QTemporaryDir relativeDirectory(QStringLiteral("fetch-test-XXXXXX"));
    if (kind == QStringLiteral("relative path")) {
        QVERIFY(relativeDirectory.isValid());
        uri = QDir::current().relativeFilePath(relativeDirectory.filePath(QStringLiteral("definition.xml")));
        QVERIFY(TestFixtures::writeFile(uri, QByteArrayLiteral("<definition/>")));
        QVERIFY(!uri.startsWith(QLatin1Char('.')));
    } else if (kind == QStringLiteral("file URL")) {
        uri = QUrl::fromLocalFile(source).toString();
    } else if (kind == QStringLiteral("resource")) {
        uri = QStringLiteral(":/unittest/TranslationTest_de_DE.ts");
    } else if (kind == QStringLiteral("resource URL")) {
        uri = QStringLiteral("qrc:/unittest/TranslationTest_de_DE.ts");
    }

    QObject owner;
    Result result;
    expectLogMessage("Vehicle.VehicleFileFetch", QtWarningMsg,
                     QRegularExpression(QStringLiteral("only MAVLink FTP and HTTP\\(S\\) are fetched")));
    QVERIFY(!VehicleFileFetch::start(&owner, nullptr, uri, {.directory = directory.path()},
                                     [&result](const QString&, const QString&) { result.calls++; }));
    verifyExpectedLogMessage();
    QVERIFY(owner.children().isEmpty());
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList());
    QCOMPARE(result.calls, 0);
}

// Over HTTP the URL or the caller names the file in the directory, never where it goes, and the same way on every
// system. A name that cannot be used is refused, as over MAVLink FTP (FTPManagerTest::_testDownloadFileName)
void VehicleFileFetchTest::_testHttpFileName_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("fileName");
    // Name of the fetched file in the directory, empty if the fetch is refused
    QTest::addColumn<QString>("localName");

    // Decoded, "%5C" is a backslash, a separator on Windows
    QTest::newRow("encoded backslashes in the URL") << "/a/..%5C..%5Cescaped.json" << "" << "escaped.json";
    QTest::newRow("backslashes in the name") << "/file.bin" << "..\\..\\escaped.json" << "escaped.json";
    QTest::newRow("no name in the URL") << "/dir/" << "" << "";
    QTest::newRow("a device in the URL") << "/a/COM1.json" << "" << "";
    QTest::newRow("relative to a drive in the name") << "/file.bin" << "C:escaped.json" << "";
}

void VehicleFileFetchTest::_testHttpFileName()
{
    QFETCH(QString, path);
    QFETCH(QString, fileName);
    QFETCH(QString, localName);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QByteArray body = fetchTestSizeFileContent(1024);
    server.installHttpResponder(body);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    // Two levels down, so that a name that climbs two levels would still land in the temporary directory
    const QString directory = root.filePath(QStringLiteral("a/b"));
    QVERIFY(QDir().mkpath(directory));

    QObject owner;
    Result result;
    const auto record = [&result](const QString& file, const QString& errorMsg) {
        result.calls++;
        result.file = file;
        result.error = errorMsg;
    };
    if (localName.isEmpty()) {
        expectLogMessage("Vehicle.VehicleFileFetch", QtWarningMsg, QRegularExpression("not a file name"));
        QVERIFY(!VehicleFileFetch::start(&owner, nullptr, server.url(path),
                                         {.directory = directory, .fileName = fileName}, record));
        verifyExpectedLogMessage();
        QCOMPARE(server.connectionCount(), 0);
    } else {
        QVERIFY(VehicleFileFetch::start(&owner, nullptr, server.url(path),
                                        {.directory = directory, .fileName = fileName}, record));
        QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::mediumMs());
        QCOMPARE(result.error, QString());
        QCOMPARE(result.file, QDir(directory).filePath(localName));
        QCOMPARE(TestFixtures::readFile(result.file), body);
    }

    QStringList written;
    QDirIterator files(root.path(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        written.append(QDir(root.path()).relativeFilePath(files.next()));
    }
    QCOMPARE(written, localName.isEmpty() ? QStringList() : QStringList{QStringLiteral("a/b/") + localName});
}

// The scheme is taken in any case, and a URI without one that names a host is HTTP, as the camera took it before
void VehicleFileFetchTest::_testHttpUriForms_data()
{
    QTest::addColumn<QString>("form");

    QTest::newRow("scheme in capitals") << QStringLiteral("HTTP://%1/file.bin");
    QTest::newRow("no scheme") << QStringLiteral("%1/file.bin");
}

void VehicleFileFetchTest::_testHttpUriForms()
{
    QFETCH(QString, form);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QByteArray body = fetchTestSizeFileContent(1024);
    server.installHttpResponder(body);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QObject owner;
    Result result;
    QVERIFY(VehicleFileFetch::start(&owner, nullptr, form.arg(QStringLiteral("127.0.0.1:%1").arg(server.port())),
                                    {.directory = directory.path()},
                                    [&result](const QString& file, const QString& errorMsg) {
                                        result.calls++;
                                        result.file = file;
                                        result.error = errorMsg;
                                    }));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::mediumMs());
    QCOMPARE(result.error, QString());
    QCOMPARE(TestFixtures::readFile(result.file), body);
    QCOMPARE(server.connectionCount(), 1);
}

// With a cache directory, a file that the server lets be cached is fetched from the cache the next time
void VehicleFileFetchTest::_testHttpCache()
{
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QByteArray body = fetchTestSizeFileContent(1024);
    server.installHttpResponder(body, 200, "application/octet-stream", 3600);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString cacheDirectory = QDir(directory.path()).filePath(QStringLiteral("cache"));

    QObject owner;
    const auto fetchFile = [&](const QString& fileName, const QString& httpCache) {
        Result result;
        VehicleFileFetch::Options options{.directory = directory.path(), .fileName = fileName};
        options.httpCacheDirectory = httpCache;
        if (!VehicleFileFetch::start(&owner, nullptr, server.url(QStringLiteral("/file.bin")), options,
                                     [&result](const QString& file, const QString& errorMsg) {
                                         result.calls++;
                                         result.file = file;
                                         result.error = errorMsg;
                                     })) {
            return QByteArrayLiteral("not started");
        }
        if (!UnitTest::waitForCondition([&result]() { return result.calls > 0; }, TestTimeout::mediumMs(),
                                        QStringLiteral("fetch result"))) {
            return QByteArrayLiteral("no result");
        }
        return result.error.isEmpty() ? TestFixtures::readFile(result.file) : result.error.toUtf8();
    };

    QCOMPARE(fetchFile(QStringLiteral("first.bin"), cacheDirectory), body);
    QCOMPARE(server.connectionCount(), 1);
    QCOMPARE(fetchFile(QStringLiteral("cached.bin"), cacheDirectory), body);
    QCOMPARE(server.connectionCount(), 1);
    QCOMPARE(fetchFile(QStringLiteral("uncached.bin"), QString()), body);
    QCOMPARE(server.connectionCount(), 2);
}

// A server certificate that cannot be verified, here a self-signed one, fails the fetch unless the caller accepts any
void VehicleFileFetchTest::_testServerCertificate_data()
{
    QTest::addColumn<bool>("verifyPeer");

    QTest::newRow("verified") << true;
    QTest::newRow("not verified") << false;
}

void VehicleFileFetchTest::_testServerCertificate()
{
    QFETCH(bool, verifyPeer);

    if (!QSslSocket::supportsSsl()) {
        QSKIP("No TLS backend");
    }

    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.setLocalCertificate(QSslCertificate(GPSTest::SERVER_CERT_PEM, QSsl::Pem));
    configuration.setPrivateKey(QSslKey(GPSTest::PRIVATE_KEY_PEM, QSsl::Rsa, QSsl::Pem));
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslServer server;
    server.setSslConfiguration(configuration);
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const QByteArray body("A camera definition");
    (void) connect(&server, &QSslServer::pendingConnectionAvailable, &server, [&server, body]() {
        while (QTcpSocket* const socket = server.nextPendingConnection()) {
            (void) connect(socket, &QTcpSocket::readyRead, socket, [socket, body]() {
                if (!socket->readAll().contains("\r\n\r\n")) {
                    return;
                }
                socket->write(QByteArrayLiteral("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: ") +
                              QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });

    if (verifyPeer) {
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("SSL")));
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QObject owner;
    Result result;
    QVERIFY(VehicleFileFetch::start(&owner, nullptr,
                                    QStringLiteral("https://127.0.0.1:%1/file").arg(server.serverPort()),
                                    {.directory = directory.path(), .verifyPeer = verifyPeer},
                                    [&result](const QString& file, const QString& errorMsg) {
                                        result.calls++;
                                        result.file = file;
                                        result.error = errorMsg;
                                    }));

    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::mediumMs());
    if (verifyPeer) {
        verifyExpectedLogMessage();
        QVERIFY(!result.error.isEmpty());
        QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList());
    } else {
        QCOMPARE(result.error, QString());
        QCOMPARE(TestFixtures::readFile(result.file), body);
    }
}

UT_REGISTER_TEST(VehicleFileFetchTest, TestLabel::Integration, TestLabel::Vehicle)
