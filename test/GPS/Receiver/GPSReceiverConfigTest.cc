#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <type_traits>
#include <utility>

#include <QtTest/QTest>

#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "PortableTest.h"

namespace {
using Error = GPSReceiverConfigError;
using Role = GPSReceiverConfig::Role;

constexpr double NAN_DOUBLE = std::numeric_limits<double>::quiet_NaN();
constexpr double INF_DOUBLE = std::numeric_limits<double>::infinity();
constexpr float NAN_FLOAT = std::numeric_limits<float>::quiet_NaN();
constexpr float INF_FLOAT = std::numeric_limits<float>::infinity();
constexpr int64_t MAX_DURATION = (std::numeric_limits<uint32_t>::max)();
constexpr double MAX_SURVEY_ACCURACY = static_cast<double>(MAX_DURATION) / 10000.0;
constexpr float PI = std::numbers::pi_v<float>;
constexpr std::pair<const char*, GPSType> RECEIVERS[] = {
    {"ublox", GPSType::ublox},
    {"trimble", GPSType::trimble},
    {"septentrio", GPSType::septentrio},
    {"femto", GPSType::femto},
};
constexpr std::pair<const char*, Role> ROLES[] = {{"base", Role::RTKBase}, {"position", Role::Position}};
constexpr GPSBaseStationConfig VALID_SURVEY{.surveyInAccMeters = 0.0001, .surveyInDurationSecs = 1};
constexpr GPSBaseStationConfig VALID_FIXED{
    .useFixedBase = true, .fixedBaseLatitude = 0.0, .fixedBaseLongitude = 0.0, .fixedBaseAltitudeMeters = 0.0f};
}  // namespace

static_assert(std::is_aggregate_v<GPSReceiverConfig>);

class GPSReceiverConfigTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _baseValidation_data();
    void _baseValidation();
    void _surveyWireRange();
    void _fixedWireRepresentability();
    void _capabilities_data();
    void _capabilities();
    void _receiverValidation_data();
    void _receiverValidation();
    void _constellations_data();
    void _constellations();
    void _dynamicModel_data();
    void _dynamicModel();
    void _headingOffset_data();
    void _headingOffset();
    void _optionalRequests();
    void _validationPrecedence_data();
    void _validationPrecedence();
};

void GPSReceiverConfigTest::_defaults()
{
    const GPSReceiverConfig config;
    QCOMPARE(config.role, Role::RTKBase);
    QCOMPARE(config.constellationMask, 0u);
    QVERIFY(!config.dynamicModel.has_value());
    QVERIFY(!config.headingOffsetRadians.has_value());
    QVERIFY(!config.base.useFixedBase);
    QCOMPARE(config.base.surveyInAccMeters, 0.0);
    QCOMPARE(config.base.surveyInDurationSecs, int64_t{0});
    QVERIFY(std::isnan(config.base.fixedBaseLatitude));
    QVERIFY(std::isnan(config.base.fixedBaseLongitude));
    QVERIFY(std::isnan(config.base.fixedBaseAltitudeMeters));
    QCOMPARE(config.base.fixedBaseAccuracyMeters, 0.0f);

    const GPSReceiverCapabilities capabilities;
    QVERIFY(!capabilities.recognized);
    QVERIFY(!capabilities.position);
    QVERIFY(!capabilities.rtkBase);
    QCOMPARE(capabilities.constellationMask, 0u);
    QVERIFY(!capabilities.dynamicModel);
    QVERIFY(!capabilities.headingOffset);
}

void GPSReceiverConfigTest::_baseValidation_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::addColumn<Error>("expected");

    const auto survey = [](const char* name, double accuracy, int64_t duration, Error expected) {
        QTest::newRow(name) << GPSBaseStationConfig{.surveyInAccMeters = accuracy, .surveyInDurationSecs = duration}
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
        QTest::newRow(name) << GPSBaseStationConfig{.useFixedBase = true,
                                                    .fixedBaseLatitude = latitude,
                                                    .fixedBaseLongitude = longitude,
                                                    .fixedBaseAltitudeMeters = altitude,
                                                    .fixedBaseAccuracyMeters = accuracy}
                            << expected;
    };
    QTest::newRow("missing-fixed-position") << GPSBaseStationConfig{.useFixedBase = true} << Error::InvalidFixedBase;
    QTest::newRow("missing-fixed-latitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLongitude = 8, .fixedBaseAltitudeMeters = 500}
        << Error::InvalidFixedBase;
    QTest::newRow("missing-fixed-longitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLatitude = 47, .fixedBaseAltitudeMeters = 500}
        << Error::InvalidFixedBase;
    QTest::newRow("missing-fixed-altitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLatitude = 47, .fixedBaseLongitude = 8}
        << Error::InvalidFixedBase;
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
    // A single multiplication by 10000.f incorrectly rejects this valid legacy boundary.
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
    const GPSBaseStationConfig config{.surveyInAccMeters = MAX_SURVEY_ACCURACY, .surveyInDurationSecs = MAX_DURATION};
    QCOMPARE(gpsValidateBaseStationConfig(config), Error::None);
    QCOMPARE(static_cast<uint32_t>(config.surveyInAccMeters * 10000.0), (std::numeric_limits<uint32_t>::max)());
    QCOMPARE(static_cast<uint32_t>(config.surveyInDurationSecs), (std::numeric_limits<uint32_t>::max)());
}

void GPSReceiverConfigTest::_fixedWireRepresentability()
{
    const GPSBaseStationConfig config{.useFixedBase = true,
                                      .fixedBaseLatitude = 47,
                                      .fixedBaseLongitude = 8,
                                      .fixedBaseAltitudeMeters = 21474836.0f,
                                      .fixedBaseAccuracyMeters = 429496.71875f};
    QCOMPARE(gpsValidateBaseStationConfig(config), Error::None);
    QCOMPARE(static_cast<int32_t>(static_cast<double>(config.fixedBaseAltitudeMeters) * 100.0), 2147483600);
    const float accuracyMillimeters = config.fixedBaseAccuracyMeters * 1000.0f;
    QCOMPARE(static_cast<uint32_t>(accuracyMillimeters * 10.0f), 4294967040u);
}

void GPSReceiverConfigTest::_capabilities_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<Role>("role");
    QTest::addColumn<GPSReceiverCapabilities>("expected");
    for (const auto& [typeName, type] : RECEIVERS) {
        for (const auto& [roleName, role] : ROLES) {
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(typeName, roleName)))
                << type << role
                << GPSReceiverCapabilities{.recognized = true,
                                           .position = type == GPSType::ublox,
                                           .rtkBase = true,
                                           .constellationMask = type == GPSType::ublox ? 0x1fu : 0u,
                                           .dynamicModel = type == GPSType::ublox && role == Role::Position,
                                           .headingOffset = false};
        }
    }
    for (int value : {-1, 4, 255}) {
        for (const auto& [roleName, role] : ROLES) {
            QTest::newRow(qPrintable(QStringLiteral("unknown-type-%1-%2").arg(value).arg(roleName)))
                << static_cast<GPSType>(value) << role << GPSReceiverCapabilities{};
        }
    }
    for (int value : {-1, 2, 255}) {
        for (const auto& [typeName, type] : RECEIVERS) {
            QTest::newRow(qPrintable(QStringLiteral("%1-unknown-role-%2").arg(typeName).arg(value)))
                << type << static_cast<Role>(value) << GPSReceiverCapabilities{};
        }
        QTest::newRow(qPrintable(QStringLiteral("unknown-type-and-role-%1").arg(value)))
            << static_cast<GPSType>(-1) << static_cast<Role>(value) << GPSReceiverCapabilities{};
    }
}

void GPSReceiverConfigTest::_capabilities()
{
    QFETCH(GPSType, type);
    QFETCH(Role, role);
    QFETCH(GPSReceiverCapabilities, expected);
    const auto actual = gpsReceiverCapabilities(type, role);
    QCOMPARE(actual.recognized, expected.recognized);
    QCOMPARE(actual.position, expected.position);
    QCOMPARE(actual.rtkBase, expected.rtkBase);
    QCOMPARE(actual.constellationMask, expected.constellationMask);
    QCOMPARE(actual.dynamicModel, expected.dynamicModel);
    QCOMPARE(actual.headingOffset, expected.headingOffset);
}

void GPSReceiverConfigTest::_receiverValidation_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");
    for (const auto& [typeName, type] : RECEIVERS) {
        for (const auto& [roleName, role] : ROLES) {
            const auto add = [=](const char* name, const GPSBaseStationConfig& base, Error expected) {
                if (role == Role::Position && type != GPSType::ublox) {
                    expected = Error::UnsupportedRole;
                }
                QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3").arg(typeName, roleName, name)))
                    << type << GPSReceiverConfig{.role = role, .base = base} << expected;
            };
            add("survey", VALID_SURVEY, Error::None);
            add("fixed", VALID_FIXED, Error::None);
            add("missing-survey", {}, role == Role::Position ? Error::None : Error::InvalidSurveyIn);
            add("missing-fixed", {.useFixedBase = true},
                role == Role::Position ? Error::None : Error::InvalidFixedBase);
        }
    }
    for (int value : {-1, 4, 255}) {
        for (const auto& [roleName, role] : ROLES) {
            QTest::newRow(qPrintable(QStringLiteral("unknown-type-%1-%2").arg(value).arg(roleName)))
                << static_cast<GPSType>(value) << GPSReceiverConfig{.role = role} << Error::UnknownReceiver;
        }
    }
    for (int value : {-1, 2, 255}) {
        for (const auto& [typeName, type] : RECEIVERS) {
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

void GPSReceiverConfigTest::_constellations_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<Role>("role");
    QTest::addColumn<uint32_t>("mask");
    QTest::addColumn<Error>("expected");
    for (const auto& [typeName, type] : RECEIVERS) {
        for (const auto& [roleName, role] : ROLES) {
            const auto add = [=](uint32_t mask, Error expected) {
                if (role == Role::Position && type != GPSType::ublox) {
                    expected = Error::UnsupportedRole;
                }
                QTest::newRow(qPrintable(QStringLiteral("%1-%2-mask-%3").arg(typeName, roleName).arg(mask)))
                    << type << role << mask << expected;
            };
            for (uint32_t mask = 0; mask <= 31; ++mask) {
                add(mask, type == GPSType::ublox || mask == 0 ? Error::None : Error::UnsupportedConstellations);
            }
            for (uint32_t mask : {32u, 33u, 64u, 0x80000000u, 0xffffffffu}) {
                add(mask, type == GPSType::ublox ? Error::InvalidConstellations : Error::UnsupportedConstellations);
            }
        }
    }
}

void GPSReceiverConfigTest::_constellations()
{
    QFETCH(GPSType, type);
    QFETCH(Role, role);
    QFETCH(uint32_t, mask);
    QFETCH(Error, expected);
    const GPSReceiverConfig config{.role = role, .base = VALID_SURVEY, .constellationMask = mask};
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_dynamicModel_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");
    for (const auto& [typeName, type] : RECEIVERS) {
        for (const auto& [roleName, role] : ROLES) {
            const auto add = [=](const char* name, std::optional<int> model, Error expected) {
                if (role == Role::Position && type != GPSType::ublox) {
                    expected = Error::UnsupportedRole;
                } else if (model.has_value() && !(type == GPSType::ublox && role == Role::Position)) {
                    expected = Error::UnsupportedDynamicModel;
                }
                QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3").arg(typeName, roleName, name)))
                    << type << GPSReceiverConfig{.role = role, .base = VALID_SURVEY, .dynamicModel = model} << expected;
            };
            add("absent", std::nullopt, Error::None);
            add("int-minimum", (std::numeric_limits<int>::min)(), Error::InvalidDynamicModel);
            add("negative", -1, Error::InvalidDynamicModel);
            add("portable", 0, Error::None);
            add("reserved", 1, Error::InvalidDynamicModel);
            add("stationary", 2, Error::None);
            add("pedestrian", 3, Error::None);
            add("automotive", 4, Error::None);
            add("sea", 5, Error::None);
            add("airborne-1g", 6, Error::None);
            add("airborne-2g", 7, Error::None);
            add("airborne-4g", 8, Error::None);
            add("unsupported-newer-model", 9, Error::InvalidDynamicModel);
            add("byte-maximum", 255, Error::InvalidDynamicModel);
            add("must-not-narrow-to-portable", 256, Error::InvalidDynamicModel);
            add("int-maximum", (std::numeric_limits<int>::max)(), Error::InvalidDynamicModel);
        }
    }
}

void GPSReceiverConfigTest::_dynamicModel()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_headingOffset_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");
    for (const auto& [typeName, type] : RECEIVERS) {
        for (const auto& [roleName, role] : ROLES) {
            const auto add = [=](const char* name, std::optional<float> offset, Error expected) {
                if (role == Role::Position && type != GPSType::ublox) {
                    expected = Error::UnsupportedRole;
                } else if (offset.has_value()) {
                    expected = Error::UnsupportedHeadingOffset;
                }
                QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3").arg(typeName, roleName, name)))
                    << type << GPSReceiverConfig{.role = role, .base = VALID_SURVEY, .headingOffsetRadians = offset}
                    << expected;
            };
            add("absent", std::nullopt, Error::None);
            add("explicit-zero", 0.0f, Error::None);
            add("negative-zero", -0.0f, Error::None);
            add("positive", 0.5f, Error::None);
            add("negative", -0.5f, Error::None);
            add("minimum", -PI, Error::None);
            add("maximum", PI, Error::None);
            add("below-minimum", std::nextafter(-PI, -INF_FLOAT), Error::InvalidHeadingOffset);
            add("above-maximum", std::nextafter(PI, INF_FLOAT), Error::InvalidHeadingOffset);
            add("nan", NAN_FLOAT, Error::InvalidHeadingOffset);
            add("positive-infinity", INF_FLOAT, Error::InvalidHeadingOffset);
            add("negative-infinity", -INF_FLOAT, Error::InvalidHeadingOffset);
        }
    }
}

void GPSReceiverConfigTest::_headingOffset()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_optionalRequests()
{
    GPSReceiverConfig config{
        .role = Role::Position, .constellationMask = 31, .dynamicModel = 0, .headingOffsetRadians = 0.0f};
    QVERIFY(config.dynamicModel.has_value());
    QCOMPARE(*config.dynamicModel, 0);
    QVERIFY(config.headingOffsetRadians.has_value());
    QCOMPARE(*config.headingOffsetRadians, 0.0f);
    QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), Error::UnsupportedHeadingOffset);
    config.headingOffsetRadians.reset();
    QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), Error::None);
    config.dynamicModel.reset();
    QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), Error::None);
}

void GPSReceiverConfigTest::_validationPrecedence_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");
    GPSReceiverConfig config{.constellationMask = 32, .dynamicModel = 1, .headingOffsetRadians = NAN_FLOAT};
    QTest::newRow("base-precedes-optional-requests") << GPSType::ublox << config << Error::InvalidSurveyIn;
    config.role = Role::Position;
    for (const auto& [typeName, type] : RECEIVERS) {
        if (type != GPSType::ublox) {
            QTest::newRow(qPrintable(QStringLiteral("%1-role-precedes-optional-requests").arg(typeName)))
                << type << config << Error::UnsupportedRole;
        }
    }
    QTest::newRow("unknown-receiver-precedes-optional-requests")
        << static_cast<GPSType>(-1) << config << Error::UnknownReceiver;
    GPSReceiverConfig invalidRole = config;
    invalidRole.role = static_cast<Role>(-1);
    QTest::newRow("invalid-role-precedes-unknown-receiver")
        << static_cast<GPSType>(-1) << invalidRole << Error::InvalidRole;
    QTest::newRow("constellations-precede-dynamic-model") << GPSType::ublox << config << Error::InvalidConstellations;
    config.constellationMask = 0;
    QTest::newRow("dynamic-model-precedes-heading") << GPSType::ublox << config << Error::InvalidDynamicModel;
    config.dynamicModel.reset();
    QTest::newRow("unsupported-heading-precedes-value") << GPSType::ublox << config << Error::UnsupportedHeadingOffset;
    QTest::newRow("unsupported-role-precedes-heading-value") << GPSType::septentrio << config << Error::UnsupportedRole;
}

void GPSReceiverConfigTest::_validationPrecedence()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

QGC_REGISTER_PORTABLE_TEST(GPSReceiverConfigTest, TestLabel::Unit)
#include "GPSReceiverConfigTest.moc"
