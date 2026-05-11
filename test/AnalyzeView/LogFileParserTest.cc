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

void LogFileParserTest::_fieldSamplesFilteredComprehensiveTest()
{
    // -----------------------------------------------------------------------
    // Dataset: 500 samples, t_i = (i*2000 + 1000) μs  for i = 0..499
    //          → 0.001 s, 0.003 s, ..., 0.999 s  (step = 0.002 s)
    //
    // 10 macro-buckets  b = 0..9, each containing 50 consecutive samples.
    // Within macro-bucket b  (i = b*50 + offset, offset = 0..49):
    //   offset  0  → y =  1000.0 + b   unique "first" per bucket
    //   offset 15  → y = -9999.0       global minimum
    //   offset 30  → y = +9999.0       global maximum
    //   offset 49  → y = -(1000.0 + b) unique "last" per bucket
    //   all other  → y =  0.0
    //
    // With filter range [0.0, 1.0] and pixelWidth=10, col = floor(t*10),
    // so macro-bucket b maps exactly to pixel column b (verified analytically).
    // 500 > 4×10=40 → filtering applies; expected output = 40 points.
    // -----------------------------------------------------------------------
    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            w.messageFormat(ulog_cpp::MessageFormat{
                "sens",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"float",    "val"}}
            });
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 1, "sens"});
            for (int i = 0; i < 500; i++) {
                const uint64_t ts     = static_cast<uint64_t>(i) * 2000ULL + 1000ULL;
                const int      b      = i / 50;   // macro-bucket 0..9
                const int      offset = i % 50;   // position within bucket
                float y = 0.0f;
                if      (offset ==  0) y =  1000.0f + static_cast<float>(b);
                else if (offset == 15) y = -9999.0f;
                else if (offset == 30) y = +9999.0f;
                else if (offset == 49) y = -(1000.0f + static_cast<float>(b));
                w.data(ulog_cpp::Data{1, makePayload64Float(ts, y)});
            }
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QCOMPARE(parser.fieldSamples(QStringLiteral("sens.val")).size(), 500);

    // -----------------------------------------------------------------------
    // Full range [0.0, 1.0], pixelWidth=10
    // 500 > 4×10=40 → filtering; one pixel column per macro-bucket.
    // Each bucket has 4 distinct landmarks → exactly 40 output points.
    // -----------------------------------------------------------------------
    {
        const QVariantList result = parser.fieldSamplesFiltered(
            QStringLiteral("sens.val"), 0.0, 1.0, 10);

        QCOMPARE(result.size(), 40);

        int minCount = 0, maxCount = 0, firstCount = 0, lastCount = 0;
        for (const QVariant &v : result) {
            const double y = v.toPointF().y();
            if (qAbs(y - (-9999.0)) < 0.01) ++minCount;
            if (qAbs(y - (+9999.0)) < 0.01) ++maxCount;
            if (y >= 1000.0 && y <= 1009.0)   ++firstCount;  // 1000+b, b=0..9
            if (y <= -1000.0 && y >= -1009.0)  ++lastCount;  // -(1000+b)
        }
        QCOMPARE(minCount,   10);  // one per bucket
        QCOMPARE(maxCount,   10);
        QCOMPARE(firstCount, 10);
        QCOMPARE(lastCount,  10);

        for (int i = 1; i < result.size(); i++) {
            QVERIFY(result[i].toPointF().x() >= result[i - 1].toPointF().x());
        }
    }

    // -----------------------------------------------------------------------
    // Passthrough: pixelWidth=500 → threshold 4×500=2000 ≥ 500 → no filtering
    // -----------------------------------------------------------------------
    {
        const QVariantList result = parser.fieldSamplesFiltered(
            QStringLiteral("sens.val"), 0.0, 1.0, 500);
        QCOMPARE(result.size(), 500);
    }

    // -----------------------------------------------------------------------
    // Zoom: first half [0.0, 0.5], pixelWidth=10
    // Slice = 250 samples (i=0..249) > 40 → filtering; 10 sub-buckets of 25.
    // Sub-buckets 0,2,4,6,8 contain the min; sub-buckets 1,3,5,7,9 the max.
    // -----------------------------------------------------------------------
    {
        const QVariantList result = parser.fieldSamplesFiltered(
            QStringLiteral("sens.val"), 0.0, 0.5, 10);

        QVERIFY(result.size() > 0);
        QVERIFY(result.size() <= 40);

        int minCount = 0, maxCount = 0;
        for (const QVariant &v : result) {
            const QPointF p = v.toPointF();
            QVERIFY(p.x() >= 0.0 && p.x() <= 0.5);
            if (qAbs(p.y() - (-9999.0)) < 0.01) ++minCount;
            if (qAbs(p.y() - (+9999.0)) < 0.01) ++maxCount;
        }
        QCOMPARE(minCount, 5);
        QCOMPARE(maxCount, 5);

        for (int i = 1; i < result.size(); i++) {
            QVERIFY(result[i].toPointF().x() >= result[i - 1].toPointF().x());
        }
    }

    // -----------------------------------------------------------------------
    // Zoom: single macro-bucket [0.0, 0.1], pixelWidth=10
    // Slice = 50 samples > 40 → filtering; 10 sub-buckets of 5 samples each.
    // Macro-bucket 0 landmarks land in distinct sub-buckets:
    //   offset  0 → col 0  (y =  1000.0  first of b=0)
    //   offset 15 → col 3  (y = -9999.0)
    //   offset 30 → col 6  (y = +9999.0)
    //   offset 49 → col 9  (y = -1000.0  last of b=0)
    // -----------------------------------------------------------------------
    {
        const QVariantList result = parser.fieldSamplesFiltered(
            QStringLiteral("sens.val"), 0.0, 0.1, 10);

        QVERIFY(result.size() > 0);
        QVERIFY(result.size() <= 40);

        bool hasFirst = false, hasMin = false, hasMax = false, hasLast = false;
        for (const QVariant &v : result) {
            const QPointF p = v.toPointF();
            QVERIFY(p.x() >= 0.0 && p.x() <= 0.1);
            if (qAbs(p.y() -   1000.0) < 0.01) hasFirst = true;
            if (qAbs(p.y() - (-9999.0)) < 0.01) hasMin   = true;
            if (qAbs(p.y() -   9999.0) < 0.01) hasMax   = true;
            if (qAbs(p.y() - (-1000.0)) < 0.01) hasLast  = true;
        }
        QVERIFY(hasFirst);
        QVERIFY(hasMin);
        QVERIFY(hasMax);
        QVERIFY(hasLast);

        for (int i = 1; i < result.size(); i++) {
            QVERIFY(result[i].toPointF().x() >= result[i - 1].toPointF().x());
        }
    }

    // -----------------------------------------------------------------------
    // Out-of-range: no samples in [5.0, 6.0] → empty result
    // -----------------------------------------------------------------------
    {
        const QVariantList result = parser.fieldSamplesFiltered(
            QStringLiteral("sens.val"), 5.0, 6.0, 100);
        QCOMPARE(result.size(), 0);
    }
}

// ============================================================================
// gpsPath() tests
// ============================================================================

namespace {

// Build a ULog payload: uint64_t timestamp + two doubles (lat, lon).
std::vector<uint8_t> makePayload64DoubleDouble(uint64_t ts, double lat, double lon)
{
    std::vector<uint8_t> buf(24);
    memcpy(buf.data(),      &ts,  8);
    memcpy(buf.data() +  8, &lat, 8);
    memcpy(buf.data() + 16, &lon, 8);
    return buf;
}

// Build a DataFlash payload for a POS message: Q (uint64) + L (int32) + L (int32).
// 'L' format = int32 stored as degrees * 1e7.
QByteArray makePOSPayload(uint64_t timeUs, double latDeg, double lonDeg)
{
    QByteArray payload(16, '\0');
    memcpy(payload.data(), &timeUs, 8);
    const int32_t latRaw = static_cast<int32_t>(latDeg * 1.0e7);
    const int32_t lonRaw = static_cast<int32_t>(lonDeg * 1.0e7);
    memcpy(payload.data() + 8,  &latRaw, 4);
    memcpy(payload.data() + 12, &lonRaw, 4);
    return payload;
}

void appendBinMessage(QByteArray &bytes, uint8_t type, const QByteArray &payload)
{
    bytes.append(static_cast<char>(0xA3));
    bytes.append(static_cast<char>(0x95));
    bytes.append(static_cast<char>(type));
    bytes.append(payload);
}

QByteArray makeFmtPayloadStr(uint8_t type, uint8_t length, const char *name,
                              const char *format, const char *columns)
{
    QByteArray p(86, '\0');
    p[0] = static_cast<char>(type);
    p[1] = static_cast<char>(length);
    memcpy(p.data() + 2,  name,    qMin<int>(4,  static_cast<int>(strlen(name))));
    memcpy(p.data() + 6,  format,  qMin<int>(16, static_cast<int>(strlen(format))));
    memcpy(p.data() + 22, columns, qMin<int>(64, static_cast<int>(strlen(columns))));
    return p;
}

} // anonymous namespace

void LogFileParserTest::_gpsPathULogVehicleGlobalPositionTest()
{
    // Build a ULog with vehicle_global_position containing lat/lon as double (degrees).
    // Three samples at known coordinates.
    struct Sample { double lat; double lon; };
    static const Sample samples[] = {
        { 47.397742, 8.545594 },
        { 47.397800, 8.545700 },
        { 47.397900, 8.545800 },
    };

    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            w.messageFormat(ulog_cpp::MessageFormat{
                "vehicle_global_position",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"double",   "lat"},
                 ulog_cpp::Field{"double",   "lon"}}
            });
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 1, "vehicle_global_position"});
            uint64_t ts = 500000ULL;
            for (const auto &s : samples) {
                w.data(ulog_cpp::Data{1, makePayload64DoubleDouble(ts, s.lat, s.lon)});
                ts += 500000ULL;
            }
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QVERIFY(parser.parsed());

    const QVariantList path = parser.gpsPath();
    QCOMPARE(path.size(), static_cast<int>(std::size(samples)));

    for (int i = 0; i < path.size(); i++) {
        const QVariantMap coord = path[i].toMap();
        QVERIFY(qAbs(coord.value(QStringLiteral("latitude")).toDouble()  - samples[i].lat) < 1e-6);
        QVERIFY(qAbs(coord.value(QStringLiteral("longitude")).toDouble() - samples[i].lon) < 1e-6);
    }
}

void LogFileParserTest::_gpsPathULogVehicleGpsPositionLatDegTest()
{
    // Simulates newer PX4 firmware that logs vehicle_gps_position with
    // latitude_deg/longitude_deg (double, degrees) instead of lat/lon.
    // vehicle_global_position is intentionally absent to confirm fallback.
    struct Sample { double lat; double lon; };
    static const Sample samples[] = {
        { 47.397742, 8.545594 },
        { 47.397800, 8.545700 },
        { 47.397900, 8.545800 },
    };

    const QByteArray bytes = buildULog(
        [](ulog_cpp::Writer &w) {
            w.messageFormat(ulog_cpp::MessageFormat{
                "vehicle_gps_position",
                {ulog_cpp::Field{"uint64_t", "timestamp"},
                 ulog_cpp::Field{"double",   "latitude_deg"},
                 ulog_cpp::Field{"double",   "longitude_deg"}}
            });
        },
        [](ulog_cpp::Writer &w) {
            w.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, 1, "vehicle_gps_position"});
            uint64_t ts = 500000ULL;
            for (const auto &s : samples) {
                w.data(ulog_cpp::Data{1, makePayload64DoubleDouble(ts, s.lat, s.lon)});
                ts += 500000ULL;
            }
        });

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.ulg"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QVERIFY(parser.parsed());

    const QVariantList path = parser.gpsPath();
    QCOMPARE(path.size(), static_cast<int>(std::size(samples)));

    for (int i = 0; i < path.size(); i++) {
        const QVariantMap coord = path[i].toMap();
        QVERIFY(qAbs(coord.value(QStringLiteral("latitude")).toDouble()  - samples[i].lat) < 1e-6);
        QVERIFY(qAbs(coord.value(QStringLiteral("longitude")).toDouble() - samples[i].lon) < 1e-6);
    }
}

void LogFileParserTest::_gpsPathAPMDataFlashPOSTest()
{
    // Build a minimal DataFlash .bin with a POS message (format QLL).
    // 'L' type: int32 stored, value = raw / 1e7 (degrees).
    struct Sample { double lat; double lon; };
    static const Sample samples[] = {
        { -35.363261, 149.165230 },
        { -35.363100, 149.165400 },
        { -35.362900, 149.165600 },
    };

    QByteArray bytes;
    // FMT for POS: type=155, length= 3(hdr)+8(Q)+4(L)+4(L)=19, name="POS", format="QLL"
    appendBinMessage(bytes, 128, makeFmtPayloadStr(155, 19, "POS", "QLL", "TimeUS,Lat,Lng"));

    uint64_t ts = 1000000ULL;
    for (const auto &s : samples) {
        appendBinMessage(bytes, 155, makePOSPayload(ts, s.lat, s.lon));
        ts += 1000000ULL;
    }

    QTemporaryFile tmp;
    tmp.setFileTemplate(QDir::tempPath() + QStringLiteral("/logtest_XXXXXX.bin"));
    QVERIFY(writeTempFile(tmp, bytes));

    LogFileParser parser;
    QVERIFY(parser.parseFile(tmp.fileName()));
    QVERIFY(parser.parsed());

    const QVariantList path = parser.gpsPath();
    QCOMPARE(path.size(), static_cast<int>(std::size(samples)));

    for (int i = 0; i < path.size(); i++) {
        const QVariantMap coord = path[i].toMap();
        // 'L' precision is 1e-7 degrees; allow a small rounding tolerance
        QVERIFY(qAbs(coord.value(QStringLiteral("latitude")).toDouble()  - samples[i].lat) < 1e-6);
        QVERIFY(qAbs(coord.value(QStringLiteral("longitude")).toDouble() - samples[i].lon) < 1e-6);
    }
}

UT_REGISTER_TEST(LogFileParserTest, TestLabel::Unit, TestLabel::AnalyzeView)
