#include "GPSReceiverConfigTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

#include "FactMetaData.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverReports.h"

using namespace std::chrono_literals;

namespace {
using Error = GPSReceiverConfigError;

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
}  // namespace

void GPSReceiverConfigTest::_unknownDefaults()
{
    // An unset configuration needs consent for persistent changes and has no fixed position.
    const GPSReceiverConfig config;
    QVERIFY(!config.allowPersistentChanges);
    const GPSBaseStationConfig::Fixed fixed;
    QVERIFY(std::isnan(fixed.position.latitudeDegrees));
    QVERIFY(std::isnan(fixed.position.longitudeDegrees));
    QVERIFY(std::isnan(fixed.position.altitudeMeters));
    QVERIFY(!GPSReceiverCapabilities{}.recognized);

    // Reports start out unknown rather than at a plausible value.
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

void GPSReceiverConfigTest::_capabilities_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverCapabilities>("expected");
    constexpr GPSReceiverCapabilities survey{.recognized = true, .rtkBase = true, .surveyIn = true};
    QTest::newRow("trimble") << GPSType::trimble << survey;
    QTest::newRow("septentrio") << GPSType::septentrio << survey;
    QTest::newRow("femto") << GPSType::femto << survey;
    QTest::newRow("ublox") << GPSType::ublox
                           << GPSReceiverCapabilities{
                                  .recognized = true, .rtkBase = true, .surveyIn = true, .compactObservations = true};
    QTest::newRow("unicore") << GPSType::unicore
                             << GPSReceiverCapabilities{.recognized = true, .rtkBase = true, .receiverAveraging = true};
    QTest::newRow("quectel") << GPSType::quectel
                             << GPSReceiverCapabilities{.recognized = true,
                                                        .rtkBase = true,
                                                        .surveyIn = true,
                                                        .persistentConfiguration = true};
    QTest::newRow("passive") << GPSType::passive << GPSReceiverCapabilities{.recognized = true, .passive = true};
    // Automatic accepts every option some base family supports, until detection names the family.
    QTest::newRow("automatic") << GPSType::automatic
                               << GPSReceiverCapabilities{.recognized = true,
                                                          .rtkBase = true,
                                                          .surveyIn = true,
                                                          .receiverAveraging = true,
                                                          .persistentConfiguration = true,
                                                          .compactObservations = true};
    for (int value : {-1, 8, 255}) {
        QTest::addRow("unknown-type-%d", value) << static_cast<GPSType>(value) << GPSReceiverCapabilities{};
    }
}

void GPSReceiverConfigTest::_capabilities()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverCapabilities, expected);
    const auto actual = gpsReceiverCapabilities(type);
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
            QTest::addRow("%s-%s", typeName, name) << type << GPSReceiverConfig{.base = base} << expected;
        };
        add("survey", VALID_SURVEY, Error::None);
        add("fixed", VALID_FIXED, Error::None);
        add("missing-survey", {}, Error::InvalidSurveyIn);
        add("missing-fixed", {.mode = GPSBaseStationConfig::Fixed{}}, Error::InvalidFixedBase);
    }
    QTest::newRow("passive-valid") << GPSType::passive << GPSReceiverConfig{.baudRate = 115200} << Error::None;
    QTest::newRow("passive-rejects-base")
        << GPSType::passive << GPSReceiverConfig{.base = VALID_SURVEY, .baudRate = 115200}
        << Error::UnsupportedBaseMode;
    QTest::newRow("passive-requires-baud") << GPSType::passive << GPSReceiverConfig{} << Error::InvalidBaudRate;
    QTest::newRow("unsupported-persistent")
        << GPSType::ublox << GPSReceiverConfig{.base = VALID_SURVEY, .allowPersistentChanges = true}
        << Error::UnsupportedPersistentConfiguration;
    QTest::newRow("supported-persistent")
        << GPSType::quectel << GPSReceiverConfig{.base = VALID_SURVEY, .allowPersistentChanges = true} << Error::None;
    QTest::newRow("unicore-persistent") << GPSType::unicore
                                        << GPSReceiverConfig{.base = {.mode =
                                                                          GPSBaseStationConfig::ReceiverAveraging{1s}},
                                                             .allowPersistentChanges = true}
                                        << Error::UnsupportedPersistentConfiguration;
    QTest::newRow("receiver-averaging") << GPSType::unicore
                                        << GPSReceiverConfig{.base = {.mode =
                                                                          GPSBaseStationConfig::ReceiverAveraging{1s}}}
                                        << Error::None;
    QTest::newRow("compact-observations")
        << GPSType::ublox << GPSReceiverConfig{.base = {.mode = VALID_SURVEY.mode, .compactObservations = true}}
        << Error::None;
    for (const auto type : {GPSType::septentrio, GPSType::unicore, GPSType::quectel}) {
        QTest::addRow("unsupported-compact-observations-%d", static_cast<int>(type))
            << type << GPSReceiverConfig{.base = {.mode = VALID_FIXED.mode, .compactObservations = true}}
            << Error::UnsupportedCompactObservations;
    }
    QTest::newRow("passive-rejects-compact-observations")
        << GPSType::passive << GPSReceiverConfig{.base = {.compactObservations = true}, .baudRate = 115200}
        << Error::UnsupportedBaseMode;
    for (const auto type : {GPSType::ublox, GPSType::quectel}) {
        QTest::addRow("unsupported-receiver-averaging-%d", static_cast<int>(type))
            << type << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{1s}}}
            << Error::UnsupportedBaseMode;
    }
    for (int value : {-1, 8, 255}) {
        QTest::addRow("unknown-type-%d", value)
            << static_cast<GPSType>(value) << GPSReceiverConfig{} << Error::UnknownReceiver;
    }
}

void GPSReceiverConfigTest::_receiverValidation()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(Error, expected);
    QCOMPARE(gpsValidateReceiverConfig(type, config), expected);
}

void GPSReceiverConfigTest::_validationPrecedence_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<Error>("expected");

    QTest::newRow("unknown-receiver-precedes-invalid-base")
        << static_cast<GPSType>(-1) << GPSReceiverConfig{} << Error::UnknownReceiver;
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

void GPSReceiverConfigTest::_descriptorIdentities()
{
    for (const auto& descriptor : gpsReceiverDescriptors()) {
        QCOMPARE(gpsReceiverDescriptor(descriptor.type), &descriptor);
        QCOMPARE(gpsReceiverDescriptorForManufacturer(descriptor.manufacturerId), &descriptor);
        QVERIFY(descriptor.capabilities.recognized);
        QVERIFY(descriptor.capabilities.rtkBase || descriptor.capabilities.passive);
    }
    QVERIFY(!gpsReceiverDescriptor(static_cast<GPSType>(-1)));
    QVERIFY(!gpsReceiverDescriptorForManufacturer(-1));
    // The protocols a passive input decodes.
    QCOMPARE(gpsInputProtocolName(GPSType::ublox), QStringLiteral("u-blox (UBX)"));
    QCOMPARE(gpsInputProtocolName(GPSType::septentrio), QStringLiteral("Septentrio (SBF)"));
    QCOMPARE(gpsInputProtocolName(GPSType::passive), QStringLiteral("NMEA"));
    QVERIFY(gpsInputProtocolName(GPSType::trimble).isEmpty());

    // The settings select each base family by its manufacturer ID; passive input is a role, not a manufacturer.
    QObject owner;
    const auto settings = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/RTK.SettingsGroup.json"), &owner);
    const FactMetaData* selectable = settings.value(QStringLiteral("baseReceiverManufacturers"));
    QVERIFY(selectable);
    QList<int> offered;
    for (const QVariant& value : selectable->enumValues()) {
        offered.append(value.toInt());
    }
    QList<int> bases{GPS_AUTOMATIC_MANUFACTURER};
    for (const auto& descriptor : gpsReceiverDescriptors()) {
        if (descriptor.capabilities.rtkBase) {
            bases.append(descriptor.manufacturerId);
        }
    }
    std::ranges::sort(offered);
    std::ranges::sort(bases);
    QCOMPARE(offered, bases);
}

void GPSReceiverConfigTest::_presentation_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("specific");
    QTest::addColumn<QStringList>("trueKeys");
    QTest::addColumn<QStringList>("falseKeys");

    // Automatic offers Quectel's flash-save consent and notes, for the case that it identifies a Quectel receiver.
    QTest::newRow("automatic") << 0 << false << QStringList{"automatic",         "rtkBase",
                                                            "surveyIn",          "receiverAveraging",
                                                            "surveyAccuracy",    "surveyDuration",
                                                            "fixedBaseAccuracy", "persistentConfiguration",
                                                            "restartOnConnect",  "surveyMaySavePosition"}
                               << QStringList{"specificReceiver", "passive", "observationAccuracyFilter",
                                              "acceptedObservationTime", "reportsSurveyDuration"};
    QTest::newRow("ublox") << 4 << true << QStringList{"specificReceiver",     "rtkBase",        "surveyIn",
                                                       "surveyAccuracy",       "surveyDuration", "fixedBaseAccuracy",
                                                       "reportsSurveyDuration"}
                           << QStringList{"receiverAveraging", "passive", "persistentConfiguration", "restartOnConnect",
                                          "surveyMaySavePosition"};
    QTest::newRow("unicore") << 5 << true << QStringList{"specificReceiver", "rtkBase", "receiverAveraging"}
                             << QStringList{"surveyIn",          "passive",
                                            "surveyAccuracy",    "surveyDuration",
                                            "fixedBaseAccuracy", "persistentConfiguration",
                                            "restartOnConnect",  "surveyMaySavePosition"};
    QTest::newRow("quectel") << 6 << true
                             << QStringList{"specificReceiver",
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
    QTest::newRow("passive") << 7 << true << QStringList{"specificReceiver", "passive"}
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
        << QStringList{"specificReceiver", "automatic", "rtkBase", "surveyIn", "receiverAveraging", "passive"};
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

void GPSReceiverConfigTest::_baseDiagnostic_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::addColumn<QString>("expected");
    QTest::newRow("invalid-survey") << GPSBaseStationConfig{}
                                    << QStringLiteral("Enter a valid survey-in accuracy and duration");
    QTest::newRow("valid-survey") << GPSBaseStationConfig{.mode = GPSBaseStationConfig::SurveyIn{1, 60s}} << QString();
    QTest::newRow("invalid-fixed") << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{}}
                                   << QStringLiteral("Enter a valid fixed base position and accuracy");
    QTest::newRow("valid-fixed")
        << GPSBaseStationConfig{.mode =
                                    GPSBaseStationConfig::Fixed{
                                        .position = {.latitudeDegrees = 0, .longitudeDegrees = 0, .altitudeMeters = 0}}}
        << QString();
    QTest::newRow("fixed-unavailable-accuracy")
        << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = 47,
                                                                                 .longitudeDegrees = 8,
                                                                                 .altitudeMeters = 500},
                                                                    .accuracyMeters =
                                                                        std::numeric_limits<float>::quiet_NaN()}}
        << QStringLiteral("Enter a valid fixed base position and accuracy");
}

void GPSReceiverConfigTest::_baseDiagnostic()
{
    QFETCH(GPSBaseStationConfig, config);
    QFETCH(QString, expected);
    QCOMPARE(gpsReceiverConfigErrorText(gpsValidateBaseStationConfig(config)), expected);
}

void GPSReceiverConfigTest::_receiverDiagnostic_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<QString>("expected");
    QTest::newRow("valid") << GPSType::ublox
                           << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{1, 60s}}} << QString();
    QTest::newRow("unknown-receiver") << static_cast<GPSType>(-1) << GPSReceiverConfig{}
                                      << QStringLiteral("Unsupported GPS receiver type");
    QTest::newRow("invalid-survey") << GPSType::ublox << GPSReceiverConfig{}
                                    << QStringLiteral("Enter a valid survey-in accuracy and duration");
    QTest::newRow("invalid-fixed") << GPSType::ublox
                                   << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::Fixed{}}}
                                   << QStringLiteral("Enter a valid fixed base position and accuracy");
    QTest::newRow("unsupported-persistent-configuration")
        << GPSType::ublox
        << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{1, 60s}}, .allowPersistentChanges = true}
        << QStringLiteral("This driver does not support persistent receiver configuration");
}

void GPSReceiverConfigTest::_receiverDiagnostic()
{
    QFETCH(GPSType, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(QString, expected);
    QCOMPARE(gpsReceiverConfigError(type, config), expected);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverConfigTest, TestLabel::Unit)
