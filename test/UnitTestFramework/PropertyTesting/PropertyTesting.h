#pragma once

/// @file PropertyTesting.h
/// @brief RapidCheck property-based testing support for QGroundControl unit tests
///
/// RapidCheck generates random test inputs to verify properties/invariants
/// rather than testing specific cases. This helps find edge cases that
/// manual test cases might miss.
///
/// @see https://github.com/emil-e/rapidcheck
///
/// ## Basic Usage
///
/// ```cpp
/// #include "PropertyTesting.h"
///
/// void MyTest::_testDistanceProperties()
/// {
///     // Test that distance is always non-negative
///     RC_QT_PROP("distance >= 0", []{
///         const auto lat = *rc::gen::inRange(-90.0, 90.0);
///         const auto lon = *rc::gen::inRange(-180.0, 180.0);
///         QGeoCoordinate c1(lat, lon);
///         QGeoCoordinate c2(0, 0);
///         RC_ASSERT(c1.distanceTo(c2) >= 0.0);
///     });
/// }
/// ```
///
/// ## Generators
///
/// ```cpp
/// // Built-in generators
/// const auto i = *rc::gen::inRange(0, 100);        // int in [0, 100)
/// const auto d = *rc::gen::inRange(-1.0, 1.0);     // double in [-1, 1)
/// const auto b = *rc::gen::arbitrary<bool>();      // random bool
/// const auto s = *rc::gen::arbitrary<std::string>(); // random string
///
/// // Containers
/// const auto vec = *rc::gen::container<std::vector<int>>(rc::gen::inRange(0, 10));
///
/// // Custom generators
/// auto coordGen = rc::gen::map(
///     rc::gen::tuple(rc::gen::inRange(-90.0, 90.0), rc::gen::inRange(-180.0, 180.0)),
///     [](std::tuple<double, double> t) {
///         return QGeoCoordinate(std::get<0>(t), std::get<1>(t));
///     });
/// ```
///
/// ## Assertions
///
/// ```cpp
/// RC_ASSERT(condition);              // Fail if false
/// RC_ASSERT_FALSE(condition);        // Fail if true
/// RC_ASSERT_THROWS(expr);            // Must throw
/// RC_ASSERT_THROWS_AS(expr, type);   // Must throw specific type
/// RC_FAIL("message");                // Unconditional fail
/// RC_SUCCEED_IF(condition);          // Pass early if true
/// RC_DISCARD();                       // Discard this test case (regenerate)
/// RC_CLASSIFY(condition, "label");   // Classify test cases for statistics
/// RC_TAG("label");                   // Tag unconditionally
/// ```
///
/// ## Good Properties to Test
///
/// 1. **Roundtrip**: serialize(deserialize(x)) == x
/// 2. **Symmetry**: distance(a, b) == distance(b, a)
/// 3. **Idempotence**: f(f(x)) == f(x)
/// 4. **Invariants**: size >= 0, coordinates in valid range
/// 5. **Commutativity**: f(a, b) == f(b, a)
/// 6. **Associativity**: f(f(a, b), c) == f(a, f(b, c))

#include <QtTest/QTest>

#include <rapidcheck.h>
#include <rapidcheck/detail/Results.h>

/// Run a RapidCheck property test integrated with Qt Test.
/// On failure, reports the failing case through QFAIL.
///
/// @param name Description of the property being tested
/// @param testable Lambda or callable that uses RC_ASSERT macros
///
/// Example:
/// @code
/// RC_QT_PROP("addition is commutative", []{
///     const auto a = *rc::gen::arbitrary<int>();
///     const auto b = *rc::gen::arbitrary<int>();
///     RC_ASSERT(a + b == b + a);
/// });
/// @endcode
#define RC_QT_PROP(name, ...)                                                               \
    do {                                                                                    \
        const auto _rc_result = rc::detail::checkTestable(__VA_ARGS__);                     \
        if (_rc_result.template is<rc::detail::FailureResult>()) {                          \
            const auto& _rc_fail = _rc_result.template get<rc::detail::FailureResult>();    \
            const QString _rc_msg = QStringLiteral("Property '%1' failed:\n%2")             \
                                        .arg(QString::fromUtf8(name))                       \
                                        .arg(QString::fromStdString(_rc_fail.description)); \
            QFAIL(qPrintable(_rc_msg));                                                     \
        } else if (_rc_result.template is<rc::detail::GaveUpResult>()) {                    \
            const auto& _rc_gave = _rc_result.template get<rc::detail::GaveUpResult>();     \
            const QString _rc_msg = QStringLiteral("Property '%1' gave up: %2")             \
                                        .arg(QString::fromUtf8(name))                       \
                                        .arg(QString::fromStdString(_rc_gave.description)); \
            QFAIL(qPrintable(_rc_msg));                                                     \
        } else if (_rc_result.template is<rc::detail::Error>()) {                           \
            const auto& _rc_err = _rc_result.template get<rc::detail::Error>();             \
            const QString _rc_msg = QStringLiteral("Property '%1' error: %2")               \
                                        .arg(QString::fromUtf8(name))                       \
                                        .arg(QString::fromStdString(_rc_err.description));  \
            QFAIL(qPrintable(_rc_msg));                                                     \
        }                                                                                   \
    } while (false)

/// Namespace for QGC-specific RapidCheck generators
namespace rc::qgc {

/// Generator for valid latitude values [-90, 90]
inline auto latitude()
{
    return rc::gen::map(rc::gen::inRange(-9000, 9001), [](int i) { return static_cast<double>(i) / 100.0; });
}

/// Generator for valid longitude values [-180, 180]
inline auto longitude()
{
    return rc::gen::map(rc::gen::inRange(-18000, 18001), [](int i) { return static_cast<double>(i) / 100.0; });
}

/// Generator for valid altitude values (reasonable range for aircraft)
inline auto altitude()
{
    return rc::gen::map(rc::gen::inRange(-100, 50000), [](int i) { return static_cast<double>(i); });
}

}  // namespace rc::qgc
