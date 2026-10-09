#include "NTRIPHttpCodecTest.h"

#include <algorithm>
#include <chrono>
#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtTest/QTest>

#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "NTRIPHttpCodec.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QGCNetworkClient.h"

void NTRIPHttpCodecTest::_parseStatusLine_data()
{
    QTest::addColumn<QByteArray>("line");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("reason");
    QTest::newRow("http-200") << QByteArray("HTTP/1.1 200 OK") << true << 200 << QStringLiteral("OK");
    QTest::newRow("icy-200") << QByteArray("ICY 200 OK") << true << 200 << QStringLiteral("OK");
    QTest::newRow("source-table") << QByteArray("SOURCETABLE 200 OK") << true << 200 << QStringLiteral("OK");
    QTest::newRow("unauthorized") << QByteArray("HTTP/1.0 401 Unauthorized") << true << 401
                                  << QStringLiteral("Unauthorized");
    QTest::newRow("not-found") << QByteArray("HTTP/1.1 404 Not Found") << true << 404 << QStringLiteral("Not Found");
    QTest::newRow("created") << QByteArray("HTTP/1.1 201 Created") << true << 201 << QStringLiteral("Created");
    QTest::newRow("server-error") << QByteArray("HTTP/1.0 500 Internal Server Error") << true << 500
                                  << QStringLiteral("Internal Server Error");
    QTest::newRow("no-reason") << QByteArray("HTTP/1.1 400") << true << 400 << QString();
    QTest::newRow("empty") << QByteArray() << false << 0 << QString();
    QTest::newRow("garbage") << QByteArray("garbage data") << false << 0 << QString();
    QTest::newRow("missing-protocol") << QByteArray("200 OK") << false << 0 << QString();
}

void NTRIPHttpCodecTest::_parseStatusLine()
{
    QFETCH(QByteArray, line);
    QFETCH(bool, valid);
    QFETCH(int, code);
    QFETCH(QString, reason);
    const auto status = NTRIPHttpDecoder::parseStatusLine(line);
    QCOMPARE(status.valid, valid);
    QCOMPARE(status.code, code);
    QCOMPARE(status.reason, reason);
}

void NTRIPHttpCodecTest::_buildRequest_data()
{
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<bool>("useTls");
    for (const bool useTls : {false, true}) {
        const char* suffix = useTls ? "tls" : "plaintext";
        QTest::addRow("anonymous-%s", suffix) << QString{} << QString{} << QByteArray{} << useTls;
        QTest::addRow("username-and-password-%s", suffix)
            << QStringLiteral("user") << QStringLiteral("pass") << QByteArray("dXNlcjpwYXNz") << useTls;
        QTest::addRow("username-only-%s", suffix)
            << QStringLiteral("user") << QString{} << QByteArray("dXNlcjo=") << useTls;
        QTest::addRow("password-only-%s", suffix)
            << QString{} << QStringLiteral("pass") << QByteArray("OnBhc3M=") << useTls;
        QTest::addRow("utf8-credentials-%s", suffix)
            << QString::fromUtf8("Us\xc3\xa9r") << QString::fromUtf8("Pa\xc3\x9fs") << QByteArray("VXPDqXI6UGHDn3M=")
            << useTls;
    }
}

void NTRIPHttpCodecTest::_buildRequest()
{
    QFETCH(QString, username);
    QFETCH(QString, password);
    QFETCH(QByteArray, encoded);
    QFETCH(bool, useTls);
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("Caster.Example.com");
    config.mountpoint = QStringLiteral("MixedCase_1");
    config.username = username;
    config.password = password;
    config.useTls = useTls;
    QVERIFY(config.streamValidationError().isEmpty());
    const auto request = NTRIPHttpRequest::build(config);
    const QByteArray authorization = encoded.isEmpty() ? QByteArray{} : "Authorization: Basic " + encoded + "\r\n";
    QVERIFY(request.error.isEmpty());
    // Only credentials sent without TLS ask the caller to warn.
    QCOMPARE(config.sendsCredentialsInClear(), !encoded.isEmpty() && !useTls);
    QCOMPARE(config.credentialsInClearWarning().isEmpty(), encoded.isEmpty() || useTls);
    QCOMPARE(request.bytes,
             "GET /MixedCase_1 HTTP/1.1\r\n"
             "Host: caster.example.com:2101\r\n"
             "Ntrip-Version: Ntrip/2.0\r\n"
             "User-Agent: NTRIP " +
                 QGCNetworkHelper::defaultUserAgent().toLatin1() + "\r\n" + authorization + "\r\n");
}

void NTRIPHttpCodecTest::_buildRequestAuthority_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QByteArray>("authority");
    QTest::newRow("hostname") << QStringLiteral("Caster.Example.com") << 2101 << QByteArray("caster.example.com:2101");
    QTest::newRow("ipv4") << QStringLiteral("127.0.0.1") << 8080 << QByteArray("127.0.0.1:8080");
    QTest::newRow("ipv6") << QStringLiteral("::1") << 2101 << QByteArray("[::1]:2101");
}

void NTRIPHttpCodecTest::_buildRequestAuthority()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QByteArray, authority);
    NTRIPConnectionConfig configuration;
    configuration.host = host;
    configuration.port = port;
    configuration.mountpoint = QStringLiteral("TEST");
    configuration.username = QStringLiteral("user");
    configuration.password = QStringLiteral("pass");
    const auto stream = NTRIPHttpRequest::build(configuration);
    const auto table = NTRIPHttpRequest::build(configuration, NTRIPHttpPurpose::SourceTable);
    QVERIFY(stream.error.isEmpty());
    QVERIFY(table.error.isEmpty());
    QVERIFY(stream.bytes.contains("Host: " + authority + "\r\n"));
    QVERIFY(table.bytes.contains("Host: " + authority + "\r\n"));
    QCOMPARE(table.bytes.sliced(table.bytes.indexOf("\r\n")), stream.bytes.sliced(stream.bytes.indexOf("\r\n")));
    QVERIFY(table.bytes.startsWith("GET / HTTP/1.1\r\n"));
}

void NTRIPHttpCodecTest::_buildRequestRejectsInvalidConfig_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("mountpoint");
    QTest::newRow("host-crlf") << QStringLiteral("caster\r\nInjected: value") << QStringLiteral("TEST");
    QTest::newRow("host-space") << QStringLiteral("caster example") << QStringLiteral("TEST");
    QTest::newRow("host-del") << QStringLiteral("caster\x7f") << QStringLiteral("TEST");
    QTest::newRow("mountpoint-crlf") << QStringLiteral("localhost") << QStringLiteral("TEST\r\nInjected: value");
    QTest::newRow("mountpoint-space") << QStringLiteral("localhost") << QStringLiteral("TEST OTHER");
    QTest::newRow("mountpoint-del") << QStringLiteral("localhost") << QStringLiteral("TEST\x7f");
    QTest::newRow("mountpoint-empty") << QStringLiteral("localhost") << QString();
}

void NTRIPHttpCodecTest::_buildRequestRejectsInvalidConfig()
{
    QFETCH(QString, host);
    QFETCH(QString, mountpoint);
    NTRIPConnectionConfig config;
    config.host = host;
    config.mountpoint = mountpoint;
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    const auto request = NTRIPHttpRequest::build(config);
    QVERIFY(!request.error.isEmpty());
    QVERIFY(request.bytes.isEmpty());
}

void NTRIPHttpCodecTest::_framing_data()
{
    QTest::addColumn<QByteArray>("wire");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("connected");
    QTest::addColumn<int>("error");
    const int invalid = static_cast<int>(NTRIPError::InvalidHttpResponse);
    const int oversized = static_cast<int>(NTRIPError::HeaderTooLarge);
    const int mountpoint = static_cast<int>(NTRIPError::InvalidMountpoint);
    const QByteArray ok = "HTTP/1.1 200 OK\r\n";
    QTest::newRow("close-delimited") << ok + "\r\nabc" << QByteArray("abc") << true << -1;
    QTest::newRow("content-length") << ok + "Content-Length: 3\r\n\r\nabc" << QByteArray("abc") << true << -1;
    QTest::newRow("duplicate-length") << ok + "Content-Length: 3, 03\r\ncontent-length: 3\r\n\r\nabc"
                                      << QByteArray("abc") << true << -1;
    QTest::newRow("zero-length") << ok + "Content-Length: 0\r\n\r\n" << QByteArray() << true << -1;
    QTest::newRow("empty-status") << QByteArray("HTTP/1.1 204 No Content\r\n\r\n") << QByteArray() << true << -1;
    QTest::newRow("informational") << QByteArray("HTTP/1.1 100 Continue\r\n\r\n") + ok + "\r\nabc" << QByteArray("abc")
                                   << true << -1;
    const QByteArray binary = QByteArray::fromHex("d30000");
    QTest::newRow("bare-icy") << QByteArray("ICY 200 OK\r\n") + binary << binary << true << -1;
    QTest::newRow("icy-separator") << QByteArray("ICY 200 OK\r\n\r\n") + binary << binary << true << -1;
    QTest::newRow("icy-headers") << QByteArray("icy 200 OK\r\nServer: legacy\r\nContent-Length: 3\r\n\r\n") + binary
                                 << binary << true << -1;
    QTest::newRow("icy-non-header-prefix") << QByteArray("ICY 200 OK\r\n Content-Length: 3\r\n\r\n")
                                           << QByteArray(" Content-Length: 3\r\n\r\n") << true << -1;
    QTest::newRow("icy-missing-separator")
        << QByteArray("ICY 200 OK\r\nContent-Length: 3\r\n") + binary << binary << true << -1;
    QTest::newRow("source-table") << QByteArray("SOURCETABLE 200 OK\r\nSTR;MP\r\n") << QByteArray() << false
                                  << mountpoint;
    QTest::newRow("source-content-type") << ok + "Content-Type: gnss/sourcetable; charset=utf-8\r\n\r\n"
                                         << QByteArray() << false << mountpoint;
    const QByteArray chunked = ok + "Transfer-Encoding: chunked\r\n\r\n";
    QTest::newRow("chunks") << chunked + "1;name=\"escaped\\\"value\"\r\na\r\n2\r\nbc\r\n0\r\nX-End: yes\r\n\r\n"
                            << QByteArray("abc") << true << -1;
    QTest::newRow("chunk-extension-whitespace") << chunked + "3 \t; name = value ;flag\r\nabc\r\n0\r\n\r\n"
                                                << QByteArray("abc") << true << -1;
    QTest::newRow("malformed-chunk-extensions-ignored")
        << chunked + "1;=x\r\na\r\n1;x=\"\r\nb\r\n1;x=\vvalue\r\nc\r\n0\r\n\r\n"
        << QByteArray("abc") << true << -1;
    QTest::newRow("truncated-status") << QByteArray("HTTP/1.1 20") << QByteArray() << false << invalid;
    QTest::newRow("truncated-headers") << ok + "Server: x\r\n" << QByteArray() << false << invalid;
    QTest::newRow("truncated-length") << ok + "Content-Length: 4\r\n\r\nabc" << QByteArray("abc") << true << invalid;
    QTest::newRow("maximum-length-streamed")
        << ok + "Content-Length: 18446744073709551615\r\n\r\nabc" << QByteArray("abc") << true << invalid;
    QTest::newRow("extra-body") << ok + "Content-Length: 2\r\n\r\nabc" << QByteArray("ab") << true << invalid;
    QTest::newRow("truncated-chunk") << chunked + "4\r\nabc" << QByteArray("abc") << true << invalid;
    QTest::newRow("truncated-chunk-end") << chunked + "3\r\nabc\r" << QByteArray("abc") << true << invalid;
    QTest::newRow("missing-last-chunk") << chunked + "3\r\nabc\r\n" << QByteArray("abc") << true << invalid;
    QTest::newRow("truncated-trailer") << chunked + "3\r\nabc\r\n0\r\nX-End: yes\r\n"
                                       << QByteArray("abc") << true << invalid;
    QTest::newRow("bad-chunk-end") << chunked + "3\r\nabc!\n" << QByteArray("abc") << true << invalid;
    QTest::newRow("bad-trailer") << chunked + "0\r\nFolded: x\r\n y\r\n\r\n" << QByteArray() << true << invalid;
    QTest::newRow("framing-trailer") << chunked + "0\r\nContent-Length: 3\r\n\r\n" << QByteArray() << true << invalid;
    QTest::newRow("maximum-chunk-streamed") << chunked + "1000000\r\nx" << QByteArray("x") << true << invalid;
    for (const QByteArray size : {"-1", "+1", " 1", "1 ", "0x1", "1000001", "10000000000000000", "g;x"}) {
        QTest::addRow("chunk-%s", size.constData()) << chunked + size + "\r\n" << QByteArray() << true << invalid;
    }
    for (const QByteArray header : {"Content-Length: -1",
                                    "Content-Length: +1",
                                    "Content-Length: 1.0",
                                    "Content-Length:",
                                    "Content-Length: 18446744073709551616",
                                    "Content-Length: 3,4",
                                    "Content-Length: 3\r\nContent-Length: 4",
                                    "Content-Length: 3\r\nTransfer-Encoding: chunked",
                                    "Transfer-Encoding: chunked\r\nContent-Length: 3",
                                    "Transfer-Encoding: gzip, chunked",
                                    "Transfer-Encoding: identity",
                                    "Transfer-Encoding: chunked, chunked",
                                    "Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked",
                                    "Content-Encoding: gzip",
                                    "Bad Header: x",
                                    "Content-Length : 3",
                                    " Content-Length: 3",
                                    "X: value\r\n folded",
                                    "X: value\nOther: value",
                                    "X: bad\rvalue",
                                    "X: bad\x01value"}) {
        QTest::newRow(header.constData()) << ok + header + "\r\n\r\n" << QByteArray() << false << invalid;
    }
    QTest::newRow("nul-header") << ok + QByteArray("X: a\0b\r\n\r\n", 10) << QByteArray() << false << invalid;
    QTest::newRow("del-header") << ok + "X: value\x7f\r\n\r\n" << QByteArray() << false << invalid;
    for (const QByteArray status : {"garbage", "noise\r\nHTTP/1.1 200 OK", "HTTP/2 200 OK", "HTTP/1.1 600 Bad",
                                    "HTTP/1.1 2000 OK", "HTTP/1.1\t200 OK"}) {
        QTest::newRow(status.constData()) << status + "\r\n\r\n" << QByteArray() << false << invalid;
    }
    QTest::newRow("switching-protocol") << QByteArray("HTTP/1.1 101 Switching Protocols\r\n\r\n") << QByteArray()
                                        << false << invalid;
    QTest::newRow("too-many-informationals")
        << QByteArray("HTTP/1.1 100 Continue\r\n\r\n").repeated(5) << QByteArray() << false << invalid;
    QTest::newRow("http10-chunked") << QByteArray("HTTP/1.0 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n")
                                    << QByteArray() << false << invalid;
    QTest::newRow("line-limit") << ok + "X: " + QByteArray(NTRIPHttpDecoder::MAX_LINE_BYTES - 5, 'x') + "\r\n\r\n"
                                << QByteArray() << true << -1;
    QTest::newRow("oversized-line") << ok + "X: " + QByteArray(NTRIPHttpDecoder::MAX_LINE_BYTES - 4, 'x') + "\r\n\r\n"
                                    << QByteArray() << false << oversized;
    const QByteArray largeHeader = "X: " + QByteArray(8187, 'x') + "\r\n";
    const QByteArray nearLimit = ok + largeHeader.repeated(3);
    const auto remaining = NTRIPHttpDecoder::MAX_HEADER_BYTES - nearLimit.size() - 7;
    QTest::newRow("header-limit") << nearLimit + "X: " + QByteArray(remaining, 'x') + "\r\n\r\n"
                                  << QByteArray() << true << -1;
    QTest::newRow("oversized-header") << nearLimit + "X: " + QByteArray(remaining + 1, 'x') + "\r\n\r\n"
                                      << QByteArray() << false << oversized;
    QTest::newRow("header-count-limit") << ok + QByteArray("X: y\r\n").repeated(128) + "\r\n"
                                        << QByteArray() << true << -1;
    QTest::newRow("too-many-headers") << ok + QByteArray("X: y\r\n").repeated(129) + "\r\n"
                                      << QByteArray() << false << oversized;
}

void NTRIPHttpCodecTest::_framing()
{
    QFETCH(QByteArray, wire);
    QFETCH(QByteArray, body);
    QFETCH(bool, connected);
    QFETCH(int, error);
    const auto now = QDateTime::fromString(QStringLiteral("2026-09-16T12:00:00Z"), Qt::ISODate);
    for (qsizetype fragment : {qsizetype(1), qsizetype(7), wire.size()}) {
        NTRIPHttpDecoder decoder;
        QByteArray decoded;
        int handshakes = 0;
        bool complete = false;
        std::optional<NTRIPFailure> failure;
        const auto consume = [&](const NTRIPHttpDecoder::Result& result) {
            decoded += result.body;
            handshakes += result.connected;
            complete = result.complete;
            if (result.failure) {
                failure = result.failure;
            }
        };
        for (qsizetype offset = 0; offset < wire.size(); offset += fragment) {
            consume(decoder.feed(QByteArrayView(wire).sliced(offset, std::min(fragment, wire.size() - offset)), now));
        }
        consume(decoder.finish());
        QCOMPARE(decoded, body);
        QCOMPARE(handshakes, connected ? 1 : 0);
        QCOMPARE(failure ? static_cast<int>(failure->code) : -1, error);
        QCOMPARE(complete, error == -1);
        decoder.reset();
        const auto fresh = decoder.feed("HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nx", now);
        QVERIFY(fresh.connected && fresh.complete && !fresh.failure);
        QCOMPARE(fresh.body, QByteArray("x"));
    }
}

void NTRIPHttpCodecTest::_errorDiagnostics_data()
{
    QTest::addColumn<QByteArray>("wire");
    QTest::addColumn<int>("error");
    QTest::addColumn<QString>("preview");
    QTest::addColumn<int>("retryMs");
    const int http = static_cast<int>(NTRIPError::HttpError);
    // A refused request cannot succeed when repeated; other statuses may.
    const int rejected = static_cast<int>(NTRIPError::RequestRejected);
    const QByteArray denied = "HTTP/1.1 403 Forbidden\r\n";
    const QByteArray unavailable = "HTTP/1.1 503 Unavailable\r\nRetry-After: 120\r\n";
    const QByteArray html = "<b>Mountpoint denied</b>\nUse another mountpoint";
    const QString preview = QStringLiteral("Mountpoint denied Use another mountpoint");
    const QByteArray length = "Content-Length: " + QByteArray::number(html.size()) + "\r\n\r\n";
    QTest::newRow("content-length") << denied + length + html << rejected << preview << 0;
    QTest::newRow("close-delimited") << denied + "\r\n" + html << rejected << preview << 0;
    QTest::newRow("truncated-length") << denied + "Content-Length: 1000\r\n\r\n" + html << rejected << preview << 0;
    QTest::newRow("bad-request") << QByteArray("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n") << rejected
                                 << QStringLiteral("HTTP 400") << 0;
    QTest::newRow("not-found") << QByteArray("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n") << http
                               << QStringLiteral("HTTP 404") << 0;
    const QByteArray chunks =
        unavailable + "Transfer-Encoding: chunked\r\n\r\n" + QByteArray::number(html.size(), 16) + "\r\n" + html;
    QTest::newRow("chunked") << chunks + "\r\n0\r\n\r\n" << http << preview << 120000;
    QTest::newRow("malformed-error-chunk") << chunks + "!\n" << http << preview << 120000;
    QTest::newRow("binary-error-body") << denied + "\r\n" + GPSTest::rtcmMessage(1005) << rejected << QString() << 0;
    QTest::newRow("control-characters") << denied + "\r\nAccess\x01 denied\nRetry later" << rejected
                                        << QStringLiteral("Access denied Retry later") << 0;
    QTest::newRow("compressed-authentication")
        << QByteArray("HTTP/1.1 401 Unauthorized\r\nContent-Encoding: gzip\r\nContent-Length: 20\r\n\r\n") +
               QByteArray::fromHex("1f8b080000000000000303000000000000000000")
        << static_cast<int>(NTRIPError::AuthFailed) << QString() << 0;
    QTest::newRow("compressed-retry") << unavailable + "Content-Encoding: gzip\r\n\r\n" << http << QString() << 120000;
    QTest::newRow("unsupported-error-transfer") << unavailable + "Transfer-Encoding: gzip, chunked\r\n\r\n"
                                                << http << QString() << 120000;
}

void NTRIPHttpCodecTest::_errorDiagnostics()
{
    QFETCH(QByteArray, wire);
    QFETCH(int, error);
    QFETCH(QString, preview);
    QFETCH(int, retryMs);
    for (const qsizetype fragment : {qsizetype(1), qsizetype(7), wire.size()}) {
        NTRIPHttpDecoder decoder;
        std::optional<NTRIPFailure> failure;
        int failures = 0;
        const auto consume = [&](const NTRIPHttpDecoder::Result& result) {
            QVERIFY(result.body.isEmpty());
            QVERIFY(!result.connected);
            QVERIFY(!result.complete);
            if (result.failure) {
                failure = result.failure;
                ++failures;
            }
        };
        for (qsizetype offset = 0; offset < wire.size(); offset += fragment) {
            consume(decoder.feed(QByteArrayView(wire).sliced(offset, std::min(fragment, wire.size() - offset)),
                                 QDateTime::currentDateTimeUtc()));
        }
        consume(decoder.finish());
        QCOMPARE(failures, 1);
        QVERIFY(failure);
        QCOMPARE(static_cast<int>(failure->code), error);
        QCOMPARE(failure->retryAfter, std::chrono::milliseconds(retryMs));
        QVERIFY2(failure->detail.contains(preview), qPrintable(failure->detail));
        QVERIFY(!failure->detail.contains(QLatin1Char('<')));
        QVERIFY(!failure->detail.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]"))));
        QVERIFY(!failure->detail.contains(QStringLiteral("encoding")));
        const auto separator = failure->detail.indexOf(QStringLiteral(" \u2014 "));
        if (separator >= 0) {
            QVERIFY(failure->detail.size() - separator - 3 <= NTRIPHttpDecoder::MAX_ERROR_PREVIEW_CHARS);
        }
    }
}

void NTRIPHttpCodecTest::_errorBodyBounds()
{
    NTRIPHttpDecoder decoder;
    auto result = decoder.feed("HTTP/1.1 403 Forbidden\r\nContent-Length: 1000000\r\n\r\n", {});
    QVERIFY(decoder.awaitingErrorBody());
    QVERIFY(!result.failure);
    result = decoder.feed(QByteArray(NTRIPHttpDecoder::MAX_ERROR_BODY_BYTES - 1, 'x'), {});
    QVERIFY(decoder.awaitingErrorBody());
    QVERIFY(!result.failure);
    result = decoder.feed("x", {});
    QVERIFY(result.failure);
    QVERIFY(!decoder.awaitingErrorBody());
    QVERIFY(result.body.isEmpty());
    QCOMPARE(result.failure->detail, QStringLiteral("HTTP 403: Forbidden \u2014 ") +
                                         QString(NTRIPHttpDecoder::MAX_ERROR_PREVIEW_CHARS, QLatin1Char('x')));
    QVERIFY(!decoder.feed("must not be appended", {}).failure);
}

void NTRIPHttpCodecTest::_retryAfter_data()
{
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("delayMs");
    QTest::newRow("zero") << QByteArray("0") << 0;
    QTest::newRow("delta") << QByteArray("17") << 17000;
    QTest::newRow("cap") << QByteArray("301") << 300000;
    QTest::newRow("uint64-max") << QByteArray("18446744073709551615") << 300000;
    QTest::newRow("overflow") << QByteArray("18446744073709551616") << 0;
    QTest::newRow("negative") << QByteArray("-10") << 0;
    QTest::newRow("positive-sign") << QByteArray("+10") << 0;
    QTest::newRow("decimal") << QByteArray("1.5") << 0;
    QTest::newRow("invalid") << QByteArray("tomorrow") << 0;
    QTest::newRow("empty") << QByteArray() << 0;
    QTest::newRow("duplicate") << QByteArray("10\r\nRetry-After: 20") << 0;
    QTest::newRow("date") << QByteArray("Wed, 16 Sep 2026 12:00:10 GMT") << 9500;
    QTest::newRow("rfc850-date") << QByteArray("Wednesday, 16-Sep-26 12:00:10 GMT") << 9500;
    QTest::newRow("asctime-date") << QByteArray("Wed Sep 16 12:00:10 2026") << 9500;
    QTest::newRow("asctime-single-digit-day") << QByteArray("Tue Oct  6 12:00:10 2026") << 300000;
    QTest::newRow("rfc850-previous-century") << QByteArray("Friday, 16-Sep-94 12:00:10 GMT") << 0;
    QTest::newRow("past-date") << QByteArray("Wed, 16 Sep 2026 11:59:59 GMT") << 0;
    QTest::newRow("capped-date") << QByteArray("Wed, 16 Sep 2026 12:10:00 GMT") << 300000;
    QTest::newRow("invalid-date") << QByteArray("Mon, 31 Feb 2026 12:00:10 GMT") << 0;
}

void NTRIPHttpCodecTest::_retryAfter()
{
    QFETCH(QByteArray, value);
    QFETCH(int, delayMs);
    const QByteArray wire = "HTTP/1.1 503 Unavailable\r\nRetry-After: " + value + "\r\nContent-Length: 0\r\n\r\n";
    const auto now = QDateTime::fromString(QStringLiteral("2026-09-16T12:00:00.500Z"), Qt::ISODateWithMs);
    const auto result = NTRIPHttpDecoder().feed(wire, now);
    QVERIFY(result.failure);
    QCOMPARE(result.failure->code, NTRIPError::HttpError);
    QCOMPARE(result.failure->retryAfter, std::chrono::milliseconds(delayMs));
    if (value.contains("GMT")) {
        // A date cannot be resolved without the current time.
        const auto undated = NTRIPHttpDecoder().feed(wire, {});
        QVERIFY(undated.failure);
        QCOMPARE(undated.failure->retryAfter, std::chrono::milliseconds(0));
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPHttpCodecTest, TestLabel::Unit)
