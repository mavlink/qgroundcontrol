#pragma once

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonParseError>
#include <QtCore/QRandomGenerator>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <cmath>
#include <concepts>
#include <functional>
#include <span>
#include <type_traits>

/// @file
/// @brief Helper utilities for QGC unit tests
///
/// This file provides utilities that complement QtTestExtensions.h:
/// - Wait utilities for asynchronous testing
/// - Verification helpers with descriptive messages
/// - Numeric and JSON comparison utilities
/// - Performance benchmarking helpers
/// - File and directory verification
/// - Coordinate/geo utilities
/// - Property-based testing utilities

namespace TestHelpers {

// ============================================================================
// Timeout Constants
// ============================================================================

inline constexpr int kDefaultTimeoutMs = 5000;
inline constexpr int kShortTimeoutMs = 1000;
inline constexpr int kLongTimeoutMs = 30000;
inline constexpr int kDefaultPollIntervalMs = 50;

// ============================================================================
// C++20 Concepts for Type Constraints
// ============================================================================

/// Concept for types that can be compared for equality
template<typename T>
concept Equatable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

/// Concept for numeric types
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

/// Concept for callable returning bool (predicates)
template<typename F>
concept Predicate = std::is_invocable_r_v<bool, F>;

// ============================================================================
// Wait Utilities
// ============================================================================

/// Waits for a condition to become true with a timeout
/// Preferred over QTest::qWait() as it returns immediately when condition is met
/// @param condition Lambda returning bool when condition is met
/// @param timeoutMs Maximum time to wait in milliseconds
/// @param pollIntervalMs How often to check the condition
/// @return true if condition met within timeout, false otherwise
template<Predicate F>
inline bool waitFor(F&& condition, int timeoutMs = kDefaultTimeoutMs, int pollIntervalMs = kDefaultPollIntervalMs)
{
    QElapsedTimer timer;
    timer.start();

    while (!std::invoke(std::forward<F>(condition))) {
        if (timer.elapsed() > timeoutMs) {
            return false;
        }
        QTest::qWait(pollIntervalMs);
    }

    return true;
}

/// Waits for a value to equal expected value
/// @param getValue Lambda returning current value
/// @param expected Expected value
/// @param timeoutMs Maximum time to wait
/// @return true if value equals expected within timeout
template<typename T>
inline bool waitForValue(std::function<T()> getValue, const T& expected, int timeoutMs = kDefaultTimeoutMs)
{
    return waitFor([&]() { return getValue() == expected; }, timeoutMs);
}

/// Waits for multiple signals to be emitted (all must fire)
/// @param spies List of QSignalSpy objects to monitor
/// @param timeoutMs Maximum time to wait for all signals
/// @return true if all signals emitted within timeout
inline bool waitForAllSignals(const QList<QSignalSpy*>& spies, int timeoutMs = kDefaultTimeoutMs)
{
    QElapsedTimer timer;
    timer.start();

    for (QSignalSpy* spy : spies) {
        if (!spy) {
            continue;
        }

        const int remainingTime = timeoutMs - static_cast<int>(timer.elapsed());
        if (remainingTime <= 0) {
            return false;
        }

        if (spy->count() == 0 && !spy->wait(remainingTime)) {
            return false;
        }
    }

    return true;
}

/// Waits for any one of multiple signals to be emitted
/// @param spies List of QSignalSpy objects to monitor
/// @param timeoutMs Maximum time to wait
/// @return Index of first signal emitted, or -1 on timeout
inline int waitForAnySignal(const QList<QSignalSpy*>& spies, int timeoutMs = kDefaultTimeoutMs)
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        for (int i = 0; i < spies.size(); ++i) {
            if (spies[i] && spies[i]->count() > 0) {
                return i;
            }
        }
        QTest::qWait(kDefaultPollIntervalMs);
    }

    return -1;
}

/// Processes Qt events for a short time to allow signals to propagate
/// Use sparingly - prefer waitFor() with conditions
/// @param maxTimeMs Maximum time to process events
inline void processEvents(int maxTimeMs = 100)
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < maxTimeMs) {
        QCoreApplication::processEvents();
        QTest::qWait(10);
    }
}

// ============================================================================
// Verification Utilities
// ============================================================================

/// Verifies a pointer is not null with a descriptive failure message
/// @param ptr Pointer to check
/// @param name Name of the pointer for error message
/// @return true if not null
inline bool verifyNotNull(const void* ptr, const char* name)
{
    if (!ptr) {
        qWarning() << "Null pointer check failed:" << name;
        return false;
    }
    return true;
}

// ============================================================================
// Numeric Comparison Utilities
// ============================================================================

/// Compares two doubles with epsilon tolerance
/// @param actual Actual value
/// @param expected Expected value
/// @param epsilon Tolerance (default 1e-6)
/// @return true if values are within epsilon
inline bool fuzzyCompare(double actual, double expected, double epsilon = 1e-6)
{
    return std::abs(actual - expected) < epsilon;
}

/// Checks if value is within range [min, max]
/// @return true if min <= value <= max
template<typename T>
    requires std::totally_ordered<T>
inline bool withinRange(T value, T min, T max)
{
    return value >= min && value <= max;
}

/// Checks if value is within percentage of expected
/// @param actual Actual value
/// @param expected Expected value
/// @param percentTolerance Tolerance as percentage (e.g., 5.0 for 5%)
/// @return true if within tolerance
inline bool withinPercent(double actual, double expected, double percentTolerance)
{
    if (expected == 0.0) {
        return actual == 0.0;
    }
    const double tolerance = std::abs(expected) * (percentTolerance / 100.0);
    return std::abs(actual - expected) <= tolerance;
}

// ============================================================================
// JSON Comparison Utilities
// ============================================================================

/// Compares two JSON documents for equality
/// @return true if documents are equal
inline bool jsonEquals(const QJsonDocument& actual, const QJsonDocument& expected)
{
    return actual == expected;
}

/// Compares two JSON objects for equality
/// @return true if objects are equal
inline bool jsonEquals(const QJsonObject& actual, const QJsonObject& expected)
{
    return actual == expected;
}

/// Checks if JSON object contains expected key
/// @return true if key exists
inline bool jsonHasKey(const QJsonObject& obj, const QString& key)
{
    return obj.contains(key);
}

/// Gets JSON value with type checking
/// @param obj JSON object
/// @param key Key to look up
/// @param defaultValue Value to return if key missing or wrong type
/// @return Value at key or default
template<typename T>
inline T jsonValue(const QJsonObject& obj, const QString& key, const T& defaultValue);

template<>
inline int jsonValue(const QJsonObject& obj, const QString& key, const int& defaultValue)
{
    return obj.contains(key) ? obj[key].toInt(defaultValue) : defaultValue;
}

template<>
inline double jsonValue(const QJsonObject& obj, const QString& key, const double& defaultValue)
{
    return obj.contains(key) ? obj[key].toDouble(defaultValue) : defaultValue;
}

template<>
inline QString jsonValue(const QJsonObject& obj, const QString& key, const QString& defaultValue)
{
    return obj.contains(key) ? obj[key].toString(defaultValue) : defaultValue;
}

template<>
inline bool jsonValue(const QJsonObject& obj, const QString& key, const bool& defaultValue)
{
    return obj.contains(key) ? obj[key].toBool(defaultValue) : defaultValue;
}

// ============================================================================
// Scoped Timer for Performance Testing
// ============================================================================

/// Creates a scoped timer that logs elapsed time when destroyed
class ScopedTimer
{
public:
    explicit ScopedTimer(const char* name) : _name(name)
    {
        _timer.start();
    }

    ~ScopedTimer()
    {
        qDebug() << "Timer" << _name << "elapsed:" << _timer.elapsed() << "ms";
    }

    qint64 elapsed() const { return _timer.elapsed(); }

private:
    const char* _name;
    QElapsedTimer _timer;
};

/// RAII helper for measuring test performance
class TestBenchmark
{
public:
    explicit TestBenchmark(const char* name, int iterations = 1)
        : _name(name), _iterations(iterations)
    {
        _timer.start();
    }

    ~TestBenchmark()
    {
        const qint64 total = _timer.elapsed();
        const double perIteration = static_cast<double>(total) / _iterations;
        qDebug() << "Benchmark" << _name << "-" << _iterations << "iterations,"
                 << total << "ms total," << perIteration << "ms/iteration";
    }

private:
    const char* _name;
    int _iterations;
    QElapsedTimer _timer;
};

// ============================================================================
// Timeout Guard for Operations
// ============================================================================

/// Timeout guard that fails the test if an operation takes too long
/// Use SCOPED_TIMEOUT macro for automatic failure reporting
class TimeoutGuard
{
public:
    /// Creates a timeout guard
    /// @param name Name of the operation being guarded
    /// @param timeoutMs Maximum allowed time in milliseconds
    explicit TimeoutGuard(const char* name, int timeoutMs)
        : _name(name), _timeoutMs(timeoutMs)
    {
        _timer.start();
    }

    ~TimeoutGuard()
    {
        check();
    }

    /// Checks if timeout has been exceeded
    /// @return true if still within timeout
    bool check() const
    {
        if (_timer.elapsed() > _timeoutMs) {
            if (!_warned) {
                qWarning() << "Timeout guard" << _name << "exceeded:"
                           << _timer.elapsed() << "ms >" << _timeoutMs << "ms";
                _warned = true;
            }
            return false;
        }
        return true;
    }

    /// Returns remaining time in milliseconds
    int remaining() const
    {
        return qMax(0, _timeoutMs - static_cast<int>(_timer.elapsed()));
    }

    /// Returns elapsed time in milliseconds
    qint64 elapsed() const { return _timer.elapsed(); }

    /// Returns true if timeout has been exceeded
    bool isExpired() const { return _timer.elapsed() > _timeoutMs; }

private:
    const char* _name;
    int _timeoutMs;
    QElapsedTimer _timer;
    mutable bool _warned = false;
};

/// Adaptive timeout that adjusts based on system load
/// Returns timeout multiplied by slowdown factor on CI or slow systems
inline int adaptiveTimeout(int baseTimeoutMs)
{
    // Check for CI environment (common env vars)
    static bool isCi = qEnvironmentVariableIsSet("CI") ||
                       qEnvironmentVariableIsSet("GITHUB_ACTIONS") ||
                       qEnvironmentVariableIsSet("GITLAB_CI") ||
                       qEnvironmentVariableIsSet("JENKINS_URL");

    // CI environments and VMs often run slower
    if (isCi) {
        return baseTimeoutMs * 2;
    }

    return baseTimeoutMs;
}

// ============================================================================
// File/Directory Verification Utilities
// ============================================================================

/// Verifies a file exists
/// @param path Path to the file
/// @return true if file exists
inline bool fileExists(const QString& path)
{
    return QFileInfo::exists(path) && QFileInfo(path).isFile();
}

/// Verifies a directory exists
/// @param path Path to the directory
/// @return true if directory exists
inline bool dirExists(const QString& path)
{
    return QFileInfo::exists(path) && QFileInfo(path).isDir();
}

/// Verifies a file contains expected content (substring match)
/// @param path Path to the file
/// @param content Content to search for
/// @return true if file contains the content
inline bool fileContains(const QString& path, const QString& content)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "fileContains: cannot open file:" << path;
        return false;
    }
    const QString fileContent = QString::fromUtf8(file.readAll());
    return fileContent.contains(content);
}

/// Verifies a file contains valid JSON
/// @param path Path to the file
/// @param doc Optional output parameter for parsed document
/// @return true if file contains valid JSON
inline bool fileIsValidJson(const QString& path, QJsonDocument* doc = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "fileIsValidJson: cannot open file:" << path;
        return false;
    }

    QJsonParseError error;
    const QJsonDocument parsed = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "fileIsValidJson: parse error:" << error.errorString();
        return false;
    }

    if (doc) {
        *doc = parsed;
    }
    return true;
}

/// Returns file size in bytes, or -1 if file doesn't exist
inline qint64 fileSize(const QString& path)
{
    QFileInfo info(path);
    return info.exists() ? info.size() : -1;
}

/// Reads entire file content as string
/// @param path Path to the file
/// @param ok Optional output parameter indicating success
/// @return File content or empty string on failure
inline QString readFileContent(const QString& path, bool* ok = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        return QString();
    }
    if (ok) *ok = true;
    return QString::fromUtf8(file.readAll());
}

/// Reads entire file content as bytes
/// @param path Path to the file
/// @param ok Optional output parameter indicating success
/// @return File content or empty array on failure
inline QByteArray readFileBytes(const QString& path, bool* ok = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (ok) *ok = false;
        return QByteArray();
    }
    if (ok) *ok = true;
    return file.readAll();
}

// ============================================================================
// Coordinate/Geo Utilities
// ============================================================================

/// Default tolerance for coordinate comparisons (1 meter)
constexpr double kDefaultCoordToleranceMeters = 1.0;

/// Compares two coordinates with distance tolerance
/// @param coord1 First coordinate
/// @param coord2 Second coordinate
/// @param toleranceMeters Maximum distance in meters
/// @return true if coordinates are within tolerance
inline bool coordsEqual(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2,
                        double toleranceMeters = kDefaultCoordToleranceMeters)
{
    if (!coord1.isValid() || !coord2.isValid()) {
        return coord1.isValid() == coord2.isValid();
    }
    return coord1.distanceTo(coord2) <= toleranceMeters;
}

/// Compares coordinates including altitude
/// @param coord1 First coordinate
/// @param coord2 Second coordinate
/// @param horizontalToleranceMeters Horizontal distance tolerance
/// @param altitudeToleranceMeters Altitude difference tolerance
/// @return true if coordinates are within tolerance
inline bool coordsEqual3D(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2,
                          double horizontalToleranceMeters = kDefaultCoordToleranceMeters,
                          double altitudeToleranceMeters = 1.0)
{
    if (!coordsEqual(coord1, coord2, horizontalToleranceMeters)) {
        return false;
    }

    // If neither has altitude, consider them equal
    if (qIsNaN(coord1.altitude()) && qIsNaN(coord2.altitude())) {
        return true;
    }

    // If only one has altitude, not equal
    if (qIsNaN(coord1.altitude()) || qIsNaN(coord2.altitude())) {
        return false;
    }

    return std::abs(coord1.altitude() - coord2.altitude()) <= altitudeToleranceMeters;
}

/// Verifies a coordinate is valid
inline bool coordValid(const QGeoCoordinate& coord)
{
    return coord.isValid();
}

/// Verifies latitude is in valid range [-90, 90]
inline bool latitudeValid(double latitude)
{
    return latitude >= -90.0 && latitude <= 90.0;
}

/// Verifies longitude is in valid range [-180, 180]
inline bool longitudeValid(double longitude)
{
    return longitude >= -180.0 && longitude <= 180.0;
}

/// Creates a coordinate offset from the original by the specified distance and bearing
/// @param coord Original coordinate
/// @param distanceMeters Distance in meters
/// @param bearingDegrees Bearing in degrees (0 = North, 90 = East)
/// @return New offset coordinate
inline QGeoCoordinate offsetCoord(const QGeoCoordinate& coord, double distanceMeters, double bearingDegrees = 0.0)
{
    return coord.atDistanceAndAzimuth(distanceMeters, bearingDegrees);
}

/// Calculates distance between two coordinates in meters
inline double coordDistance(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2)
{
    return coord1.distanceTo(coord2);
}

// ============================================================================
// Polygon/Polyline Utilities
// ============================================================================

/// Verifies a list of coordinates forms a valid polygon (at least 3 points)
inline bool isValidPolygon(const QList<QGeoCoordinate>& coords)
{
    if (coords.size() < 3) {
        return false;
    }

    for (const QGeoCoordinate& coord : coords) {
        if (!coord.isValid()) {
            return false;
        }
    }

    return true;
}

/// Verifies a list of coordinates forms a valid polyline (at least 2 points)
inline bool isValidPolyline(const QList<QGeoCoordinate>& coords)
{
    if (coords.size() < 2) {
        return false;
    }

    for (const QGeoCoordinate& coord : coords) {
        if (!coord.isValid()) {
            return false;
        }
    }

    return true;
}

/// Calculates the total length of a polyline in meters
inline double polylineLength(const QList<QGeoCoordinate>& coords)
{
    if (coords.size() < 2) {
        return 0.0;
    }

    double totalLength = 0.0;
    for (int i = 1; i < coords.size(); ++i) {
        totalLength += coords[i - 1].distanceTo(coords[i]);
    }
    return totalLength;
}

} // namespace TestHelpers

// ============================================================================
// Convenience Macros
// ============================================================================

/// Verifies pointer is not null with automatic message
#define VERIFY_NOT_NULL(ptr) \
    QVERIFY2(TestHelpers::verifyNotNull(ptr, #ptr), "Pointer " #ptr " is null")

/// Waits for condition with timeout and fails test if not met
#define WAIT_FOR_CONDITION(condition, timeout) \
    QVERIFY2(TestHelpers::waitFor([&]() { return (condition); }, timeout), \
             "Timeout waiting for condition: " #condition)

/// Fuzzy compare for doubles with automatic message
#define FUZZY_COMPARE(actual, expected) \
    QVERIFY2(TestHelpers::fuzzyCompare(actual, expected), \
             qPrintable(QString("Fuzzy compare failed: %1 != %2").arg(actual).arg(expected)))

/// Fuzzy compare with custom epsilon
#define FUZZY_COMPARE_EPS(actual, expected, eps) \
    QVERIFY2(TestHelpers::fuzzyCompare(actual, expected, eps), \
             qPrintable(QString("Fuzzy compare failed: %1 != %2 (eps=%3)").arg(actual).arg(expected).arg(eps)))

/// Verify value is within range
#define VERIFY_IN_RANGE(value, min, max) \
    QVERIFY2(TestHelpers::withinRange(value, min, max), \
             qPrintable(QString("Value %1 not in range [%2, %3]").arg(value).arg(min).arg(max)))

// ============================================================================
// Timeout Macros
// ============================================================================

/// Creates a scoped timeout guard - warns if the scope takes too long
/// Usage: SCOPED_TIMEOUT("operation name", 5000);
#define SCOPED_TIMEOUT(name, timeoutMs) \
    TestHelpers::TimeoutGuard _timeoutGuard_##__LINE__(name, timeoutMs)

/// Creates a scoped timeout with adaptive timeout (adjusts for CI)
#define SCOPED_TIMEOUT_ADAPTIVE(name, baseTimeoutMs) \
    TestHelpers::TimeoutGuard _timeoutGuard_##__LINE__(name, TestHelpers::adaptiveTimeout(baseTimeoutMs))

/// Verify operation completed within timeout
#define VERIFY_TIMEOUT(guard) \
    QVERIFY2((guard).check(), \
             qPrintable(QString("Operation timed out after %1ms").arg((guard).elapsed())))

// ============================================================================
// File/Directory Macros
// ============================================================================

/// Verify file exists
#define VERIFY_FILE_EXISTS(path) \
    QVERIFY2(TestHelpers::fileExists(path), \
             qPrintable(QString("File does not exist: %1").arg(path)))

/// Verify file does not exist
#define VERIFY_FILE_NOT_EXISTS(path) \
    QVERIFY2(!TestHelpers::fileExists(path), \
             qPrintable(QString("File unexpectedly exists: %1").arg(path)))

/// Verify directory exists
#define VERIFY_DIR_EXISTS(path) \
    QVERIFY2(TestHelpers::dirExists(path), \
             qPrintable(QString("Directory does not exist: %1").arg(path)))

/// Verify file contains content (substring)
#define VERIFY_FILE_CONTAINS(path, content) \
    QVERIFY2(TestHelpers::fileContains(path, content), \
             qPrintable(QString("File %1 does not contain: %2").arg(path).arg(content)))

/// Verify file is valid JSON
#define VERIFY_JSON_VALID(path) \
    QVERIFY2(TestHelpers::fileIsValidJson(path), \
             qPrintable(QString("File is not valid JSON: %1").arg(path)))

/// Verify file size
#define VERIFY_FILE_SIZE(path, expectedSize) \
    QCOMPARE(TestHelpers::fileSize(path), static_cast<qint64>(expectedSize))

// ============================================================================
// Coordinate/Geo Macros
// ============================================================================

/// Verify two coordinates are equal within tolerance
#define VERIFY_COORDS_EQUAL(coord1, coord2) \
    QVERIFY2(TestHelpers::coordsEqual(coord1, coord2), \
             qPrintable(QString("Coordinates not equal: (%1,%2) vs (%3,%4), distance=%5m") \
                       .arg((coord1).latitude()).arg((coord1).longitude()) \
                       .arg((coord2).latitude()).arg((coord2).longitude()) \
                       .arg((coord1).distanceTo(coord2))))

/// Verify two coordinates are equal within custom tolerance
#define VERIFY_COORDS_EQUAL_TOL(coord1, coord2, toleranceMeters) \
    QVERIFY2(TestHelpers::coordsEqual(coord1, coord2, toleranceMeters), \
             qPrintable(QString("Coordinates not equal within %1m: distance=%2m") \
                       .arg(toleranceMeters).arg((coord1).distanceTo(coord2))))

/// Verify coordinate is valid
#define VERIFY_COORD_VALID(coord) \
    QVERIFY2(TestHelpers::coordValid(coord), "Coordinate is not valid")

/// Verify polygon is valid (at least 3 valid points)
#define VERIFY_POLYGON_VALID(coords) \
    QVERIFY2(TestHelpers::isValidPolygon(coords), \
             qPrintable(QString("Invalid polygon: %1 points").arg((coords).size())))

/// Verify polyline is valid (at least 2 valid points)
#define VERIFY_POLYLINE_VALID(coords) \
    QVERIFY2(TestHelpers::isValidPolyline(coords), \
             qPrintable(QString("Invalid polyline: %1 points").arg((coords).size())))

// ============================================================================
// Property-Based Testing Utilities
// ============================================================================

namespace TestHelpers {

/// Default number of iterations for property-based tests
constexpr int kDefaultPropertyIterations = 100;

/// Result of a property test
struct PropertyTestResult {
    bool passed = true;
    int failedIteration = -1;
    QString failureMessage;
};

/// Runs a property test with random inputs
/// @param iterations Number of test iterations
/// @param generator Lambda that generates test input
/// @param property Lambda that tests the property (returns true if property holds)
/// @return PropertyTestResult with success/failure information
template<typename Generator, typename Property>
inline PropertyTestResult checkProperty(int iterations, Generator generator, Property property)
{
    PropertyTestResult result;

    for (int i = 0; i < iterations; ++i) {
        auto input = generator();

        if (!property(input)) {
            result.passed = false;
            result.failedIteration = i;
            return result;
        }
    }

    return result;
}

/// Runs a property test and reports failures
/// @param name Name of the property being tested
/// @param iterations Number of test iterations
/// @param generator Lambda that generates test input
/// @param property Lambda that tests the property
/// @return true if all iterations passed
template<typename Generator, typename Property>
inline bool testProperty(const char* name, int iterations, Generator generator, Property property)
{
    auto result = checkProperty(iterations, generator, property);

    if (!result.passed) {
        qWarning() << "Property" << name << "failed at iteration" << result.failedIteration;
        return false;
    }

    return true;
}

// Note: For random data generation with seeding support, use MockHelpers::TestDataGenerator

/// Generates a list of random values using a generator function
template<typename Generator>
inline QList<typename std::invoke_result<Generator>::type> randomList(int minSize, int maxSize, Generator generator)
{
    QList<typename std::invoke_result<Generator>::type> result;
    const int size = QRandomGenerator::global()->bounded(minSize, maxSize + 1);
    result.reserve(size);
    for (int i = 0; i < size; ++i) {
        result.append(generator());
    }
    return result;
}

/// Picks a random element from a list
template<typename T>
inline T randomElement(const QList<T>& list)
{
    if (list.isEmpty()) {
        return T();
    }
    return list.at(QRandomGenerator::global()->bounded(list.size()));
}

// ============================================================================
// RAII Signal Capture - Simplified single-signal waiting and extraction
// ============================================================================

/// RAII helper for capturing a single signal with simplified argument extraction.
/// Reduces boilerplate for the common pattern of waiting for one signal.
///
/// Example:
/// @code
/// SignalCapture capture(vehicle, &Vehicle::armedChanged);
/// vehicle->setArmed(true);
/// QVERIFY(capture.wait());
/// QCOMPARE(capture.arg<bool>(0), true);
/// @endcode
class SignalCapture
{
public:
    /// Create a signal capture for a signal
    template<typename Sender, typename Signal>
    SignalCapture(Sender* sender, Signal signal, int timeoutMs = kDefaultTimeoutMs)
        : _spy(sender, signal), _timeoutMs(timeoutMs)
    {
    }

    /// Wait for the signal to be emitted
    /// @return true if signal was received before timeout
    bool wait() { return _spy.wait(_timeoutMs); }

    /// Wait with custom timeout
    bool wait(int timeoutMs) { return _spy.wait(timeoutMs); }

    /// Check if signal was already emitted (non-blocking)
    bool received() const { return _spy.count() > 0; }

    /// Get emission count
    int count() const { return _spy.count(); }

    /// Get argument from signal emission
    /// @param argIndex Which argument (0 = first)
    /// @param emission Which emission (0 = most recent, -1 for last)
    template<typename T>
    T arg(int argIndex = 0, int emission = 0) const
    {
        const int idx = (emission < 0) ? _spy.count() - 1 : emission;
        if (idx < 0 || idx >= _spy.count()) {
            return T{};
        }
        return _spy.at(idx).at(argIndex).value<T>();
    }

    /// Get all arguments from an emission as QVariantList
    QList<QVariant> args(int emission = 0) const
    {
        if (emission < 0 || emission >= _spy.count()) {
            return {};
        }
        return _spy.at(emission);
    }

    /// Clear captured signals
    void clear() { _spy.clear(); }

    /// Access the underlying spy
    QSignalSpy& spy() { return _spy; }
    const QSignalSpy& spy() const { return _spy; }

private:
    QSignalSpy _spy;
    int _timeoutMs;
};

// ============================================================================
// Scoped Cleanup Helper - RAII for test cleanup
// ============================================================================

/// RAII helper that calls a cleanup function when destroyed.
/// Useful for ensuring cleanup happens even if test assertions fail.
///
/// Example:
/// @code
/// auto cleanup = ScopedCleanup([&]() { delete controller; controller = nullptr; });
/// // ... test code that might fail ...
/// // cleanup runs automatically when scope exits
/// @endcode
class ScopedCleanup
{
public:
    template<typename Func>
    explicit ScopedCleanup(Func&& func) : _func(std::forward<Func>(func)) {}

    ~ScopedCleanup()
    {
        if (_func) {
            _func();
        }
    }

    /// Prevent cleanup from running
    void dismiss() { _func = nullptr; }

    // Non-copyable
    ScopedCleanup(const ScopedCleanup&) = delete;
    ScopedCleanup& operator=(const ScopedCleanup&) = delete;

    // Movable
    ScopedCleanup(ScopedCleanup&& other) noexcept : _func(std::move(other._func))
    {
        other._func = nullptr;
    }

private:
    std::function<void()> _func;
};

// ============================================================================
// Test Case Runner - Reduce parameterized test boilerplate
// ============================================================================

/// Runs a worker function for each test case in an array, with debug output.
/// Reduces boilerplate for parameterized tests.
///
/// Example:
/// @code
/// struct TestCase { int input; int expected; };
/// static const TestCase cases[] = {{1, 2}, {2, 4}, {3, 6}};
/// runTestCases(cases, [this](const TestCase& tc) {
///     QCOMPARE(doubleIt(tc.input), tc.expected);
/// });
/// @endcode
template<typename TestCase, typename Worker>
inline void runTestCases(std::span<const TestCase> testCases, Worker&& worker)
{
    int index = 0;
    for (const TestCase& testCase : testCases) {
        qDebug() << "Running test case" << index++;
        worker(testCase);
    }
}

/// Overload for mutable test cases (when worker needs to modify test case state)
template<typename TestCase, typename Worker>
inline void runTestCases(std::span<TestCase> testCases, Worker&& worker)
{
    int index = 0;
    for (TestCase& testCase : testCases) {
        qDebug() << "Running test case" << index++;
        worker(testCase);
    }
}

/// Overload for C-style arrays (const)
template<typename TestCase, size_t N, typename Worker>
inline void runTestCases(const TestCase (&testCases)[N], Worker&& worker)
{
    runTestCases(std::span<const TestCase>(testCases, N), std::forward<Worker>(worker));
}

/// Overload for C-style arrays (mutable)
template<typename TestCase, size_t N, typename Worker>
inline void runTestCases(TestCase (&testCases)[N], Worker&& worker)
{
    runTestCases(std::span<TestCase>(testCases, N), std::forward<Worker>(worker));
}

// ============================================================================
// Signal Sequence Verifier - Verify signals fire in expected order
// ============================================================================

/// Verifies that signals are emitted in a specific order.
///
/// Example:
/// @code
/// SignalSequence seq;
/// seq.expect(obj, &MyClass::started)
///    .expect(obj, &MyClass::progress)
///    .expect(obj, &MyClass::finished);
/// obj->doWork();
/// QVERIFY(seq.verify());
/// @endcode
class SignalSequence
{
public:
    SignalSequence() = default;

    /// Add expected signal to the sequence
    template<typename Sender, typename Signal>
    SignalSequence& expect(Sender* sender, Signal signal)
    {
        const int expectedIndex = _expectedOrder.size();
        _expectedOrder.append(expectedIndex);

        auto conn = QObject::connect(sender, signal, [this, expectedIndex]() {
            _actualOrder.append(expectedIndex);
        });
        _connections.append(conn);

        return *this;
    }

    /// Verify signals were emitted in expected order
    bool verify() const
    {
        return _actualOrder == _expectedOrder;
    }

    /// Get actual order of emissions (for debugging)
    QList<int> actualOrder() const { return _actualOrder; }

    /// Reset for reuse
    void reset()
    {
        _actualOrder.clear();
    }

    ~SignalSequence()
    {
        for (const auto& conn : _connections) {
            QObject::disconnect(conn);
        }
    }

private:
    QList<int> _expectedOrder;
    QList<int> _actualOrder;
    QList<QMetaObject::Connection> _connections;
};

// ============================================================================
// Deferred Value Checker - Check value after delay
// ============================================================================

/// Checks that a value eventually reaches an expected state.
/// Useful for async operations where the final state is what matters.
///
/// Example:
/// @code
/// QVERIFY(eventuallyEquals([&]() { return obj->state(); }, ExpectedState, 5000));
/// @endcode
template<typename Getter, typename T>
inline bool eventuallyEquals(Getter&& getValue, const T& expected, int timeoutMs = kDefaultTimeoutMs)
{
    return waitFor([&]() { return getValue() == expected; }, timeoutMs);
}

/// Check value eventually becomes true
template<typename Getter>
inline bool eventuallyTrue(Getter&& getValue, int timeoutMs = kDefaultTimeoutMs)
{
    return waitFor([&]() { return static_cast<bool>(getValue()); }, timeoutMs);
}

/// Check value eventually becomes false
template<typename Getter>
inline bool eventuallyFalse(Getter&& getValue, int timeoutMs = kDefaultTimeoutMs)
{
    return waitFor([&]() { return !static_cast<bool>(getValue()); }, timeoutMs);
}

} // namespace TestHelpers

// ============================================================================
// Additional Convenience Macros
// ============================================================================

/// Macro for property-based tests
/// Usage: VERIFY_PROPERTY("property name", 100, generator, property_check)
#define VERIFY_PROPERTY(name, iterations, generator, property) \
    QVERIFY2(TestHelpers::testProperty(name, iterations, generator, property), \
             qPrintable(QString("Property '%1' failed").arg(name)))

/// Macro for property-based tests with default iterations
#define VERIFY_PROPERTY_DEFAULT(name, generator, property) \
    VERIFY_PROPERTY(name, TestHelpers::kDefaultPropertyIterations, generator, property)

/// Create a scoped cleanup that runs on scope exit
/// Usage: SCOPED_CLEANUP([&]() { delete ptr; });
#define SCOPED_CLEANUP(func) \
    TestHelpers::ScopedCleanup _scopedCleanup_##__LINE__(func)

/// Verify signal was emitted with QSignalSpy
#define QGC_VERIFY_SPY_VALID(spy) \
    QVERIFY2((spy).isValid(), "Signal spy is not valid - check signal signature")

/// Verify a container is empty
#define QGC_VERIFY_EMPTY(container) \
    QVERIFY2((container).isEmpty(), \
             qPrintable(QString("Expected empty container, got %1 items").arg((container).size())))

/// Verify a container has expected size
#define QGC_VERIFY_SIZE(container, expectedSize) \
    QCOMPARE_EQ((container).size(), static_cast<decltype((container).size())>(expectedSize))

/// Wait for value to equal expected
#define WAIT_FOR_VALUE(getter, expected, timeout) \
    QVERIFY2(TestHelpers::eventuallyEquals([&]() { return (getter); }, (expected), (timeout)), \
             qPrintable(QString("Timeout waiting for value to equal expected")))

/// Wait for condition to become true
#define WAIT_FOR_TRUE(getter, timeout) \
    QVERIFY2(TestHelpers::eventuallyTrue([&]() { return (getter); }, (timeout)), \
             qPrintable(QString("Timeout waiting for condition to become true")))
