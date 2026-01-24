#pragma once

/// @file ParameterizedTest.h
/// @brief Parameterized (data-driven) test framework for Qt Test.
///
/// Provides macros and utilities for running the same test logic with
/// multiple input combinations, reducing boilerplate and improving coverage.
///
/// This integrates with QtTestExtensions.h for comprehensive test assertions.
///
/// Usage:
/// @code
/// void MyTest::testAddition_data()
/// {
///     QGC_TEST_COLUMNS(int, a, int, b, int, expected);
///     QGC_TEST_ROW("positive") << 1 << 2 << 3;
///     QGC_TEST_ROW("negative") << -1 << -2 << -3;
///     QGC_TEST_ROW("zero") << 0 << 0 << 0;
/// }
///
/// void MyTest::testAddition()
/// {
///     QGC_FETCH_DATA(int, a, int, b, int, expected);
///     QCOMPARE(a + b, expected);
/// }
/// @endcode
///
/// Advanced usage with test case generator:
/// @code
/// void MyTest::testCoordinates_data()
/// {
///     TestCases::columns<double, double, bool>("lat", "lon", "valid");
///
///     // Generate boundary test cases
///     TestCases::boundaryValues("valid_lat", -90.0, 90.0, 0.0, true);
///     TestCases::boundaryValues("valid_lon", -180.0, 180.0, 0.0, true);
///
///     // Add specific edge cases
///     TestCases::add("north_pole", 90.0, 0.0, true);
///     TestCases::add("invalid_lat", 91.0, 0.0, false);
/// }
/// @endcode

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtTest/QTest>

#include <concepts>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <vector>

// =============================================================================
// Basic Data-Driven Test Macros
// =============================================================================

/// Define test data columns (up to 8 columns)
/// Usage: QGC_TEST_COLUMNS(type1, name1, type2, name2, ...)
#define QGC_TEST_COLUMNS(...) \
    QGC_TEST_COLUMNS_IMPL(__VA_ARGS__)

#define QGC_TEST_COLUMNS_IMPL(...) \
    QGC_TEST_COLUMNS_EXPAND(QGC_TEST_COLUMNS_COUNT(__VA_ARGS__), __VA_ARGS__)

#define QGC_TEST_COLUMNS_COUNT(...) \
    QGC_TEST_COLUMNS_COUNT_IMPL(__VA_ARGS__, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0)

#define QGC_TEST_COLUMNS_COUNT_IMPL( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N

#define QGC_TEST_COLUMNS_EXPAND(N, ...) \
    QGC_TEST_COLUMNS_##N(__VA_ARGS__)

#define QGC_TEST_COLUMNS_2(t1, n1) \
    QTest::addColumn<t1>(#n1)

#define QGC_TEST_COLUMNS_4(t1, n1, t2, n2) \
    QTest::addColumn<t1>(#n1); \
    QTest::addColumn<t2>(#n2)

#define QGC_TEST_COLUMNS_6(t1, n1, t2, n2, t3, n3) \
    QTest::addColumn<t1>(#n1); \
    QTest::addColumn<t2>(#n2); \
    QTest::addColumn<t3>(#n3)

#define QGC_TEST_COLUMNS_8(t1, n1, t2, n2, t3, n3, t4, n4) \
    QTest::addColumn<t1>(#n1); \
    QTest::addColumn<t2>(#n2); \
    QTest::addColumn<t3>(#n3); \
    QTest::addColumn<t4>(#n4)

/// Add a test row with the given name
/// Usage: QGC_TEST_ROW("test_name") << value1 << value2 << ...;
#define QGC_TEST_ROW(name) \
    QTest::newRow(name)

/// Fetch test data in the test function (up to 8 values)
/// Usage: QGC_FETCH_DATA(type1, name1, type2, name2, ...)
#define QGC_FETCH_DATA(...) \
    QGC_FETCH_DATA_IMPL(__VA_ARGS__)

#define QGC_FETCH_DATA_IMPL(...) \
    QGC_FETCH_DATA_EXPAND(QGC_TEST_COLUMNS_COUNT(__VA_ARGS__), __VA_ARGS__)

#define QGC_FETCH_DATA_EXPAND(N, ...) \
    QGC_FETCH_DATA_##N(__VA_ARGS__)

#define QGC_FETCH_DATA_2(t1, n1) \
    QFETCH(t1, n1)

#define QGC_FETCH_DATA_4(t1, n1, t2, n2) \
    QFETCH(t1, n1); \
    QFETCH(t2, n2)

#define QGC_FETCH_DATA_6(t1, n1, t2, n2, t3, n3) \
    QFETCH(t1, n1); \
    QFETCH(t2, n2); \
    QFETCH(t3, n3)

#define QGC_FETCH_DATA_8(t1, n1, t2, n2, t3, n3, t4, n4) \
    QFETCH(t1, n1); \
    QFETCH(t2, n2); \
    QFETCH(t3, n3); \
    QFETCH(t4, n4)

// =============================================================================
// Test Case Generator
// =============================================================================

/// Namespace for test case generation utilities
namespace TestCases {

namespace detail {
    template<typename T>
    void addColumn(const char* name) {
        QTest::addColumn<T>(name);
    }

    template<std::size_t I, typename... Types>
    void addColumnsHelper([[maybe_unused]] const std::array<const char*, sizeof...(Types)>& names) {
        if constexpr (I < sizeof...(Types)) {
            addColumn<std::tuple_element_t<I, std::tuple<Types...>>>(names[I]);
            addColumnsHelper<I + 1, Types...>(names);
        }
    }
}

/// Add columns for parameterized tests
/// Usage: columns<int, QString, bool>("count", "name", "valid");
template<typename... Types>
void columns(const char* const (&names)[sizeof...(Types)]) {
    std::array<const char*, sizeof...(Types)> nameArray;
    for (std::size_t i = 0; i < sizeof...(Types); ++i) {
        nameArray[i] = names[i];
    }
    detail::addColumnsHelper<0, Types...>(nameArray);
}

/// Add a test row with values
template<typename... Args>
void add(const char* name, Args&&... args) {
    auto row = QTest::newRow(name);
    ()std::initializer_list<int>{(row << std::forward<Args>(args), 0)...};
}

/// Generate boundary value test cases for a numeric range
template<typename T>
void boundaryValues(const char* prefix, T minVal, T maxVal, T midVal, bool validExpected) {
    QString name;

    // Minimum boundary
    name = QStringLiteral("%1_min").arg(prefix);
    QTest::newRow(qPrintable(name)) << minVal << validExpected;

    // Just above minimum
    if constexpr (std::is_floating_point_v<T>) {
        name = QStringLiteral("%1_min_plus_epsilon").arg(prefix);
        QTest::newRow(qPrintable(name)) << (minVal + std::numeric_limits<T>::epsilon()) << validExpected;
    } else {
        name = QStringLiteral("%1_min_plus_1").arg(prefix);
        QTest::newRow(qPrintable(name)) << static_cast<T>(minVal + 1) << validExpected;
    }

    // Middle value
    name = QStringLiteral("%1_mid").arg(prefix);
    QTest::newRow(qPrintable(name)) << midVal << validExpected;

    // Just below maximum
    if constexpr (std::is_floating_point_v<T>) {
        name = QStringLiteral("%1_max_minus_epsilon").arg(prefix);
        QTest::newRow(qPrintable(name)) << (maxVal - std::numeric_limits<T>::epsilon()) << validExpected;
    } else {
        name = QStringLiteral("%1_max_minus_1").arg(prefix);
        QTest::newRow(qPrintable(name)) << static_cast<T>(maxVal - 1) << validExpected;
    }

    // Maximum boundary
    name = QStringLiteral("%1_max").arg(prefix);
    QTest::newRow(qPrintable(name)) << maxVal << validExpected;
}

/// Generate invalid boundary test cases (just outside valid range)
template<typename T>
void invalidBoundaries(const char* prefix, T minVal, T maxVal) {
    QString name;

    // Just below minimum
    if constexpr (std::is_floating_point_v<T>) {
        name = QStringLiteral("%1_below_min").arg(prefix);
        QTest::newRow(qPrintable(name)) << (minVal - std::numeric_limits<T>::epsilon() * 10) << false;
    } else {
        if (minVal > std::numeric_limits<T>::min()) {
            name = QStringLiteral("%1_below_min").arg(prefix);
            QTest::newRow(qPrintable(name)) << static_cast<T>(minVal - 1) << false;
        }
    }

    // Just above maximum
    if constexpr (std::is_floating_point_v<T>) {
        name = QStringLiteral("%1_above_max").arg(prefix);
        QTest::newRow(qPrintable(name)) << (maxVal + std::numeric_limits<T>::epsilon() * 10) << false;
    } else {
        if (maxVal < std::numeric_limits<T>::max()) {
            name = QStringLiteral("%1_above_max").arg(prefix);
            QTest::newRow(qPrintable(name)) << static_cast<T>(maxVal + 1) << false;
        }
    }
}

/// Generate coordinate test cases for latitude/longitude validation
inline void coordinateTestCases() {
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<bool>("valid");

    // Valid coordinates
    QTest::newRow("origin") << 0.0 << 0.0 << true;
    QTest::newRow("north_pole") << 90.0 << 0.0 << true;
    QTest::newRow("south_pole") << -90.0 << 0.0 << true;
    QTest::newRow("date_line_east") << 0.0 << 180.0 << true;
    QTest::newRow("date_line_west") << 0.0 << -180.0 << true;
    QTest::newRow("san_francisco") << 37.7749 << -122.4194 << true;
    QTest::newRow("sydney") << -33.8688 << 151.2093 << true;
    QTest::newRow("tokyo") << 35.6762 << 139.6503 << true;

    // Invalid coordinates
    QTest::newRow("lat_too_high") << 90.1 << 0.0 << false;
    QTest::newRow("lat_too_low") << -90.1 << 0.0 << false;
    QTest::newRow("lon_too_high") << 0.0 << 180.1 << false;
    QTest::newRow("lon_too_low") << 0.0 << -180.1 << false;
    QTest::newRow("nan_lat") << std::numeric_limits<double>::quiet_NaN() << 0.0 << false;
    QTest::newRow("nan_lon") << 0.0 << std::numeric_limits<double>::quiet_NaN() << false;
    QTest::newRow("inf_lat") << std::numeric_limits<double>::infinity() << 0.0 << false;
    QTest::newRow("inf_lon") << 0.0 << std::numeric_limits<double>::infinity() << false;
}

/// Generate altitude test cases
inline void altitudeTestCases() {
    QTest::addColumn<double>("altitude");
    QTest::addColumn<bool>("valid");

    // Valid altitudes (meters)
    QTest::newRow("ground_level") << 0.0 << true;
    QTest::newRow("typical_flight") << 100.0 << true;
    QTest::newRow("high_altitude") << 10000.0 << true;
    QTest::newRow("dead_sea") << -430.0 << true;  // Below sea level
    QTest::newRow("everest") << 8848.0 << true;

    // Edge cases
    QTest::newRow("negative_reasonable") << -500.0 << true;
    QTest::newRow("very_high") << 50000.0 << true;  // Near space

    // Invalid
    QTest::newRow("nan") << std::numeric_limits<double>::quiet_NaN() << false;
    QTest::newRow("inf") << std::numeric_limits<double>::infinity() << false;
    QTest::newRow("neg_inf") << -std::numeric_limits<double>::infinity() << false;
}

/// Generate speed test cases
inline void speedTestCases() {
    QTest::addColumn<double>("speed");
    QTest::addColumn<bool>("valid");

    // Valid speeds (m/s)
    QTest::newRow("stationary") << 0.0 << true;
    QTest::newRow("walking") << 1.4 << true;
    QTest::newRow("car") << 30.0 << true;
    QTest::newRow("plane") << 250.0 << true;
    QTest::newRow("fast_jet") << 600.0 << true;

    // Invalid
    QTest::newRow("negative") << -1.0 << false;
    QTest::newRow("nan") << std::numeric_limits<double>::quiet_NaN() << false;
    QTest::newRow("inf") << std::numeric_limits<double>::infinity() << false;
}

/// Generate heading/bearing test cases
inline void headingTestCases() {
    QTest::addColumn<double>("heading");
    QTest::addColumn<double>("normalized");  // Expected normalized value

    // Standard headings
    QTest::newRow("north") << 0.0 << 0.0;
    QTest::newRow("east") << 90.0 << 90.0;
    QTest::newRow("south") << 180.0 << 180.0;
    QTest::newRow("west") << 270.0 << 270.0;
    QTest::newRow("almost_360") << 359.9 << 359.9;

    // Normalization cases
    QTest::newRow("360_to_0") << 360.0 << 0.0;
    QTest::newRow("450_to_90") << 450.0 << 90.0;
    QTest::newRow("720_to_0") << 720.0 << 0.0;
    QTest::newRow("neg_90_to_270") << -90.0 << 270.0;
    QTest::newRow("neg_180_to_180") << -180.0 << 180.0;
    QTest::newRow("neg_270_to_90") << -270.0 << 90.0;
}

} // namespace TestCases

// =============================================================================
// Parameterized Test Class Template
// =============================================================================

/// Base class for parameterized tests with automatic data generation
/// Usage:
/// @code
/// class MyParamTest : public ParameterizedTest<int, int, int> {
///     void generateTestData() override {
///         addTestCase("case1", 1, 2, 3);
///         addTestCase("case2", -1, -2, -3);
///     }
///
///     void runTest(int a, int b, int expected) override {
///         QCOMPARE(a + b, expected);
///     }
/// };
/// @endcode
template<typename... Params>
class ParameterizedTest {
public:
    using TestCase = std::tuple<QString, Params...>;

    virtual ~ParameterizedTest() = default;

    /// Add a test case with name and parameters
    void addTestCase(const QString& name, Params... params) {
        _testCases.push_back(std::make_tuple(name, params...));
    }

    /// Run all test cases
    void runAllTests() {
        generateTestData();

        for (const auto& testCase : _testCases) {
            QString name = std::get<0>(testCase);
            qDebug() << "Running test case:" << name;

            try {
                std::apply([this](const QString&, Params... params) {
                    runTest(params...);
                }, testCase);
            } catch (const std::exception& e) {
                QFAIL(qPrintable(QStringLiteral("Test case '%1' failed: %2").arg(name, e.what())));
            }
        }
    }

    /// Get the number of test cases
    size_t testCaseCount() const { return _testCases.size(); }

protected:
    /// Override to generate test data
    virtual void generateTestData() = 0;

    /// Override to run a single test case
    virtual void runTest(Params... params) = 0;

private:
    std::vector<TestCase> _testCases;
};

// =============================================================================
// Assertion Helpers for Parameterized Tests
// =============================================================================

/// Compare with context message showing the test parameters
#define QCOMPARE_PARAM(actual, expected, context) \
    do { \
        if ((actual) != (expected)) { \
            QString msg = QStringLiteral("Comparison failed [%1]: %2 != %3") \
                .arg(context) \
                .arg(actual) \
                .arg(expected); \
            QFAIL(qPrintable(msg)); \
        } \
    } while (false)

/// Verify with context message
#define QVERIFY_PARAM(condition, context) \
    do { \
        if (!(condition)) { \
            QString msg = QStringLiteral("Verification failed [%1]: %2") \
                .arg(context, #condition); \
            QFAIL(qPrintable(msg)); \
        } \
    } while (false)

/// Compare floating point with tolerance and context
#define QCOMPARE_DOUBLE_PARAM(actual, expected, tolerance, context) \
    do { \
        double _diff = std::abs((actual) - (expected)); \
        if (_diff > (tolerance)) { \
            QString msg = QStringLiteral("Float comparison failed [%1]: %2 != %3 (diff: %4, tolerance: %5)") \
                .arg(context) \
                .arg(actual) \
                .arg(expected) \
                .arg(_diff) \
                .arg(tolerance); \
            QFAIL(qPrintable(msg)); \
        } \
    } while (false)
