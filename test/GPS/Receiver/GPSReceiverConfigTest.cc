#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>

#include "GPSDriverReports.h"
#include "GPSEllipsoidPosition.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "UnitTest.h"

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
    .mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 0.0001, .durationSecs = 1}};
constexpr GPSBaseStationConfig VALID_FIXED{
    .mode = GPSBaseStationConfig::Fixed{
        .position = {.latitudeDegrees = 0.0, .longitudeDegrees = 0.0, .altitudeMeters = 0.0f}}};
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
};

void GPSReceiverConfigTest::_defaults()
{
    const GPSReceiverConfig config;
    QCOMPARE(config.role, Role::RTKBase);
    QVERIFY(!config.allowPersistentChanges);
    QVERIFY(std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode));
    QCOMPARE(std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters, 0.0);
    QCOMPARE(std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs, int64_t{0});
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
        QTest::newRow(name) << GPSBaseStationConfig{.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracy,
                                                                                           .durationSecs = duration}}
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
        .mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = MAX_SURVEY_ACCURACY, .durationSecs = MAX_DURATION}};
    QCOMPARE(gpsValidateBaseStationConfig(config), Error::None);
    QCOMPARE(static_cast<uint32_t>(std::get<GPSBaseStationConfig::SurveyIn>(config.mode).accuracyMeters * 10000.0),
             (std::numeric_limits<uint32_t>::max)());
    QCOMPARE(static_cast<uint32_t>(std::get<GPSBaseStationConfig::SurveyIn>(config.mode).durationSecs),
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
                                                                          GPSBaseStationConfig::ReceiverAveraging{1}}}
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
        << GPSType::ublox << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1}}}
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
    config.base.mode = GPSBaseStationConfig::SurveyIn{0.0001, 1};
    QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), Error::None);
    config.base.mode = GPSBaseStationConfig::ReceiverAveraging{1};
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
        << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{0}}, .baudRate = 1}
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
        << GPSType::quectel << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1}}}
        << Error::UnsupportedBaseMode;
    QTest::newRow("unicore-receiver-averaging")
        << GPSType::unicore << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1}}}
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
    QSet<QByteArray> keys;
    for (const auto& descriptor : gpsReceiverDescriptors()) {
        QVERIFY(!manufacturers.contains(descriptor.manufacturerId));
        manufacturers.insert(descriptor.manufacturerId);
        QVERIFY(!descriptor.detectionKey.empty());
        const QByteArray key(descriptor.detectionKey.data(), qsizetype(descriptor.detectionKey.size()));
        QVERIFY(!keys.contains(key));
        keys.insert(key);
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

    QTest::newRow("all") << 0 << false
                         << QStringList{"recognized",     "rtkBase",        "surveyIn",         "receiverAveraging",
                                        "surveyAccuracy", "surveyDuration", "fixedBaseAccuracy"}
                         << QStringList{"specificReceiver",          "passive",
                                        "observationAccuracyFilter", "acceptedObservationTime",
                                        "reportsSurveyDuration",     "persistentConfiguration",
                                        "restartOnConnect",          "surveyMaySavePosition"};
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
        << 999 << false << QStringList{}
        << QStringList{"recognized", "specificReceiver", "rtkBase", "surveyIn", "receiverAveraging", "passive"};
}

void GPSReceiverConfigTest::_presentation()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, specific);
    QFETCH(QStringList, trueKeys);
    QFETCH(QStringList, falseKeys);
    const QVariantMap presentation = gpsReceiverPresentation(manufacturer);
    QCOMPARE(presentation.value("specificReceiver").toBool(), specific);
    for (const auto& key : trueKeys) {
        QVERIFY2(presentation.value(key).toBool(), qPrintable(key));
    }
    for (const auto& key : falseKeys) {
        QVERIFY2(!presentation.value(key).toBool(), qPrintable(key));
    }
}

UT_REGISTER_TEST(GPSReceiverConfigTest, TestLabel::Unit)
#include "GPSReceiverConfigTest.moc"
