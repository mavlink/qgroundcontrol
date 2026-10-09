#include "GPSBaseStationSettingsTest.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "GPSReceiverSession.h"

using namespace std::chrono_literals;

namespace {
using Error = GPSReceiverConfigError;

constexpr double NAN_DOUBLE = std::numeric_limits<double>::quiet_NaN();
constexpr std::pair<const char*, GPSType> SETTINGS_RECEIVERS[] = {{"ublox", GPSType::ublox},
                                                                  {"unicore", GPSType::unicore},
                                                                  {"quectel", GPSType::quectel},
                                                                  {"passive", GPSType::passive}};
constexpr GPSEllipsoidPosition SAVED_FIXED_POSITION{
    .latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 512.0f};

/// The base settings for @a mode, a BaseModeDefinition::Mode value.
GPSBaseStationConfig savedSettings(int mode)
{
    GPSBaseStationConfig base;
    switch (mode) {
        case 1:
            base.mode = GPSBaseStationConfig::Fixed{.position = SAVED_FIXED_POSITION, .accuracyMeters = 1.5f};
            break;
        case 2:
            base.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 321s};
            break;
        default:
            base.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.75, .duration = 195s};
            break;
    }
    return base;
}
}  // namespace

void GPSBaseStationSettingsTest::_configForSettings_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("accepted");
    for (const auto& [name, type] : SETTINGS_RECEIVERS) {
        for (const int mode : {0, 1, 2}) {
            const bool accepted = type == GPSType::passive || mode == 1 ||
                                  (mode == 2 ? type == GPSType::unicore : type != GPSType::unicore);
            QTest::addRow("%s-mode-%d", name, mode) << type << mode << accepted;
        }
    }
}

void GPSBaseStationSettingsTest::_configForSettings()
{
    QFETCH(GPSType, type);
    QFETCH(int, mode);
    QFETCH(bool, accepted);
    QString error;
    const GPSReceiverConfig config = gpsReceiverConfigFor(savedSettings(mode), type, 230400, false, &error);
    QCOMPARE(error.isEmpty(), accepted);
    QCOMPARE(config.baudRate, uint32_t(230400));
    QVERIFY(!config.allowPersistentChanges);
    if (type == GPSType::passive) {
        QCOMPARE(config.base, GPSBaseStationConfig{});
        return;
    }
    const GPSBaseStationConfig::Mode expected =
        mode == 1 ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{.position = SAVED_FIXED_POSITION,
                                                                           .accuracyMeters = 1.5f}}
        : mode == 2
            ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 321s}}
            : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.75, .duration = 195s}};
    QVERIFY(config.base.mode == expected);
}

void GPSBaseStationSettingsTest::_settingsDiagnostics_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<int>("mode");
    QTest::addColumn<uint>("averagingDuration");
    QTest::addColumn<uint>("baud");
    QTest::addColumn<QString>("diagnostic");
    const QString unsupportedMode = gpsReceiverConfigErrorText(Error::UnsupportedBaseMode);
    const QString invalidAveraging = gpsReceiverConfigErrorText(Error::InvalidReceiverAveraging);
    QTest::newRow("unicore-needs-explicit-mode") << GPSType::unicore << 0 << 60U << 115200U << unsupportedMode;
    QTest::newRow("quectel-rejects-averaging") << GPSType::quectel << 2 << 60U << 115200U << unsupportedMode;
    QTest::newRow("zero-averaging") << GPSType::unicore << 2 << 0U << 115200U << invalidAveraging;
    QTest::newRow("excess-averaging") << GPSType::unicore << 2 << 3601U << 115200U << invalidAveraging;
    QTest::newRow("minimum-averaging") << GPSType::unicore << 2 << 1U << 115200U << QString();
    QTest::newRow("maximum-averaging") << GPSType::unicore << 2 << 3600U << 115200U << QString();
    QTest::newRow("passive-needs-baud") << GPSType::passive << 0 << 60U << 0U
                                        << gpsReceiverConfigErrorText(Error::InvalidBaudRate);
    QTest::newRow("passive-ignores-base-settings") << GPSType::passive << 2 << 0U << 115200U << QString();
}

void GPSBaseStationSettingsTest::_settingsDiagnostics()
{
    QFETCH(GPSType, type);
    QFETCH(int, mode);
    QFETCH(uint, averagingDuration);
    QFETCH(uint, baud);
    QFETCH(QString, diagnostic);
    auto settings = savedSettings(mode);
    if (auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&settings.mode)) {
        averaging->maximumDuration = std::chrono::seconds(averagingDuration);
    }
    // A supported request clears an earlier diagnostic.
    QString error = QStringLiteral("earlier diagnostic");
    (void) gpsReceiverConfigFor(settings, type, baud, false, &error);
    QCOMPARE(error, diagnostic);
}

void GPSBaseStationSettingsTest::_compactObservationsFollowSupport()
{
    auto settings = savedSettings(0);
    for (const bool compact : {false, true}) {
        settings.compactObservations = compact;
        QString error;
        QCOMPARE(gpsReceiverConfigFor(settings, GPSType::ublox, 115200, false, &error).base.compactObservations,
                 compact);
        QVERIFY(error.isEmpty());
        // Receivers without MSM4 support ignore the hidden option instead of failing to connect.
        QVERIFY(!gpsReceiverConfigFor(settings, GPSType::septentrio, 115200, false, &error).base.compactObservations);
        QVERIFY(error.isEmpty());
    }
}

void GPSBaseStationSettingsTest::_persistentConsent_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<bool>("allowPersistentChanges");
    for (const auto& [name, type] : SETTINGS_RECEIVERS) {
        for (const bool allow : {false, true}) {
            QTest::addRow("%s-consent-%d", name, allow) << type << allow;
        }
    }
}

void GPSBaseStationSettingsTest::_persistentConsent()
{
    QFETCH(GPSType, type);
    QFETCH(bool, allowPersistentChanges);
    QString error;
    const auto config = gpsReceiverConfigFor(savedSettings(1), type, 115200, allowPersistentChanges, &error);
    QCOMPARE(error.isEmpty(), !allowPersistentChanges || type == GPSType::quectel);
    QCOMPARE(config.allowPersistentChanges, allowPersistentChanges);
}

void GPSBaseStationSettingsTest::_baseStationState()
{
    using Mode = BaseModeDefinition::Mode;
    const auto stateFor = [](int mode, GPSType type) {
        return gpsBaseStationStateFor(type, gpsReceiverConfigFor(savedSettings(mode), type, 0, false, nullptr));
    };
    const auto passive = gpsBaseStationStateFor(GPSType::passive, {.baudRate = 115200});
    QVERIFY(!passive.mode);
    QVERIFY(!passive.position);
    QVERIFY(!passive.surveyAccuracyLimitMeters);

    const auto fixed = stateFor(1, GPSType::ublox);
    QCOMPARE(fixed.mode, std::optional(Mode::BaseFixed));
    QVERIFY(fixed.position);
    QCOMPARE(fixed.position->first, SAVED_FIXED_POSITION);
    QCOMPARE(fixed.position->second, 1.5);
    QVERIFY(!fixed.surveyAccuracyLimitMeters);

    const auto survey = stateFor(0, GPSType::ublox);
    QCOMPARE(survey.mode, std::optional(Mode::BaseSurveyIn));
    QVERIFY(!survey.position);
    QCOMPARE(survey.surveyAccuracyLimitMeters, std::optional(1.75));

    const auto averaging = stateFor(2, GPSType::unicore);
    QCOMPARE(averaging.mode, std::optional(Mode::BaseReceiverAveraging));
    QVERIFY(!averaging.position);
    QVERIFY(!averaging.surveyAccuracyLimitMeters);
}

void GPSBaseStationSettingsTest::_surveyUpdatesBasePosition_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("meanAccuracy");
    QTest::addColumn<double>("expectedAccuracy");
    // A NaN mean accuracy is unreported; a NaN expected accuracy means no position. Mode -1 is a passive receiver.
    QTest::newRow("survey-reported-accuracy") << 0 << true << 11.0 << 1.5 << 1.5;
    QTest::newRow("survey-assumes-limit") << 0 << true << 11.0 << NAN_DOUBLE << 1.75;
    QTest::newRow("survey-invalid") << 0 << false << 11.0 << 1.5 << NAN_DOUBLE;
    QTest::newRow("survey-unlocated") << 0 << true << NAN_DOUBLE << 1.5 << NAN_DOUBLE;
    QTest::newRow("averaging-reported-accuracy") << 2 << true << 11.0 << 0.5 << 0.5;
    QTest::newRow("averaging-without-accuracy") << 2 << true << 11.0 << NAN_DOUBLE << NAN_DOUBLE;
    QTest::newRow("fixed-keeps-saved-position") << 1 << true << 11.0 << 0.5 << 1.5;
    QTest::newRow("passive-has-no-position") << -1 << true << 11.0 << 0.5 << NAN_DOUBLE;
}

void GPSBaseStationSettingsTest::_surveyUpdatesBasePosition()
{
    QFETCH(int, mode);
    QFETCH(bool, valid);
    QFETCH(double, latitude);
    QFETCH(double, meanAccuracy);
    QFETCH(double, expectedAccuracy);
    const GPSType type = mode == 2 ? GPSType::unicore : GPSType::ublox;
    auto state = mode < 0
                     ? gpsBaseStationStateFor(GPSType::passive, {.baudRate = 115200})
                     : gpsBaseStationStateFor(type, gpsReceiverConfigFor(savedSettings(mode), type, 0, false, nullptr));
    GPSSurveyReport report;
    report.valid = true;
    report.position = {.latitudeDegrees = 10.0, .longitudeDegrees = 20.0, .altitudeMeters = 400.0f};
    report.meanAccuracyMeters = 3.0;
    state.applySurvey(report);
    report.valid = valid;
    report.position.latitudeDegrees = latitude;
    report.meanAccuracyMeters = std::isnan(meanAccuracy) ? std::nullopt : std::optional(meanAccuracy);
    state.applySurvey(report);
    QCOMPARE(state.position.has_value(), !std::isnan(expectedAccuracy));
    if (state.position) {
        QCOMPARE(state.position->first, mode == 1 ? SAVED_FIXED_POSITION : report.position);
        QCOMPARE(state.position->second, expectedAccuracy);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSBaseStationSettingsTest, TestLabel::Unit)
