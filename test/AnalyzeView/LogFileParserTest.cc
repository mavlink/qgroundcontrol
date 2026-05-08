#include "LogFileParserTest.h"

#include "LogFileParser.h"

#include <cstring>
#include <functional>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QPointF>
#include <QtCore/QTemporaryFile>
#include <QtCore/QVariantList>

#include <ulog_cpp/messages.hpp>
#include <ulog_cpp/writer.hpp>

// ============================================================================
// In-memory ULog builder helpers
// ============================================================================

namespace {

// Build a minimal but complete ULog byte stream via ulog_cpp::Writer.
// Callers add formats, subscriptions, and data messages via the callbacks.
using WriterFn = std::function<void(ulog_cpp::Writer &)>;

QByteArray buildULog(WriterFn headerFn, WriterFn dataFn)
{
    std::vector<uint8_t> buffer;
    ulog_cpp::Writer writer([&](const uint8_t *data, int length) {
        buffer.insert(buffer.end(), data, data + length);
    });

    writer.fileHeader(ulog_cpp::FileHeader{});
    if (headerFn) {
        headerFn(writer);
    }
    writer.headerComplete();
    if (dataFn) {
        dataFn(writer);
    }

    return QByteArray(reinterpret_cast<const char *>(buffer.data()), static_cast<int>(buffer.size()));
}

// Write bytes to a QTemporaryFile with a given suffix and leave it closed.
// Returns false if writing failed.
bool writeTempFile(QTemporaryFile &tmp, const QByteArray &bytes)
{
    if (!tmp.open()) {
        return false;
    }
    const bool ok = (tmp.write(bytes) == bytes.size());
    tmp.close();
    return ok;
}

std::vector<uint8_t> makePayload64Float(uint64_t ts, float value)
{
    std::vector<uint8_t> buf(12);
    memcpy(buf.data(),     &ts,    8);
    memcpy(buf.data() + 8, &value, 4);
    return buf;
}

std::vector<uint8_t> makePayload64Int32(uint64_t ts, int32_t value)
{
    std::vector<uint8_t> buf(12);
    memcpy(buf.data(),     &ts,    8);
    memcpy(buf.data() + 8, &value, 4);
    return buf;
}

} // anonymous namespace

// ============================================================================
// Tests
// ============================================================================

void LogFileParserTest::_parseULogNumericTopicTest()
{
    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            w.messageFormat(ulog_cpp::MessageFormat{
                "sensor_combined",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"float", "gyro_rad_x"}}
            });
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 1, "sensor_combined"});
            // Two samples
            w.data(ulog_cpp::Data{1, makePayload64Float(500000ULL, 0.1f)});
            w.data(ulog_cpp::Data{1, makePayload64Float(1000000ULL, 0.2f)});
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QVERIFY(parser.parsed());
    QCOMPARE(parser.parseError(), QString());

    // Field should appear in both available and plottable lists
    QVERIFY(parser.availableFields().contains(QStringLiteral("sensor_combined.gyro_rad_x")));
    QVERIFY(parser.plottableFields().contains(QStringLiteral("sensor_combined.gyro_rad_x")));

    // timestamp pseudo-field must NOT be plottable
    QVERIFY(!parser.plottableFields().contains(QStringLiteral("sensor_combined.timestamp")));

    // fieldSamples returns two points
    const QVariantList samples = parser.fieldSamples(QStringLiteral("sensor_combined.gyro_rad_x"));
    QCOMPARE(samples.size(), 2);
    QVERIFY(qAbs(samples[0].toPointF().x() - 0.5) < 1e-5);
    QVERIFY(qAbs(samples[0].toPointF().y() - 0.1) < 1e-5);
    QVERIFY(qAbs(samples[1].toPointF().x() - 1.0) < 1e-5);
    QVERIFY(qAbs(samples[1].toPointF().y() - 0.2) < 1e-5);
}

void LogFileParserTest::_parseULogParameterTest()
{
    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            // Include a format so the file is fully valid
            w.messageFormat(ulog_cpp::MessageFormat{
                "sensor_combined",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"float", "gyro_rad_x"}}
            });
            // Parameter in header section — name only, type inferred from value type
            w.parameter(ulog_cpp::Parameter{"MY_PARAM", 42.0f});
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 1, "sensor_combined"});
            w.data(ulog_cpp::Data{1, makePayload64Float(500000ULL, 0.0f)});
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QVERIFY(parser.parsed());

    QCOMPARE(parser.parameters().count(), 1);
    const QVariantMap param = parser.parameters().first().toMap();
    QCOMPARE(param.value(QStringLiteral("name")).toString(), QStringLiteral("MY_PARAM"));
    QVERIFY(qAbs(param.value(QStringLiteral("value")).toDouble() - 42.0) < 1e-5);
}

void LogFileParserTest::_parseULogWarningEventTest()
{
    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &) {},
        [](ulog_cpp::Writer &w) {
            // Warning-level message → should appear in both messages AND events
            w.logging(ulog_cpp::Logging{
                ulog_cpp::Logging::Level::Warning,
                "Low battery warning",
                500000ULL
            });
            // Debug-level message → messages only, NOT events
            w.logging(ulog_cpp::Logging{
                ulog_cpp::Logging::Level::Debug,
                "Debug info",
                1000000ULL
            });
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));

    QCOMPARE(parser.messages().count(), 2);
    QCOMPARE(parser.events().count(), 1);

    const QVariantMap ev = parser.events().first().toMap();
    QCOMPARE(ev.value(QStringLiteral("type")).toString(), QStringLiteral("warning"));
    QVERIFY(ev.value(QStringLiteral("description")).toString().contains(QStringLiteral("Low battery")));
}

void LogFileParserTest::_parseULogModeSegmentsTest()
{
    // Two nav_state changes: Manual(0) at t=0.5s, then Mission(3) at t=1.0s
    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            w.messageFormat(ulog_cpp::MessageFormat{
                "vehicle_status",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"int32_t", "nav_state"}}
            });
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 2, "vehicle_status"});
            w.data(ulog_cpp::Data{2, makePayload64Int32(500000ULL,  0)});  // Manual
            w.data(ulog_cpp::Data{2, makePayload64Int32(1000000ULL, 3)});  // Mission
            w.data(ulog_cpp::Data{2, makePayload64Int32(2000000ULL, 3)});  // Mission (continues)
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));

    // Should have two mode segments: Manual and Mission
    QVERIFY(parser.modeSegments().count() >= 2);

    const QStringList modes = [&]() {
        QStringList list;
        for (const QVariant &seg : parser.modeSegments()) {
            list << seg.toMap().value(QStringLiteral("mode")).toString();
        }
        return list;
    }();

    QVERIFY(modes.contains(QStringLiteral("Manual")));
    QVERIFY(modes.contains(QStringLiteral("Mission")));
}

void LogFileParserTest::_parseULogDropoutTest()
{
    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            w.messageFormat(ulog_cpp::MessageFormat{
                "sensor_combined",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"float", "gyro_rad_x"}}
            });
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 1, "sensor_combined"});
            w.data(ulog_cpp::Data{1, makePayload64Float(500000ULL, 1.0f)});
            // 100 ms dropout after the data message
            w.dropout(ulog_cpp::Dropout{100});
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));

    QCOMPARE(parser.dropouts().count(), 1);

    const QVariantMap dropout = parser.dropouts().first().toMap();
    // start should be around 0.5 s (the timestamp of the preceding data message)
    const double start = dropout.value(QStringLiteral("start")).toDouble();
    const double end   = dropout.value(QStringLiteral("end")).toDouble();
    QVERIFY(start >= 0.0);
    QVERIFY(end > start);
    // Duration should be approximately 0.1 s
    QVERIFY(qAbs((end - start) - 0.1) < 0.01);
}

void LogFileParserTest::_parseULogInvalidFileTest()
{
    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(tmp.open());
    tmp.write("this is not a valid ulg file", 28);
    tmp.close();

    LogFileParser parser;
    QVERIFY(!parser.parseFile(tmp.fileName()));
    QVERIFY(!parser.parsed());
    QVERIFY(!parser.parseError().isEmpty());
}

void LogFileParserTest::_parseDataFlashRegressionTest()
{
    // Build a minimal DataFlash binary log to verify the .bin parse path
    // still works via LogFileParser (regression guard for APMDataFlashLogParser).
    auto makeFmt = [](uint8_t type, uint8_t len, const char *name,
                      const char *fmt, const char *cols) -> QByteArray {
        QByteArray p(86, '\0');
        p[0] = static_cast<char>(type);
        p[1] = static_cast<char>(len);
        memcpy(p.data() + 2,  name, qMin<int>(4,  static_cast<int>(strlen(name))));
        memcpy(p.data() + 6,  fmt,  qMin<int>(16, static_cast<int>(strlen(fmt))));
        memcpy(p.data() + 22, cols, qMin<int>(64, static_cast<int>(strlen(cols))));
        return p;
    };
    auto appendMsg = [](QByteArray &b, uint8_t type, const QByteArray &payload) {
        b.append(static_cast<char>(0xA3));
        b.append(static_cast<char>(0x95));
        b.append(static_cast<char>(type));
        b.append(payload);
    };
    auto makeParam = [](const char *name, float value) -> QByteArray {
        QByteArray p(20, '\0');
        memcpy(p.data(), name, qMin<int>(16, static_cast<int>(strlen(name))));
        memcpy(p.data() + 16, &value, sizeof(value));
        return p;
    };

    QByteArray bytes;
    appendMsg(bytes, 128, makeFmt(150, 23, "PARM", "Nf", "Name,Value"));
    appendMsg(bytes, 150, makeParam("ARMING_CHECK", 1.0f));

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.bin"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QVERIFY(parser.parsed());
    QCOMPARE(parser.parameters().count(), 1);
}

void LogFileParserTest::_parseUnsupportedExtensionTest()
{
    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.txt"));
    QVERIFY(tmp.open());
    tmp.write("dummy", 5);
    tmp.close();

    LogFileParser parser;
    QVERIFY(!parser.parseFile(tmp.fileName()));
    QVERIFY(!parser.parsed());
    QVERIFY(!parser.parseError().isEmpty());
}

UT_REGISTER_TEST(LogFileParserTest, TestLabel::Unit, TestLabel::AnalyzeView)
