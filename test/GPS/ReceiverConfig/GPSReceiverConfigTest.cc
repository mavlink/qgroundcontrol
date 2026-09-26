#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>

#include "GPSBaseStationSettings.h"
#include "GPSEllipsoidPosition.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverReports.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {
using Error = GPSReceiverConfigError;
using Role = GPSReceiverConfig::Role;

constexpr double NAN_DOUBLE = std::numeric_limits<double>::quiet_NaN();
constexpr double INF_DOUBLE = std::numeric_limits<double>::infinity();
constexpr float NAN_FLOAT = std::numeric_limits<float>::quiet_NaN();
constexpr float INF_FLOAT = std::numeric_limits<float>::infinity();
constexpr int64_t MAX_DURATION = (std::numeric_limits<uint32_t>::max)();
constexpr double MAX_SURVEY_ACCURACY = static_cast<double>(MAX_DURATION) / 10000.0;
constexpr std::pair<const char*, GPSType> MANAGED_RECEIVERS[] = {
    {"ublox", GPSType::ublox}, {"trimble", GPSType::trimble}, {"septentrio", GPSType::septentrio},
    {"femto", GPSType::femto}, {"unicore", GPSType::unicore}, {"quectel", GPSType::quectel},
};
constexpr GPSBaseStationConfig VALID_SURVEY{
    .mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 0.0001, .duration = 1s}};
constexpr GPSBaseStationConfig VALID_FIXED{
    .mode = GPSBaseStationConfig::Fixed{
        .position = {.latitudeDegrees = 0.0, .longitudeDegrees = 0.0, .altitudeMeters = 0.0f}}};
constexpr std::pair<const char*, GPSType> SETTINGS_RECEIVERS[] = {{"ublox", GPSType::ublox},
                                                                  {"unicore", GPSType::unicore},
                                                                  {"quectel", GPSType::quectel},
                                                                  {"passive", GPSType::passive}};
constexpr GPSEllipsoidPosition SAVED_FIXED_POSITION{
    .latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 512.0f};

/// Saved settings for @a mode, a GPSBaseStationSettings::Mode value, with valid values for every mode.
GPSBaseStationSettings savedSettings(int mode)
{
    return {.mode = static_cast<GPSBaseStationSettings::Mode>(mode),
            .fixedPosition = SAVED_FIXED_POSITION,
            .fixedAccuracyMeters = 1.5f,
            .surveyInAccuracyMeters = 1.75,
            .surveyInMinimumDuration = 195s,
            .averagingMaximumDuration = 321s};
}
}  // namespace

static_assert(std::is_aggregate_v<GPSReceiverConfig>);
static_assert(std::is_copy_constructible_v<GPSPositionReport>);
static_assert(std::is_copy_constructible_v<GPSSatelliteReport>);
static_assert(std::is_copy_constructible_v<GPSSurveyReport>);
static_assert(std::is_same_v<decltype(GPSEllipsoidPosition::latitudeDegrees), double>);
static_assert(std::is_same_v<decltype(GPSEllipsoidPosition::longitudeDegrees), double>);
static_assert(std::is_same_v<decltype(GPSEllipsoidPosition::altitudeMeters), float>);

class GPSReceiverConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _reportDefaults();
    void _reportSnapshots();
    void _baseValidation_data();
    void _baseValidation();
    void _surveyWireRange();
    void _fixedWireRepresentability();
    void _capabilities_data();
    void _capabilities();
    void _receiverValidation_data();
    void _receiverValidation();
    void _baseModesExclusive();
    void _validationPrecedence_data();
    void _validationPrecedence();
    void _newReceivers_data();
    void _newReceivers();
    void _newCapabilities();
    void _descriptorIdentities();
    void _presentation_data();
    void _presentation();
    void _configForSettings_data();
    void _configForSettings();
    void _settingsDiagnostics_data();
    void _settingsDiagnostics();
    void _compactObservationsFollowSupport();
    void _persistentConsent_data();
    void _persistentConsent();
    void _baseStationState();
    void _surveyUpdatesBasePosition_data();
    void _surveyUpdatesBasePosition();
    void _detectedReceiverFit_data();
    void _detectedReceiverFit();
};

void GPSReceiverConfigTest::_defaults()
{
    const GPSReceiverConfig config;
    QCOMPARE(config.role, Role::RTKBase);
    QVERIFY(!config.allowPersistentChanges);
    QVERIFY(std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode));
    QCOMPARE(std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters, 0.0);
    QCOMPARE(std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration, std::chrono::seconds{0});
    const GPSBaseStationConfig::Fixed fixed;
    QVERIFY(std::isnan(fixed.position.latitudeDegrees));
    QVERIFY(std::isnan(fixed.position.longitudeDegrees));
    QVERIFY(std::isnan(fixed.position.altitudeMeters));
    QCOMPARE(fixed.accuracyMeters, 0.0f);

    const GPSReceiverCapabilities capabilities;
    QVERIFY(!capabilities.recognized);
    QVERIFY(!capabilities.rtkBase);
    QVERIFY(!capabilities.surveyIn);
    QVERIFY(!capabilities.receiverAveraging);
    QVERIFY(!capabilities.passive);
    QVERIFY(!capabilities.persistentConfiguration);
}

void GPSReceiverConfigTest::_reportDefaults()
{
    const GPSPositionReport position;
    QCOMPARE(position.navigation.fixType, GPSPositionReport::FixType::Unknown);
    QVERIFY(!position.navigation.satellitesUsed);
    QVERIFY(std::isnan(position.navigation.latitudeDegrees));
    QVERIFY(std::isnan(position.navigation.longitudeDegrees));
    QVERIFY(std::isnan(position.navigation.altitudeMslMeters));
    QVERIFY(std::isnan(position.navigation.altitudeEllipsoidMeters));
    const auto& integrity = position.integrity;
    QCOMPARE(integrity.timestampUs, uint64_t{0});
    QCOMPARE(integrity.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Unknown);
    QCOMPARE(integrity.corrections.use, GPSIntegrityReport::CorrectionUse::Unknown);
    QVERIFY(!integrity.rf.noisePerMillisecond);
    QVERIFY(!integrity.rf.automaticGainControl);
    QVERIFY(!integrity.rf.jammingIndicator);
    QVERIFY(!integrity.corrections.crcFailed);

    const GPSSatelliteReport satellites;
    QVERIFY(!satellites.inView);
    QVERIFY(!satellites.used);

    const GPSSurveyReport survey;
    QVERIFY(std::isnan(survey.position.latitudeDegrees));
    QVERIFY(std::isnan(survey.position.longitudeDegrees));
    QVERIFY(std::isnan(survey.position.altitudeMeters));
    QVERIFY(!survey.meanAccuracyMeters);
    QVERIFY(!survey.valid);
    QVERIFY(!survey.active);
}

void GPSReceiverConfigTest::_reportSnapshots()
{
    GPSPositionReport position;
    position.integrity.corrections.crcFailed = false;
    position.integrity.rf.noisePerMillisecond = 0;
    const auto snapshot = position;
    position.integrity.rf.noisePerMillisecond = 42;
    QCOMPARE(snapshot.integrity.rf.noisePerMillisecond, std::optional<int32_t>{0});
    QCOMPARE(snapshot.integrity.corrections.crcFailed, std::optional<bool>{false});

    const auto transported = QVariant::fromValue(snapshot).value<GPSPositionReport>();
    QCOMPARE(transported.integrity.rf.noisePerMillisecond, std::optional<int32_t>{0});
    QCOMPARE(transported.integrity.corrections.crcFailed, std::optional<bool>{false});
}

void GPSReceiverConfigTest::_baseValidation_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::addColumn<Error>("expected");

    const auto survey = [](const char* name, double accuracy, int64_t duration, Error expected) {
        QTest::newRow(name) << GPSBaseStationConfig{.mode =
                                                        GPSBaseStationConfig::SurveyIn{
                                                            .accuracyMeters = accuracy,
                                                            .duration = std::chrono::seconds(duration)}}
                            << expected;
    };
    QTest::newRow("missing-survey") << GPSBaseStationConfig{} << Error::InvalidSurveyIn;
    survey("survey-minimum-ignores-unused-fixed-position", 0.0001, 1, Error::None);
    survey("survey-maximum", MAX_SURVEY_ACCURACY, MAX_DURATION, Error::None);
    survey("survey-accuracy-negative", -1.0, 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-zero", 0.0, 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-below-one-wire-unit", std::nextafter(0.0001, 0.0), 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-above-maximum", std::nextafter(MAX_SURVEY_ACCURACY, INF_DOUBLE), 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-nan", NAN_DOUBLE, 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-positive-infinity", INF_DOUBLE, 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-negative-infinity", -INF_DOUBLE, 1, Error::InvalidSurveyIn);
    survey("survey-accuracy-conversion-overflow", (std::numeric_limits<double>::max)(), 1, Error::InvalidSurveyIn);
    survey("survey-duration-negative", 0.0001, -1, Error::InvalidSurveyIn);
    survey("survey-duration-zero", 0.0001, 0, Error::InvalidSurveyIn);
    survey("survey-duration-above-uint32-maximum", 0.0001, MAX_DURATION + 1, Error::InvalidSurveyIn);
    survey("survey-duration-int64-minimum", 0.0001, (std::numeric_limits<int64_t>::min)(), Error::InvalidSurveyIn);
    survey("survey-duration-int64-maximum", 0.0001, (std::numeric_limits<int64_t>::max)(), Error::InvalidSurveyIn);

    const auto fixed = [](const char* name, double latitude, double longitude, float altitude, float accuracy,
                          Error expected) {
        QTest::newRow(name) << GPSBaseStationConfig{.mode =
                                                        GPSBaseStationConfig::Fixed{
                                                            .position = {.latitudeDegrees = latitude,
                                                                         .longitudeDegrees = longitude,
                                                                         .altitudeMeters = altitude},
                                                            .accuracyMeters = accuracy}}
                            << expected;
    };
    QTest::newRow("missing-fixed-position")
        << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{}} << Error::InvalidFixedBase;
    fixed("fixed-explicit-zero-ignores-unused-survey", 0, 0, 0, 0, Error::None);
    fixed("fixed-unknown-accuracy", 47, 8, 500, 0, Error::None);
    fixed("fixed-negative-coordinate-limits", -90, -180, 0, 0, Error::None);
    fixed("fixed-positive-coordinate-limits", 90, 180, 0, 0, Error::None);
    fixed("latitude-below-minimum", std::nextafter(-90.0, -INF_DOUBLE), 0, 0, 0, Error::InvalidFixedBase);
    fixed("latitude-above-maximum", std::nextafter(90.0, INF_DOUBLE), 0, 0, 0, Error::InvalidFixedBase);
    fixed("latitude-nan", NAN_DOUBLE, 0, 0, 0, Error::InvalidFixedBase);
    fixed("latitude-positive-infinity", INF_DOUBLE, 0, 0, 0, Error::InvalidFixedBase);
    fixed("latitude-negative-infinity", -INF_DOUBLE, 0, 0, 0, Error::InvalidFixedBase);
    fixed("longitude-below-minimum", 0, std::nextafter(-180.0, -INF_DOUBLE), 0, 0, Error::InvalidFixedBase);
    fixed("longitude-above-maximum", 0, std::nextafter(180.0, INF_DOUBLE), 0, 0, Error::InvalidFixedBase);
    fixed("longitude-nan", 0, NAN_DOUBLE, 0, 0, Error::InvalidFixedBase);
    fixed("longitude-positive-infinity", 0, INF_DOUBLE, 0, 0, Error::InvalidFixedBase);
    fixed("longitude-negative-infinity", 0, -INF_DOUBLE, 0, 0, Error::InvalidFixedBase);
    fixed("altitude-minimum-wire-safe", 0, 0, -21474836.0f, 0, Error::None);
    fixed("altitude-maximum-wire-safe", 0, 0, 21474836.0f, 0, Error::None);
    fixed("altitude-below-minimum", 0, 0, std::nextafter(-21474836.0f, -INF_FLOAT), 0, Error::InvalidFixedBase);
    fixed("altitude-above-maximum", 0, 0, std::nextafter(21474836.0f, INF_FLOAT), 0, Error::InvalidFixedBase);
    fixed("altitude-nan", 0, 0, NAN_FLOAT, 0, Error::InvalidFixedBase);
    fixed("altitude-positive-infinity", 0, 0, INF_FLOAT, 0, Error::InvalidFixedBase);
    fixed("altitude-negative-infinity", 0, 0, -INF_FLOAT, 0, Error::InvalidFixedBase);
    fixed("fixed-accuracy-negative", 0, 0, 0, -0.0001f, Error::InvalidFixedBase);
    fixed("fixed-accuracy-negative-zero", 0, 0, 0, -0.0f, Error::None);
    fixed("fixed-accuracy-below-one-wire-unit", 0, 0, 0, 0.00001f, Error::None);
    fixed("fixed-accuracy-two-stage-float-limit", 0, 0, 0, 429496.71875f, Error::None);
    fixed("fixed-accuracy-above-maximum", 0, 0, 0, std::nextafter(429496.71875f, INF_FLOAT), Error::InvalidFixedBase);
    fixed("fixed-accuracy-nan", 0, 0, 0, NAN_FLOAT, Error::InvalidFixedBase);
    fixed("fixed-accuracy-positive-infinity", 0, 0, 0, INF_FLOAT, Error::InvalidFixedBase);
    fixed("fixed-accuracy-negative-infinity", 0, 0, 0, -INF_FLOAT, Error::InvalidFixedBase);
    fixed("fixed-accuracy-conversion-overflow", 0, 0, 0, (std::numeric_limits<float>::max)(), Error::InvalidFixedBase);
}

void GPSReceiverConfigTest::_baseValidation()
{
    QFETCH(GPSBaseStationConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateBaseStationConfig(config), expected);
}

void GPSReceiverConfigTest::_surveyWireRange()
{
    const GPSBaseStationConfig config{
        .mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = MAX_SURVEY_ACCURACY,
                                               .duration = std::chrono::seconds(MAX_DURATION)}};
    QCOMPARE(gpsValidateBaseStationConfig(config), Error::None);
    QCOMPARE(static_cast<uint32_t>(std::get<GPSBaseStationConfig::SurveyIn>(config.mode).accuracyMeters * 10000.0),
             (std::numeric_limits<uint32_t>::max)());
    QCOMPARE(static_cast<uint32_t>(std::get<GPSBaseStationConfig::SurveyIn>(config.mode).duration.count()),
             (std::numeric_limits<uint32_t>::max)());
}

void GPSReceiverConfigTest::_fixedWireRepresentability()
{
    const GPSBaseStationConfig config{
        .mode = GPSBaseStationConfig::Fixed{
            .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 21474836.0f},
            .accuracyMeters = 429496.71875f}};
    QCOMPARE(gpsValidateBaseStationConfig(config), Error::None);
    QCOMPARE(
        static_cast<int32_t>(
            static_cast<double>(std::get<GPSBaseStationConfig::Fixed>(config.mode).position.altitudeMeters) * 100.0),
        2147483600);
    const float accuracyMillimeters = std::get<GPSBaseStationConfig::Fixed>(config.mode).accuracyMeters * 1000.0f;
    QCOMPARE(static_cast<uint32_t>(accuracyMillimeters * 10.0f), 4294967040u);
}

void GPSReceiverConfigTest::_capabilities_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<Role>("role");
    QTest::addColumn<GPSReceiverCapabilities>("expected");
    for (const auto& [typeName, type] : MANAGED_RECEIVERS) {
        QTest::newRow(qPrintable(QStringLiteral("%1-base").arg(typeName)))
            << type << Role::RTKBase << gpsReceiverDescriptor(type)->capabilities;
        QTest::newRow(qPrintable(QStringLiteral("%1-passive").arg(typeName)))
            << type << Role::Passive << gpsReceiverDescriptor(type)->capabilities;
    }
    // Automatic accepts every option some base family supports, until detection names the family.
    const GPSReceiverCapabilities anyBase{.recognized = true,
                                          .rtkBase = true,
                                          .surveyIn = true,
                                          .receiverAveraging = true,
                                          .persistentConfiguration = true,
                                          .compactObservations = true};
    QTest::newRow("automatic-base") << GPSType::automatic << Role::RTKBase << anyBase;
    QTest::newRow("automatic-passive") << GPSType::automatic << Role::Passive << anyBase;
    QTest::newRow("passive-base") << GPSType::passive << Role::RTKBase
                                  << gpsReceiverDescriptor(GPSType::passive)->capabilities;
    QTest::newRow("passive-passive") << GPSType::passive << Role::Passive
                                     << gpsReceiverDescriptor(GPSType::passive)->capabilities;
    for (int value : {-1, 8, 255}) {
        for (const auto role : {Role::RTKBase, Role::Passive}) {
            QTest::newRow(qPrintable(QStringLiteral("unknown-type-%1-%2").arg(value).arg(static_cast<int>(role))))
                << static_cast<GPSType>(value) << role << GPSReceiverCapabilities{};
        }
    }
    for (int value : {-1, 2, 255}) {
        for (const auto& [typeName, type] : MANAGED_RECEIVERS) {
            QTest::newRow(qPrintable(QStringLiteral("%1-unknown-role-%2").arg(typeName).arg(value)))
                << type << static_cast<Role>(value) << GPSReceiverCapabilities{};
        }
    }
}

void GPSReceiverConfigTest::_capabilities()
{
    QFETCH(GPSType, type);
    QFETCH(Role, role);
    QFETCH(GPSReceiverCapabilities, expected);
    const auto actual = gpsReceiverCapabilities(type, role);
    QCOMPARE(actual.recognized, expected.recognized);
    QCOMPARE(actual.rtkBase, expected.rtkBase);
    QCOMPARE(actual.surveyIn, expected.surveyIn);
    QCOMPARE(actual.receiverAveraging, expected.receiverAveraging);
    QCOMPARE(actual.passive, expected.passive);
    QCOMPARE(actual.persistentConfiguration, expected.persistentConfiguration);
    QCOMPARE(actual.compactObservations, expected.compactObservations);
}

void GPSReceiverConfigTest::_receiverValidation_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");
    for (const auto& [typeName, type] : MANAGED_RECEIVERS) {
        const auto add = [=](const char* name, const GPSBaseStationConfig& base, Error expected) {
            if (type == GPSType::unicore && std::holds_alternative<GPSBaseStationConfig::SurveyIn>(base.mode)) {
                expected = Error::UnsupportedBaseMode;
            }
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(typeName, name)))
                << type << GPSReceiverConfig{.role = Role::RTKBase, .base = base} << expected;
        };
        add("survey", VALID_SURVEY, Error::None);
        add("fixed", VALID_FIXED, Error::None);
        add("missing-survey", {}, Error::InvalidSurveyIn);
        add("missing-fixed", {.mode = GPSBaseStationConfig::Fixed{}}, Error::InvalidFixedBase);
    }
    QTest::newRow("passive-valid") << GPSType::passive << GPSReceiverConfig{.role = Role::Passive, .baudRate = 115200}
                                   << Error::None;
    QTest::newRow("passive-rejects-base")
        << GPSType::passive << GPSReceiverConfig{.role = Role::Passive, .base = VALID_SURVEY, .baudRate = 115200}
        << Error::UnsupportedBaseMode;
    QTest::newRow("passive-requires-baud")
        << GPSType::passive << GPSReceiverConfig{.role = Role::Passive} << Error::InvalidBaudRate;
    QTest::newRow("managed-rejects-passive")
        << GPSType::ublox << GPSReceiverConfig{.role = Role::Passive, .baudRate = 115200} << Error::UnsupportedRole;
    QTest::newRow("passive-rejects-base-role")
        << GPSType::passive << GPSReceiverConfig{.base = VALID_SURVEY} << Error::UnsupportedRole;
    QTest::newRow("unsupported-persistent")
        << GPSType::ublox << GPSReceiverConfig{.base = VALID_SURVEY, .allowPersistentChanges = true}
        << Error::UnsupportedPersistentConfiguration;
    QTest::newRow("supported-persistent")
        << GPSType::quectel << GPSReceiverConfig{.base = VALID_SURVEY, .allowPersistentChanges = true} << Error::None;
    QTest::newRow("receiver-averaging") << GPSType::unicore
                                        << GPSReceiverConfig{.base = {.mode =
                                                                          GPSBaseStationConfig::ReceiverAveraging{1s}}}
                                        << Error::None;
    QTest::newRow("compact-observations")
        << GPSType::ublox << GPSReceiverConfig{.base = {.mode = VALID_SURVEY.mode, .compactObservations = true}}
        << Error::None;
    for (const auto type : {GPSType::septentrio, GPSType::unicore, GPSType::quectel}) {
        QTest::newRow(qPrintable(QStringLiteral("unsupported-compact-observations-%1").arg(static_cast<int>(type))))
            << type << GPSReceiverConfig{.base = {.mode = VALID_FIXED.mode, .compactObservations = true}}
            << Error::UnsupportedCompactObservations;
    }
    QTest::newRow("passive-rejects-compact-observations")
        << GPSType::passive
        << GPSReceiverConfig{.role = Role::Passive, .base = {.compactObservations = true}, .baudRate = 115200}
        << Error::UnsupportedBaseMode;
    QTest::newRow("unsupported-receiver-averaging")
        << GPSType::ublox << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1s}}}
        << Error::UnsupportedBaseMode;
    for (int value : {-1, 8, 255}) {
        QTest::newRow(qPrintable(QStringLiteral("unknown-type-%1").arg(value)))
            << static_cast<GPSType>(value) << GPSReceiverConfig{} << Error::UnknownReceiver;
    }
    for (int value : {-1, 2, 255}) {
        for (const auto& [typeName, type] : MANAGED_RECEIVERS) {
            QTest::newRow(qPrintable(QStringLiteral("%1-unknown-role-%2").arg(typeName).arg(value)))
                << type << GPSReceiverConfig{.role = static_cast<Role>(value)} << Error::InvalidRole;
        }
        QTest::newRow(qPrintable(QStringLiteral("role-precedes-unknown-receiver-%1").arg(value)))
            << static_cast<GPSType>(-1) << GPSReceiverConfig{.role = static_cast<Role>(value)} << Error::InvalidRole;
    }
}

void GPSReceiverConfigTest::_receiverValidation()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_baseModesExclusive()
{
    GPSReceiverConfig config{.base = VALID_FIXED};
    QVERIFY(std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode));
    QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), Error::None);
    config.base.mode = GPSBaseStationConfig::SurveyIn{0.0001, 1s};
    QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), Error::None);
    config.base.mode = GPSBaseStationConfig::ReceiverAveraging{1s};
    QCOMPARE(gpsValidateReceiverConfig(GPSType::unicore, config), Error::None);
}

void GPSReceiverConfigTest::_validationPrecedence_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");

    QTest::newRow("invalid-role-precedes-unknown-receiver")
        << static_cast<GPSType>(-1) << GPSReceiverConfig{.role = static_cast<Role>(-1)} << Error::InvalidRole;
    QTest::newRow("unknown-receiver-precedes-invalid-base")
        << static_cast<GPSType>(-1) << GPSReceiverConfig{} << Error::UnknownReceiver;
    QTest::newRow("unsupported-role-precedes-invalid-base")
        << GPSType::passive << GPSReceiverConfig{} << Error::UnsupportedRole;
    QTest::newRow("persistent-precedes-base") << GPSType::ublox << GPSReceiverConfig{.allowPersistentChanges = true}
                                              << Error::UnsupportedPersistentConfiguration;
    QTest::newRow("unsupported-base-mode-precedes-invalid-baud")
        << GPSType::ublox
        << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{0s}}, .baudRate = 1}
        << Error::UnsupportedBaseMode;
    QTest::newRow("invalid-base-precedes-invalid-baud")
        << GPSType::ublox << GPSReceiverConfig{.baudRate = 1} << Error::InvalidSurveyIn;
    QTest::newRow("invalid-baud-after-valid-base")
        << GPSType::ublox << GPSReceiverConfig{.base = VALID_SURVEY, .baudRate = 1} << Error::InvalidBaudRate;
}

void GPSReceiverConfigTest::_validationPrecedence()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_newReceivers_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");
    QTest::newRow("passive-valid") << GPSType::passive << GPSReceiverConfig{.role = Role::Passive, .baudRate = 115200}
                                   << Error::None;
    QTest::newRow("passive-missing-baud")
        << GPSType::passive << GPSReceiverConfig{.role = Role::Passive} << Error::InvalidBaudRate;
    QTest::newRow("quectel-persistent") << GPSType::quectel
                                        << GPSReceiverConfig{.base = VALID_SURVEY, .allowPersistentChanges = true}
                                        << Error::None;
    QTest::newRow("quectel-receiver-averaging")
        << GPSType::quectel << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1s}}}
        << Error::UnsupportedBaseMode;
    QTest::newRow("unicore-receiver-averaging")
        << GPSType::unicore << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1s}}}
        << Error::None;
    QTest::newRow("unicore-persistent") << GPSType::unicore
                                        << GPSReceiverConfig{.base = VALID_SURVEY, .allowPersistentChanges = true}
                                        << Error::UnsupportedPersistentConfiguration;
}

void GPSReceiverConfigTest::_newReceivers()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_newCapabilities()
{
    const auto quectel = gpsReceiverCapabilities(GPSType::quectel, Role::RTKBase);
    QVERIFY(quectel.recognized);
    QVERIFY(quectel.rtkBase);
    QVERIFY(quectel.surveyIn);
    QVERIFY(quectel.persistentConfiguration);
    QVERIFY(!quectel.receiverAveraging);
    QVERIFY(!quectel.passive);

    const auto unicore = gpsReceiverCapabilities(GPSType::unicore, Role::RTKBase);
    QVERIFY(unicore.recognized);
    QVERIFY(unicore.rtkBase);
    QVERIFY(unicore.receiverAveraging);
    QVERIFY(!unicore.surveyIn);
    QVERIFY(!unicore.persistentConfiguration);

    const auto passive = gpsReceiverCapabilities(GPSType::passive, Role::Passive);
    QVERIFY(passive.recognized);
    QVERIFY(passive.passive);
    QVERIFY(!passive.rtkBase);
}

void GPSReceiverConfigTest::_descriptorIdentities()
{
    QSet<int> manufacturers;
    for (const auto& descriptor : gpsReceiverDescriptors()) {
        QVERIFY(!manufacturers.contains(descriptor.manufacturerId));
        manufacturers.insert(descriptor.manufacturerId);
        QCOMPARE(gpsReceiverDescriptor(descriptor.type), &descriptor);
        QCOMPARE(gpsReceiverDescriptorForManufacturer(descriptor.manufacturerId), &descriptor);
        QVERIFY(descriptor.capabilities.recognized);
        QVERIFY(descriptor.capabilities.rtkBase || descriptor.capabilities.passive);
    }
    QVERIFY(!gpsReceiverDescriptor(static_cast<GPSType>(-1)));
    QVERIFY(!gpsReceiverDescriptorForManufacturer(-1));
}

void GPSReceiverConfigTest::_presentation_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("specific");
    QTest::addColumn<QStringList>("trueKeys");
    QTest::addColumn<QStringList>("falseKeys");

    // Automatic offers Quectel's flash-save consent and notes, for the case that it identifies a Quectel receiver.
    QTest::newRow("automatic") << 0 << false
                               << QStringList{"recognized",
                                              "automatic",
                                              "rtkBase",
                                              "surveyIn",
                                              "receiverAveraging",
                                              "surveyAccuracy",
                                              "surveyDuration",
                                              "fixedBaseAccuracy",
                                              "persistentConfiguration",
                                              "restartOnConnect",
                                              "surveyMaySavePosition"}
                               << QStringList{"specificReceiver", "passive", "observationAccuracyFilter",
                                              "acceptedObservationTime", "reportsSurveyDuration"};
    QTest::newRow("ublox") << 4 << true << QStringList{"recognized",        "specificReceiver",     "rtkBase",
                                                       "surveyIn",          "surveyAccuracy",       "surveyDuration",
                                                       "fixedBaseAccuracy", "reportsSurveyDuration"}
                           << QStringList{"receiverAveraging", "passive", "persistentConfiguration", "restartOnConnect",
                                          "surveyMaySavePosition"};
    QTest::newRow("unicore") << 5 << true
                             << QStringList{"recognized", "specificReceiver", "rtkBase", "receiverAveraging"}
                             << QStringList{"surveyIn",          "passive",
                                            "surveyAccuracy",    "surveyDuration",
                                            "fixedBaseAccuracy", "persistentConfiguration",
                                            "restartOnConnect",  "surveyMaySavePosition"};
    QTest::newRow("quectel") << 6 << true
                             << QStringList{"recognized",
                                            "specificReceiver",
                                            "rtkBase",
                                            "surveyIn",
                                            "surveyAccuracy",
                                            "surveyDuration",
                                            "observationAccuracyFilter",
                                            "acceptedObservationTime",
                                            "reportsSurveyDuration",
                                            "persistentConfiguration",
                                            "restartOnConnect",
                                            "surveyMaySavePosition"}
                             << QStringList{"receiverAveraging", "passive", "fixedBaseAccuracy"};
    QTest::newRow("passive") << 7 << true << QStringList{"recognized", "specificReceiver", "passive"}
                             << QStringList{"rtkBase",
                                            "surveyIn",
                                            "receiverAveraging",
                                            "surveyAccuracy",
                                            "surveyDuration",
                                            "fixedBaseAccuracy",
                                            "persistentConfiguration",
                                            "restartOnConnect",
                                            "surveyMaySavePosition"};
    QTest::newRow("unknown-manufacturer")
        << 999 << false << QStringList{} << QStringList{"recognized", "specificReceiver",  "automatic", "rtkBase",
                                                        "surveyIn",   "receiverAveraging", "passive"};
}

void GPSReceiverConfigTest::_presentation()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, specific);
    QFETCH(QStringList, trueKeys);
    QFETCH(QStringList, falseKeys);
    const GPSReceiverPresentation& presentation = gpsReceiverPresentation(manufacturer);
    const auto value = [&presentation](const QString& key) {
        const auto& metaObject = GPSReceiverPresentation::staticMetaObject;
        const int index = metaObject.indexOfProperty(qPrintable(key));
        return index >= 0 && metaObject.property(index).readOnGadget(&presentation).toBool();
    };
    QCOMPARE(presentation.specificReceiver, specific);
    for (const auto& key : trueKeys) {
        QVERIFY2(value(key), qPrintable(key));
    }
    for (const auto& key : falseKeys) {
        QVERIFY2(GPSReceiverPresentation::staticMetaObject.indexOfProperty(qPrintable(key)) >= 0, qPrintable(key));
        QVERIFY2(!value(key), qPrintable(key));
    }
}

void GPSReceiverConfigTest::_configForSettings_data()
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

void GPSReceiverConfigTest::_configForSettings()
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
        QCOMPARE(config.role, Role::Passive);
        QCOMPARE(config.base, GPSBaseStationConfig{});
        return;
    }
    QCOMPARE(config.role, Role::RTKBase);
    const GPSBaseStationConfig::Mode expected =
        mode == 1 ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{.position = SAVED_FIXED_POSITION,
                                                                           .accuracyMeters = 1.5f}}
        : mode == 2
            ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 321s}}
            : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.75, .duration = 195s}};
    QVERIFY(config.base.mode == expected);
}

void GPSReceiverConfigTest::_settingsDiagnostics_data()
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
    QTest::newRow("unknown-mode") << GPSType::quectel << 42 << 60U << 115200U
                                  << QStringLiteral("Select a supported base mode.");
    QTest::newRow("zero-averaging") << GPSType::unicore << 2 << 0U << 115200U << invalidAveraging;
    QTest::newRow("excess-averaging") << GPSType::unicore << 2 << 3601U << 115200U << invalidAveraging;
    QTest::newRow("minimum-averaging") << GPSType::unicore << 2 << 1U << 115200U << QString();
    QTest::newRow("maximum-averaging") << GPSType::unicore << 2 << 3600U << 115200U << QString();
    QTest::newRow("passive-needs-baud") << GPSType::passive << 0 << 60U << 0U
                                        << gpsReceiverConfigErrorText(Error::InvalidBaudRate);
    QTest::newRow("passive-ignores-stale-base-settings") << GPSType::passive << 42 << 0U << 115200U << QString();
}

void GPSReceiverConfigTest::_settingsDiagnostics()
{
    QFETCH(GPSType, type);
    QFETCH(int, mode);
    QFETCH(uint, averagingDuration);
    QFETCH(uint, baud);
    QFETCH(QString, diagnostic);
    auto settings = savedSettings(mode);
    settings.averagingMaximumDuration = std::chrono::seconds(averagingDuration);
    // A supported request clears an earlier diagnostic.
    QString error = QStringLiteral("earlier diagnostic");
    (void) gpsReceiverConfigFor(settings, type, baud, false, &error);
    QCOMPARE(error, diagnostic);
}

void GPSReceiverConfigTest::_compactObservationsFollowSupport()
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

void GPSReceiverConfigTest::_persistentConsent_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<bool>("allowPersistentChanges");
    for (const auto& [name, type] : SETTINGS_RECEIVERS) {
        for (const bool allow : {false, true}) {
            QTest::addRow("%s-consent-%d", name, allow) << type << allow;
        }
    }
}

void GPSReceiverConfigTest::_persistentConsent()
{
    QFETCH(GPSType, type);
    QFETCH(bool, allowPersistentChanges);
    QString error;
    const auto config = gpsReceiverConfigFor(savedSettings(1), type, 115200, allowPersistentChanges, &error);
    QCOMPARE(error.isEmpty(), !allowPersistentChanges || type == GPSType::quectel);
    QCOMPARE(config.allowPersistentChanges, allowPersistentChanges);
}

void GPSReceiverConfigTest::_baseStationState()
{
    using Mode = GPSBaseStationSettings::Mode;
    const auto stateFor = [](int mode, GPSType type) {
        return gpsBaseStationStateFor(gpsReceiverConfigFor(savedSettings(mode), type, 0, false, nullptr));
    };
    const auto passive = gpsBaseStationStateFor({.role = Role::Passive, .baudRate = 115200});
    QVERIFY(!passive.mode);
    QVERIFY(!passive.position);
    QVERIFY(!passive.surveyAccuracyLimitMeters);

    const auto fixed = stateFor(1, GPSType::ublox);
    QCOMPARE(fixed.mode, std::optional(Mode::Fixed));
    QVERIFY(fixed.position);
    QCOMPARE(fixed.position->first, SAVED_FIXED_POSITION);
    QCOMPARE(fixed.position->second, 1.5);
    QVERIFY(!fixed.surveyAccuracyLimitMeters);

    const auto survey = stateFor(0, GPSType::ublox);
    QCOMPARE(survey.mode, std::optional(Mode::SurveyIn));
    QVERIFY(!survey.position);
    QCOMPARE(survey.surveyAccuracyLimitMeters, std::optional(1.75));

    const auto averaging = stateFor(2, GPSType::unicore);
    QCOMPARE(averaging.mode, std::optional(Mode::ReceiverAveraging));
    QVERIFY(!averaging.position);
    QVERIFY(!averaging.surveyAccuracyLimitMeters);
}

void GPSReceiverConfigTest::_surveyUpdatesBasePosition_data()
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

void GPSReceiverConfigTest::_surveyUpdatesBasePosition()
{
    QFETCH(int, mode);
    QFETCH(bool, valid);
    QFETCH(double, latitude);
    QFETCH(double, meanAccuracy);
    QFETCH(double, expectedAccuracy);
    auto state = mode < 0 ? gpsBaseStationStateFor({.role = Role::Passive, .baudRate = 115200})
                          : gpsBaseStationStateFor(gpsReceiverConfigFor(
                                savedSettings(mode), mode == 2 ? GPSType::unicore : GPSType::ublox, 0, false, nullptr));
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

void GPSReceiverConfigTest::_detectedReceiverFit_data()
{
    QTest::addColumn<GPSType>("detected");
    QTest::addColumn<GPSReceiverConfig>("request");
    QTest::addColumn<bool>("persistent");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("fallback");
    QTest::addColumn<QString>("error");

    GPSReceiverConfig compactSurvey{.base = VALID_SURVEY, .allowPersistentChanges = true};
    compactSurvey.base.compactObservations = true;
    const GPSReceiverConfig averaging{
        .base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 60s}}};
    const GPSReceiverConfig survey{.base = VALID_SURVEY};
    // Consent is a permission, kept only where the family can save settings; MSM4 falls back to MSM7 elsewhere.
    QTest::newRow("ublox-keeps-compact") << GPSType::ublox << compactSurvey << false << true << false << QString();
    QTest::newRow("quectel-keeps-consent") << GPSType::quectel << compactSurvey << true << false << true << QString();
    QTest::newRow("septentrio-averaging")
        << GPSType::septentrio << averaging << false << false << false
        << QStringLiteral("Detected Septentrio receiver does not support receiver-managed averaging");
    QTest::newRow("unicore-survey") << GPSType::unicore << survey << false << false << false
                                    << QStringLiteral("Detected Unicore receiver does not support survey-in");
    QTest::newRow("unicore-averaging") << GPSType::unicore << averaging << false << false << false << QString();
}

void GPSReceiverConfigTest::_detectedReceiverFit()
{
    QFETCH(GPSType, detected);
    QFETCH(GPSReceiverConfig, request);
    QFETCH(bool, persistent);
    QFETCH(bool, compact);
    QFETCH(bool, fallback);
    QFETCH(QString, error);
    QVERIFY(gpsReceiverConfigError(GPSType::automatic, request).isEmpty());
    bool compactFallback = !fallback;
    const GPSReceiverConfig fitted = gpsReceiverConfigForDetected(detected, request, &compactFallback);
    QCOMPARE(fitted.allowPersistentChanges, persistent);
    QCOMPARE(fitted.base.compactObservations, compact);
    QCOMPARE(compactFallback, fallback);
    QVERIFY(fitted.base.mode == request.base.mode);
    QCOMPARE(gpsDetectedReceiverConfigError(detected, fitted), error);
}

UT_REGISTER_TEST(GPSReceiverConfigTest, TestLabel::Unit)
#include "GPSReceiverConfigTest.moc"
