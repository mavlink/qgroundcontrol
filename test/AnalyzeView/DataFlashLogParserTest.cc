#include "DataFlashLogParserTest.h"

#include "DataFlashLogParser.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

#include <cstring>

namespace {

QByteArray makeFmtPayload(uint8_t type, uint8_t length, const char *name, const char *format, const char *columns)
{
    QByteArray payload(86, '\0');
    payload[0] = static_cast<char>(type);
    payload[1] = static_cast<char>(length);
    memcpy(payload.data() + 2, name, qMin<int>(4, static_cast<int>(strlen(name))));
    memcpy(payload.data() + 6, format, qMin<int>(16, static_cast<int>(strlen(format))));
    memcpy(payload.data() + 22, columns, qMin<int>(64, static_cast<int>(strlen(columns))));
    return payload;
}

void appendMessage(QByteArray &bytes, uint8_t type, const QByteArray &payload)
{
    bytes.append(static_cast<char>(0xA3));
    bytes.append(static_cast<char>(0x95));
    bytes.append(static_cast<char>(type));
    bytes.append(payload);
}

QByteArray makeParamPayload(const char *name, float value)
{
    QByteArray payload(20, '\0'); // Nf = 16 bytes + 4 bytes
    memcpy(payload.data(), name, qMin<int>(16, static_cast<int>(strlen(name))));
    memcpy(payload.data() + 16, &value, sizeof(value));
    return payload;
}

QByteArray makeModePayload(uint64_t timeUs, uint8_t mode)
{
    QByteArray payload(9, '\0'); // QB = 8 + 1
    memcpy(payload.data(), &timeUs, sizeof(timeUs));
    payload[8] = static_cast<char>(mode);
    return payload;
}

QByteArray makeEventPayload(uint64_t timeUs, uint16_t eventId)
{
    QByteArray payload(10, '\0'); // QH = 8 + 2
    memcpy(payload.data(), &timeUs, sizeof(timeUs));
    memcpy(payload.data() + 8, &eventId, sizeof(eventId));
    return payload;
}

} // namespace

void DataFlashLogParserTest::_parseMinimalLogTest()
{
    QByteArray bytes;
    appendMessage(bytes, 128, makeFmtPayload(150, 23, "PARM", "Nf", "Name,Value"));
    appendMessage(bytes, 128, makeFmtPayload(151, 12, "MODE", "QB", "TimeUS,Mode"));
    appendMessage(bytes, 128, makeFmtPayload(152, 13, "EV", "QH", "TimeUS,Id"));
    appendMessage(bytes, 150, makeParamPayload("LOG_BITMASK", 65535.0f));
    appendMessage(bytes, 151, makeModePayload(5000000ULL, 4));
    appendMessage(bytes, 152, makeEventPayload(6000000ULL, 10));

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QVERIFY(tempFile.write(bytes) == bytes.size());
    tempFile.close();

    DataFlashLogParser parser;
    QVERIFY(parser.parseFile(tempFile.fileName()));
    QVERIFY(parser.parsed());
    QCOMPARE(parser.parseError(), QString());
    QVERIFY(parser.availableSignals().contains(QStringLiteral("PARM.Name")));
    QVERIFY(parser.availableSignals().contains(QStringLiteral("PARM.Value")));
    QCOMPARE(parser.parameters().count(), 1);
    QVERIFY(parser.events().count() >= 2);
}

void DataFlashLogParserTest::_parseInvalidLogTest()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QVERIFY(tempFile.write("invalid", 7) == 7);
    tempFile.close();

    DataFlashLogParser parser;
    QVERIFY(!parser.parseFile(tempFile.fileName()));
    QVERIFY(!parser.parsed());
    QVERIFY(!parser.parseError().isEmpty());
}

UT_REGISTER_TEST(DataFlashLogParserTest, TestLabel::Unit, TestLabel::AnalyzeView)
