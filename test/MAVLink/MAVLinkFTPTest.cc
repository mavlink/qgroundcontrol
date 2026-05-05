#include "MAVLinkFTPTest.h"

#include <QtCore/QByteArray>
#include <QtTest/QTest>
#include <cstring>

#include "MAVLinkFTP.h"

void MAVLinkFTPTest::_testDirectoryEntryCodec()
{
    MavlinkFTP::DirectoryEntry entry;
    entry.type = MavlinkFTP::DirectoryEntryType::File;
    entry.name = QString::fromUtf8("\xE6\x97\xA5\xE5\xBF\x97.ulg");
    entry.size = 4096;
    entry.modificationTime = 1700000000;

    const QString record = MavlinkFTP::formatDirectoryEntry(entry, true);
    QCOMPARE(record, QString::fromUtf8("F\xE6\x97\xA5\xE5\xBF\x97.ulg\t4096\t1700000000"));

    const MavlinkFTP::DirectoryEntryParseResult parsed = MavlinkFTP::parseDirectoryEntry(record);
    QVERIFY(parsed.valid());
    QCOMPARE(parsed.entry.type, MavlinkFTP::DirectoryEntryType::File);
    QCOMPARE(parsed.entry.name, entry.name);
    QCOMPARE(parsed.entry.size, entry.size);
    QCOMPARE(parsed.entry.modificationTime, entry.modificationTime);

    const MavlinkFTP::DirectoryEntryParseResult directory =
        MavlinkFTP::parseDirectoryEntry(QStringLiteral("D2026-07-14"));
    QVERIFY(directory.valid());
    QCOMPARE(directory.entry.type, MavlinkFTP::DirectoryEntryType::Directory);
    QCOMPARE(directory.entry.name, QStringLiteral("2026-07-14"));
    QVERIFY(!directory.entry.size.has_value());

    const MavlinkFTP::DirectoryEntryParseResult skip = MavlinkFTP::parseDirectoryEntry(QStringLiteral("S"));
    QVERIFY(skip.valid());
    QCOMPARE(skip.entry.type, MavlinkFTP::DirectoryEntryType::Skip);

    MavlinkFTP::Request response{};
    const QByteArray payload = record.toUtf8() + '\0' + QByteArray("D2026-07-14\0", 12);
    QVERIFY(payload.size() <= MavlinkFTP::dataCapacity);
    std::memcpy(response.data, payload.constData(), static_cast<size_t>(payload.size()));
    response.hdr.size = static_cast<uint8_t>(payload.size());
    const MavlinkFTP::DirectoryPayloadParseResult parsedPayload = MavlinkFTP::parseDirectoryPayload(response);
    QVERIFY(parsedPayload.valid());
    QCOMPARE(parsedPayload.records, QStringList({record, QStringLiteral("D2026-07-14")}));
}

void MAVLinkFTPTest::_testMalformedDirectoryEntries()
{
    QCOMPARE(MavlinkFTP::parseDirectoryEntry(QString()).error, MavlinkFTP::DirectoryEntryParseError::Empty);
    QCOMPARE(MavlinkFTP::parseDirectoryEntry(QStringLiteral("Xentry")).error,
             MavlinkFTP::DirectoryEntryParseError::UnknownType);

    const MavlinkFTP::DirectoryEntryParseResult missingSize =
        MavlinkFTP::parseDirectoryEntry(QStringLiteral("Fflight.ulg"));
    QCOMPARE(missingSize.error, MavlinkFTP::DirectoryEntryParseError::MissingSize);
    QCOMPARE(missingSize.entry.name, QStringLiteral("flight.ulg"));

    QCOMPARE(MavlinkFTP::parseDirectoryEntry(QStringLiteral("Fflight.ulg\tbad")).error,
             MavlinkFTP::DirectoryEntryParseError::InvalidSize);
    QCOMPARE(MavlinkFTP::parseDirectoryEntry(QStringLiteral("Fflight.ulg\t10\tbad")).error,
             MavlinkFTP::DirectoryEntryParseError::InvalidModificationTime);
    QCOMPARE(MavlinkFTP::parseDirectoryEntry(QStringLiteral("Fflight.ulg\t10\t20\textra")).error,
             MavlinkFTP::DirectoryEntryParseError::ExtraFields);
    QCOMPARE(MavlinkFTP::parseDirectoryEntry(QStringLiteral("Fflight.ulg\t10\t9223372036854775808")).error,
             MavlinkFTP::DirectoryEntryParseError::InvalidModificationTime);

    MavlinkFTP::Request response{};
    QCOMPARE(MavlinkFTP::parseDirectoryPayload(response).error, MavlinkFTP::DirectoryPayloadParseError::Empty);
    response.hdr.size = 3;
    response.data[0] = 'b';
    response.data[1] = 'a';
    response.data[2] = 'd';
    QCOMPARE(MavlinkFTP::parseDirectoryPayload(response).error,
             MavlinkFTP::DirectoryPayloadParseError::UnterminatedEntry);

    response = {};
    response.hdr.size = 3;
    response.data[0] = 'F';
    response.data[1] = 0xFF;
    response.data[2] = '\0';
    QCOMPARE(MavlinkFTP::parseDirectoryPayload(response).error, MavlinkFTP::DirectoryPayloadParseError::InvalidUtf8);

    response = {};
    response.hdr.size = 1;
    response.data[0] = '\0';
    QCOMPARE(MavlinkFTP::parseDirectoryPayload(response).error, MavlinkFTP::DirectoryPayloadParseError::EmptyEntry);

    MavlinkFTP::DirectoryEntry invalidEntry;
    QCOMPARE(MavlinkFTP::formatDirectoryEntry(invalidEntry), QString());
}

void MAVLinkFTPTest::_testMalformedDirectoryPayload_data()
{
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<int>("expectedError");

    QTest::newRow("empty-payload") << QByteArray() << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::Empty);
    QTest::newRow("unterminated-entry") << QByteArray("Flog.ulg\t1")
                                        << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::UnterminatedEntry);
    QTest::newRow("invalid-utf8") << QByteArray("F\xFF\0", 3)
                                  << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::InvalidUtf8);
    QTest::newRow("empty-entry") << QByteArray("\0", 1)
                                 << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::EmptyEntry);
    QTest::newRow("oversized-payload") << QByteArray(MavlinkFTP::dataCapacity + 1, 'x')
                                       << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::Oversized);
    QTest::newRow("valid-skip") << QByteArray("S\0", 2)
                                << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::None);
    QTest::newRow("valid-unicode") << QString::fromUtf8("F\xE6\x97\xA5\xE5\xBF\x97.ulg\t1").toUtf8().append('\0')
                                   << static_cast<int>(MavlinkFTP::DirectoryPayloadParseError::None);
}

void MAVLinkFTPTest::_testMalformedDirectoryPayload()
{
    QFETCH(QByteArray, payload);
    QFETCH(int, expectedError);

    MavlinkFTP::Request response{};
    response.hdr.size = static_cast<uint8_t>(payload.size());
    if (payload.size() <= MavlinkFTP::dataCapacity) {
        std::memcpy(response.data, payload.constData(), static_cast<size_t>(payload.size()));
    }

    const MavlinkFTP::DirectoryPayloadParseResult result = MavlinkFTP::parseDirectoryPayload(response);
    QCOMPARE(static_cast<int>(result.error), expectedError);
}

void MAVLinkFTPTest::_testRequestAndResponseCodec()
{
    MavlinkFTP::Request request{};
    const QString path = QString::fromUtf8("/\xE6\x97\xA5\xE5\xBF\x97.ulg");
    const QByteArray encodedPath = path.toUtf8();
    QVERIFY(MavlinkFTP::setRequestData(request, path));
    QCOMPARE(request.hdr.size, encodedPath.size());
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(request.data), request.hdr.size), encodedPath);

    const QString oversizedPath(MavlinkFTP::dataCapacity + 1, QLatin1Char('x'));
    QVERIFY(!MavlinkFTP::setRequestData(request, oversizedPath));
    QCOMPARE(request.hdr.size, 0);

    const QString embeddedNullPath = QStringLiteral("/log") + QChar::Null + QStringLiteral("hidden");
    QVERIFY(!MavlinkFTP::setRequestData(request, embeddedNullPath));
    QCOMPARE(request.hdr.size, 0);

    MavlinkFTP::setOpenFileLength(request, 0x12345678U);
    QCOMPARE(MavlinkFTP::decodeOpenFileLength(request), std::optional<uint32_t>(0x12345678U));
    request.hdr.size = 0;
    QVERIFY(!MavlinkFTP::decodeOpenFileLength(request).has_value());

    MavlinkFTP::Request response{};
    response.hdr.opcode = MAV_FTP_OPCODE_ACK;
    response.hdr.req_opcode = MAV_FTP_OPCODE_OPENFILERO;
    QCOMPARE(MavlinkFTP::validateResponse(response, MAV_FTP_OPCODE_OPENFILERO).result,
             MavlinkFTP::ResponseValidationResult::Valid);
    QCOMPARE(MavlinkFTP::validateResponse(response, MAV_FTP_OPCODE_READFILE).result,
             MavlinkFTP::ResponseValidationResult::Unrelated);

    response.hdr.opcode = MAV_FTP_OPCODE_NONE;
    const MavlinkFTP::ResponseValidation invalidOpcode =
        MavlinkFTP::validateResponse(response, MAV_FTP_OPCODE_OPENFILERO);
    QCOMPARE(invalidOpcode.result, MavlinkFTP::ResponseValidationResult::Malformed);
    QCOMPARE(invalidOpcode.error, MavlinkFTP::ResponseValidationError::InvalidResponseOpcode);

    response.hdr.opcode = MAV_FTP_OPCODE_NAK;
    response.hdr.size = 0;
    QCOMPARE(MavlinkFTP::validateResponse(response, MAV_FTP_OPCODE_OPENFILERO).error,
             MavlinkFTP::ResponseValidationError::MissingNakErrorCode);

    response.hdr.size = 2;
    response.data[0] = MAV_FTP_ERR_FAILERRNO;
    response.data[1] = 17;
    const std::optional<MavlinkFTP::NakError> nakError = MavlinkFTP::decodeNak(response);
    QVERIFY(nakError.has_value());
    QCOMPARE(nakError->code, MAV_FTP_ERR_FAILERRNO);
    QCOMPARE(nakError->errorNumber, std::optional<uint8_t>(17));
    QCOMPARE(MavlinkFTP::validateResponse(response, MAV_FTP_OPCODE_OPENFILERO).result,
             MavlinkFTP::ResponseValidationResult::Valid);

    response.hdr.size = 1;
    QVERIFY(!MavlinkFTP::decodeNak(response).has_value());
    QCOMPARE(MavlinkFTP::validateResponse(response, MAV_FTP_OPCODE_OPENFILERO).error,
             MavlinkFTP::ResponseValidationError::InvalidNakPayload);
}

void MAVLinkFTPTest::_testUriParsing()
{
    const QString oversizedPath(MavlinkFTP::dataCapacity + 1, QLatin1Char('x'));

    QVERIFY(MavlinkFTP::isMavlinkFtpUri(QStringLiteral("MFTP://vehicle/file")));
    QVERIFY(!MavlinkFTP::isMavlinkFtpUri(QStringLiteral("https://vehicle/file")));

    const MavlinkFTP::UriParseResult rawPath =
        MavlinkFTP::parseUri(MAV_COMP_ID_ALL, QStringLiteral("/fs/microsd/log.ulg"));
    QVERIFY(rawPath.valid());
    QCOMPARE(rawPath.path, QStringLiteral("/fs/microsd/log.ulg"));
    QCOMPARE(rawPath.componentId, static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    const MavlinkFTP::UriParseResult componentUri =
        MavlinkFTP::parseUri(MAV_COMP_ID_ALL, QStringLiteral("mftp://[;comp=42]general.json"));
    QVERIFY(componentUri.valid());
    QCOMPARE(componentUri.path, QStringLiteral("/general.json"));
    QCOMPARE(componentUri.componentId, static_cast<uint8_t>(42));

    const MavlinkFTP::UriParseResult absoluteUri =
        MavlinkFTP::parseUri(3, QStringLiteral("mftp:///fs/microsd/log.ulg"));
    QVERIFY(absoluteUri.valid());
    QCOMPARE(absoluteUri.path, QStringLiteral("/fs/microsd/log.ulg"));
    QCOMPARE(absoluteUri.componentId, static_cast<uint8_t>(3));

    const MavlinkFTP::UriParseResult pathSelector = MavlinkFTP::parseUri(3, QStringLiteral("/[;comp=7]log.ulg"));
    QVERIFY(pathSelector.valid());
    QCOMPARE(pathSelector.path, QStringLiteral("/log.ulg"));
    QCOMPARE(pathSelector.componentId, static_cast<uint8_t>(7));

    QCOMPARE(MavlinkFTP::parseUri(1, QStringLiteral("https://example.com/file")).error,
             MavlinkFTP::UriParseError::InvalidScheme);
    QCOMPARE(MavlinkFTP::parseUri(1, QStringLiteral("mftp://[;comp=256]file")).error,
             MavlinkFTP::UriParseError::InvalidComponentId);
    QCOMPARE(MavlinkFTP::parseUri(1, QStringLiteral("mftp://[;comp=bad]file")).error,
             MavlinkFTP::UriParseError::InvalidComponentId);
    QCOMPARE(MavlinkFTP::parseUri(1, oversizedPath).error, MavlinkFTP::UriParseError::PathTooLong);

    const QString maximumMultibytePath = QString(MavlinkFTP::dataCapacity / 2, QChar(0x00E9));
    QCOMPARE(maximumMultibytePath.toUtf8().size(), MavlinkFTP::dataCapacity - 1);
    QVERIFY(MavlinkFTP::parseUri(1, maximumMultibytePath).valid());
    QCOMPARE(MavlinkFTP::parseUri(1, maximumMultibytePath + QChar(0x00E9)).error,
             MavlinkFTP::UriParseError::PathTooLong);

    const QString embeddedNullUri = QStringLiteral("mftp:///log") + QChar::Null + QStringLiteral("hidden");
    QCOMPARE(MavlinkFTP::parseUri(1, embeddedNullUri).error, MavlinkFTP::UriParseError::EmbeddedNull);
}

void MAVLinkFTPTest::_testUriValidation_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<int>("expectedError");

    QTest::newRow("plain-path") << QStringLiteral("/log.ulg") << static_cast<int>(MavlinkFTP::UriParseError::None);
    QTest::newRow("other-scheme") << QStringLiteral("https://example.com/log.ulg")
                                  << static_cast<int>(MavlinkFTP::UriParseError::InvalidScheme);
    QTest::newRow("component-overflow") << QStringLiteral("mftp://[;comp=256]log.ulg")
                                        << static_cast<int>(MavlinkFTP::UriParseError::InvalidComponentId);
    QTest::newRow("component-text") << QStringLiteral("mftp://[;comp=bad]log.ulg")
                                    << static_cast<int>(MavlinkFTP::UriParseError::InvalidComponentId);
    QTest::newRow("component-unterminated")
        << QStringLiteral("mftp://[;comp=1log.ulg") << static_cast<int>(MavlinkFTP::UriParseError::InvalidComponentId);
    QTest::newRow("embedded-null") << (QStringLiteral("mftp:///log") + QChar::Null + QStringLiteral("hidden"))
                                   << static_cast<int>(MavlinkFTP::UriParseError::EmbeddedNull);
    QTest::newRow("ascii-overflow") << QString(MavlinkFTP::dataCapacity + 1, QLatin1Char('x'))
                                    << static_cast<int>(MavlinkFTP::UriParseError::PathTooLong);
    QTest::newRow("multibyte-boundary") << QString(MavlinkFTP::dataCapacity / 2, QChar(0x00E9))
                                        << static_cast<int>(MavlinkFTP::UriParseError::None);
    QTest::newRow("multibyte-overflow") << QString((MavlinkFTP::dataCapacity / 2) + 1, QChar(0x00E9))
                                        << static_cast<int>(MavlinkFTP::UriParseError::PathTooLong);
}

void MAVLinkFTPTest::_testUriValidation()
{
    QFETCH(QString, uri);
    QFETCH(int, expectedError);

    QCOMPARE(static_cast<int>(MavlinkFTP::parseUri(MAV_COMP_ID_AUTOPILOT1, uri).error), expectedError);
}

void MAVLinkFTPTest::_testGeneratedEnumStrings()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME),
             QStringLiteral("List Directory With Time"));
    QCOMPARE(MavlinkFTP::errorCodeToString(MAV_FTP_ERR_FILENOTFOUND), QStringLiteral("File Not Found"));
    QCOMPARE(MavlinkFTP::opCodeToString(static_cast<MAV_FTP_OPCODE>(255)), QStringLiteral("Unknown OpCode"));
    QCOMPARE(MavlinkFTP::errorCodeToString(static_cast<MAV_FTP_ERR>(255)), QStringLiteral("Unknown Error"));
}

UT_REGISTER_TEST(MAVLinkFTPTest, TestLabel::Unit)
