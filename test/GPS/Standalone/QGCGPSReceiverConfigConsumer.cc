#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <type_traits>

#include "GPSBaseStationConfig.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

#if defined(QT_CORE_LIB) || defined(QT_VERSION)
#error The receiver configuration consumer must not inherit Qt dependencies.
#endif

namespace {
using Error = GPSReceiverConfigError;
using Role = GPSReceiverConfig::Role;

std::string_view errorName(Error error)
{
    switch (error) {
        case Error::None:
            return "None";
        case Error::UnknownReceiver:
            return "UnknownReceiver";
        case Error::InvalidRole:
            return "InvalidRole";
        case Error::InvalidSurveyIn:
            return "InvalidSurveyIn";
        case Error::InvalidFixedBase:
            return "InvalidFixedBase";
        case Error::UnsupportedConstellations:
            return "UnsupportedConstellations";
        case Error::InvalidConstellations:
            return "InvalidConstellations";
        case Error::UnsupportedDynamicModel:
            return "UnsupportedDynamicModel";
        case Error::InvalidDynamicModel:
            return "InvalidDynamicModel";
        case Error::UnsupportedHeadingOffset:
            return "UnsupportedHeadingOffset";
        case Error::InvalidHeadingOffset:
            return "InvalidHeadingOffset";
    }
    return "Unknown error";
}

template <typename T>
struct ValueCase
{
    std::string_view name;
    T value;
    Error expected;
};
}  // namespace

int main()
{
    int failures = 0;
    const auto check = [&failures](std::string_view name, bool condition) {
        if (!condition) {
            std::cerr << name << ": failed\n";
            ++failures;
        }
    };
    const auto expectError = [&failures](std::string_view name, Error actual, Error expected) {
        if (actual != expected) {
            std::cerr << name << ": expected " << errorName(expected) << ", got " << errorName(actual) << " ("
                      << static_cast<int>(actual) << ")\n";
            ++failures;
        }
    };
    const auto checkBaseField = [&expectError](std::string_view field, auto member, const auto& cases,
                                               const GPSBaseStationConfig& initial) {
        for (const auto& entry : cases) {
            GPSBaseStationConfig base = initial;
            base.*member = entry.value;
            expectError(std::string(field) + ": " + std::string(entry.name), gpsValidateBaseStationConfig(base),
                        entry.expected);
        }
    };
    const auto emptyCapabilities = [](const GPSReceiverCapabilities& capabilities) {
        return !capabilities.recognized && !capabilities.position && !capabilities.rtkBase &&
               capabilities.constellationMask == 0 && !capabilities.dynamicModel && !capabilities.headingOffset;
    };

    constexpr double NAN_DOUBLE = std::numeric_limits<double>::quiet_NaN();
    constexpr double INF_DOUBLE = std::numeric_limits<double>::infinity();
    constexpr float NAN_FLOAT = std::numeric_limits<float>::quiet_NaN();
    constexpr float INF_FLOAT = std::numeric_limits<float>::infinity();
    constexpr int64_t MAX_DURATION = (std::numeric_limits<uint32_t>::max)();
    constexpr double MAX_SURVEY_ACCURACY = static_cast<double>(MAX_DURATION) / 10000.0;
    constexpr float PI = std::numbers::pi_v<float>;
    constexpr GPSType TYPES[] = {GPSType::ublox, GPSType::trimble, GPSType::septentrio, GPSType::femto};
    constexpr Role ROLES[] = {Role::RTKBase, Role::Position};
    const GPSBaseStationConfig validSurvey{.surveyInAccMeters = 0.0001, .surveyInDurationSecs = 1};
    const GPSBaseStationConfig validFixed{
        .useFixedBase = true, .fixedBaseLatitude = 0.0, .fixedBaseLongitude = 0.0, .fixedBaseAltitudeMeters = 0.0f};

    const GPSReceiverConfig defaults;
    check("configuration remains an aggregate", std::is_aggregate_v<GPSReceiverConfig>);
    check("default role is RTK base", defaults.role == Role::RTKBase);
    check("default constellations retain receiver defaults", defaults.constellationMask == 0);
    check("default dynamic model is absent", !defaults.dynamicModel.has_value());
    check("default heading offset is absent", !defaults.headingOffsetRadians.has_value());
    check("default base uses survey-in", !defaults.base.useFixedBase);
    check("default survey settings require configuration",
          defaults.base.surveyInAccMeters == 0.0 && defaults.base.surveyInDurationSecs == 0);
    check("default fixed base requires explicit coordinates and ellipsoid altitude",
          std::isnan(defaults.base.fixedBaseLatitude) && std::isnan(defaults.base.fixedBaseLongitude) &&
              std::isnan(defaults.base.fixedBaseAltitudeMeters));
    check("default fixed accuracy permits zero", defaults.base.fixedBaseAccuracyMeters == 0.0f);
    check("default capabilities are closed", emptyCapabilities({}));
    expectError("default survey is invalid", gpsValidateBaseStationConfig(defaults.base), Error::InvalidSurveyIn);
    expectError("default receiver base is invalid", gpsValidateReceiverConfig(GPSType::ublox, defaults),
                Error::InvalidSurveyIn);
    expectError("unset fixed position is invalid", gpsValidateBaseStationConfig({.useFixedBase = true}),
                Error::InvalidFixedBase);
    expectError("minimum survey ignores unused fixed coordinates", gpsValidateBaseStationConfig(validSurvey),
                Error::None);
    expectError("explicit zero fixed position and accuracy ignore unused survey",
                gpsValidateBaseStationConfig(validFixed), Error::None);

    const ValueCase<double> surveyAccuracyCases[] = {
        {"negative", -1.0, Error::InvalidSurveyIn},
        {"zero", 0.0, Error::InvalidSurveyIn},
        {"below one wire unit", std::nextafter(0.0001, 0.0), Error::InvalidSurveyIn},
        {"one wire unit", 0.0001, Error::None},
        {"maximum wire value", MAX_SURVEY_ACCURACY, Error::None},
        {"above maximum", std::nextafter(MAX_SURVEY_ACCURACY, INF_DOUBLE), Error::InvalidSurveyIn},
        {"NaN", NAN_DOUBLE, Error::InvalidSurveyIn},
        {"positive infinity", INF_DOUBLE, Error::InvalidSurveyIn},
        {"negative infinity", -INF_DOUBLE, Error::InvalidSurveyIn},
        {"conversion overflow", (std::numeric_limits<double>::max)(), Error::InvalidSurveyIn},
    };
    checkBaseField("survey accuracy", &GPSBaseStationConfig::surveyInAccMeters, surveyAccuracyCases, validSurvey);
    const ValueCase<int64_t> durationCases[] = {
        {"negative", -1, Error::InvalidSurveyIn},
        {"zero", 0, Error::InvalidSurveyIn},
        {"minimum", 1, Error::None},
        {"uint32 maximum", MAX_DURATION, Error::None},
        {"above uint32 maximum", MAX_DURATION + 1, Error::InvalidSurveyIn},
        {"int64 minimum", (std::numeric_limits<int64_t>::min)(), Error::InvalidSurveyIn},
        {"int64 maximum", (std::numeric_limits<int64_t>::max)(), Error::InvalidSurveyIn},
    };
    checkBaseField("survey duration", &GPSBaseStationConfig::surveyInDurationSecs, durationCases, validSurvey);
    const ValueCase<double> latitudeCases[] = {
        {"below south pole", std::nextafter(-90.0, -INF_DOUBLE), Error::InvalidFixedBase},
        {"south pole", -90.0, Error::None},
        {"north pole", 90.0, Error::None},
        {"above north pole", std::nextafter(90.0, INF_DOUBLE), Error::InvalidFixedBase},
        {"missing", NAN_DOUBLE, Error::InvalidFixedBase},
        {"positive infinity", INF_DOUBLE, Error::InvalidFixedBase},
        {"negative infinity", -INF_DOUBLE, Error::InvalidFixedBase},
    };
    checkBaseField("fixed latitude", &GPSBaseStationConfig::fixedBaseLatitude, latitudeCases, validFixed);
    const ValueCase<double> longitudeCases[] = {
        {"below west boundary", std::nextafter(-180.0, -INF_DOUBLE), Error::InvalidFixedBase},
        {"west boundary", -180.0, Error::None},
        {"east boundary", 180.0, Error::None},
        {"above east boundary", std::nextafter(180.0, INF_DOUBLE), Error::InvalidFixedBase},
        {"missing", NAN_DOUBLE, Error::InvalidFixedBase},
        {"positive infinity", INF_DOUBLE, Error::InvalidFixedBase},
        {"negative infinity", -INF_DOUBLE, Error::InvalidFixedBase},
    };
    checkBaseField("fixed longitude", &GPSBaseStationConfig::fixedBaseLongitude, longitudeCases, validFixed);
    const ValueCase<float> altitudeCases[] = {
        {"minimum representable wire-safe altitude", -21474836.0f, Error::None},
        {"below minimum", std::nextafter(-21474836.0f, -INF_FLOAT), Error::InvalidFixedBase},
        {"maximum representable wire-safe altitude", 21474836.0f, Error::None},
        {"above maximum", std::nextafter(21474836.0f, INF_FLOAT), Error::InvalidFixedBase},
        {"missing ellipsoid altitude", NAN_FLOAT, Error::InvalidFixedBase},
        {"positive infinity", INF_FLOAT, Error::InvalidFixedBase},
        {"negative infinity", -INF_FLOAT, Error::InvalidFixedBase},
    };
    checkBaseField("fixed altitude", &GPSBaseStationConfig::fixedBaseAltitudeMeters, altitudeCases, validFixed);
    const ValueCase<float> fixedAccuracyCases[] = {
        {"negative", -0.0001f, Error::InvalidFixedBase},
        {"negative zero", -0.0f, Error::None},
        {"zero", 0.0f, Error::None},
        {"below one wire unit", 0.00001f, Error::None},
        // A single multiplication by 10000.f incorrectly rejects this valid legacy boundary.
        {"two-stage float conversion boundary", 429496.71875f, Error::None},
        {"above maximum", std::nextafter(429496.71875f, INF_FLOAT), Error::InvalidFixedBase},
        {"NaN", NAN_FLOAT, Error::InvalidFixedBase},
        {"positive infinity", INF_FLOAT, Error::InvalidFixedBase},
        {"negative infinity", -INF_FLOAT, Error::InvalidFixedBase},
        {"conversion overflow", (std::numeric_limits<float>::max)(), Error::InvalidFixedBase},
    };
    checkBaseField("fixed accuracy", &GPSBaseStationConfig::fixedBaseAccuracyMeters, fixedAccuracyCases, validFixed);

    const ValueCase<int> dynamicCases[] = {
        {"int minimum", (std::numeric_limits<int>::min)(), Error::InvalidDynamicModel},
        {"negative", -1, Error::InvalidDynamicModel},
        {"portable", 0, Error::None},
        {"reserved", 1, Error::InvalidDynamicModel},
        {"stationary", 2, Error::None},
        {"pedestrian", 3, Error::None},
        {"automotive", 4, Error::None},
        {"sea", 5, Error::None},
        {"airborne 1g", 6, Error::None},
        {"airborne 2g", 7, Error::None},
        {"airborne 4g", 8, Error::None},
        {"unsupported newer model", 9, Error::InvalidDynamicModel},
        {"byte maximum", 255, Error::InvalidDynamicModel},
        {"must not narrow to portable", 256, Error::InvalidDynamicModel},
        {"int maximum", (std::numeric_limits<int>::max)(), Error::InvalidDynamicModel},
    };
    const ValueCase<float> headingCases[] = {
        {"explicit zero", 0.0f, Error::None},
        {"negative zero", -0.0f, Error::None},
        {"positive", 0.5f, Error::None},
        {"negative", -0.5f, Error::None},
        {"minimum", -PI, Error::None},
        {"maximum", PI, Error::None},
        {"below minimum", std::nextafter(-PI, -INF_FLOAT), Error::InvalidHeadingOffset},
        {"above maximum", std::nextafter(PI, INF_FLOAT), Error::InvalidHeadingOffset},
        {"NaN", NAN_FLOAT, Error::InvalidHeadingOffset},
        {"positive infinity", INF_FLOAT, Error::InvalidHeadingOffset},
        {"negative infinity", -INF_FLOAT, Error::InvalidHeadingOffset},
    };
    constexpr uint32_t INVALID_MASKS[] = {32u, 33u, 64u, 0x80000000u, 0xffffffffu};
    for (GPSType type : TYPES) {
        for (Role role : ROLES) {
            const std::string label =
                "type " + std::to_string(static_cast<int>(type)) + ", role " + std::to_string(static_cast<int>(role));
            const auto capabilities = gpsReceiverCapabilities(type, role);
            const bool ublox = type == GPSType::ublox;
            const bool position = role == Role::Position;
            check(label + ": recognized", capabilities.recognized);
            check(label + ": position path", capabilities.position);
            check(label + ": base path", capabilities.rtkBase);
            check(label + ": constellation mask", capabilities.constellationMask == (ublox ? 0x1fu : 0u));
            check(label + ": dynamic model", capabilities.dynamicModel == (ublox && position));
            check(label + ": heading offset", capabilities.headingOffset == position);
            GPSReceiverConfig config{.role = role, .base = validSurvey};
            expectError(label + ": defaults", gpsValidateReceiverConfig(type, config), Error::None);
            config.base = validFixed;
            expectError(label + ": valid fixed base", gpsValidateReceiverConfig(type, config), Error::None);
            config.base = {};
            expectError(label + ": unconfigured survey", gpsValidateReceiverConfig(type, config),
                        position ? Error::None : Error::InvalidSurveyIn);
            config.base.useFixedBase = true;
            expectError(label + ": unconfigured fixed base", gpsValidateReceiverConfig(type, config),
                        position ? Error::None : Error::InvalidFixedBase);
            config.base = validSurvey;
            for (uint32_t mask = 0; mask <= 31; ++mask) {
                config.constellationMask = mask;
                expectError(label + ": constellation mask " + std::to_string(mask),
                            gpsValidateReceiverConfig(type, config),
                            ublox || mask == 0 ? Error::None : Error::UnsupportedConstellations);
            }
            for (uint32_t mask : INVALID_MASKS) {
                config.constellationMask = mask;
                expectError(label + ": invalid constellation mask " + std::to_string(mask),
                            gpsValidateReceiverConfig(type, config),
                            ublox ? Error::InvalidConstellations : Error::UnsupportedConstellations);
            }
            config.constellationMask = 0;
            for (const auto& entry : dynamicCases) {
                config.dynamicModel = entry.value;
                expectError(label + ": dynamic " + std::string(entry.name), gpsValidateReceiverConfig(type, config),
                            ublox && position ? entry.expected : Error::UnsupportedDynamicModel);
            }
            config.dynamicModel.reset();
            for (const auto& entry : headingCases) {
                config.headingOffsetRadians = entry.value;
                expectError(label + ": heading " + std::string(entry.name), gpsValidateReceiverConfig(type, config),
                            position ? entry.expected : Error::UnsupportedHeadingOffset);
            }
            config.headingOffsetRadians.reset();
            expectError(label + ": cleared optional requests", gpsValidateReceiverConfig(type, config), Error::None);
        }
    }

    for (int value : {-1, 4, 255}) {
        const auto type = static_cast<GPSType>(value);
        for (Role role : ROLES) {
            const std::string label = "unknown receiver " + std::to_string(value);
            check(label + ": no capabilities", emptyCapabilities(gpsReceiverCapabilities(type, role)));
            expectError(label + ": receiver precedes base validation", gpsValidateReceiverConfig(type, {.role = role}),
                        Error::UnknownReceiver);
        }
    }
    for (int value : {-1, 2, 255}) {
        const auto role = static_cast<Role>(value);
        for (GPSType type : TYPES) {
            const std::string label = "unknown role " + std::to_string(value);
            check(label + ": no capabilities", emptyCapabilities(gpsReceiverCapabilities(type, role)));
            expectError(label + ": rejected", gpsValidateReceiverConfig(type, {.role = role}), Error::InvalidRole);
        }
        check("unknown role and family have no capabilities",
              emptyCapabilities(gpsReceiverCapabilities(static_cast<GPSType>(-1), role)));
        expectError("role precedes receiver validation",
                    gpsValidateReceiverConfig(static_cast<GPSType>(-1), {.role = role}), Error::InvalidRole);
    }

    GPSReceiverConfig explicitZero{
        .role = Role::Position, .constellationMask = 31, .dynamicModel = 0, .headingOffsetRadians = 0.0f};
    check("explicit portable request is retained",
          explicitZero.dynamicModel.has_value() && *explicitZero.dynamicModel == 0);
    check("explicit zero heading is retained",
          explicitZero.headingOffsetRadians.has_value() && *explicitZero.headingOffsetRadians == 0.0f);
    expectError("all supported requests together", gpsValidateReceiverConfig(GPSType::ublox, explicitZero),
                Error::None);

    GPSReceiverConfig invalid{.constellationMask = 32, .dynamicModel = 1, .headingOffsetRadians = NAN_FLOAT};
    expectError("base precedes optional requests", gpsValidateReceiverConfig(GPSType::ublox, invalid),
                Error::InvalidSurveyIn);
    invalid.role = Role::Position;
    expectError("constellations precede dynamic model", gpsValidateReceiverConfig(GPSType::ublox, invalid),
                Error::InvalidConstellations);
    invalid.constellationMask = 0;
    expectError("dynamic model precedes heading", gpsValidateReceiverConfig(GPSType::ublox, invalid),
                Error::InvalidDynamicModel);
    invalid.dynamicModel.reset();
    expectError("heading checked after earlier requests pass", gpsValidateReceiverConfig(GPSType::ublox, invalid),
                Error::InvalidHeadingOffset);

    if (failures != 0) {
        std::cerr << failures << " receiver configuration checks failed\n";
        return 1;
    }
    return 0;
}
