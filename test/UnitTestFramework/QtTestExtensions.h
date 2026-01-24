#pragma once

/// @file QtTestExtensions.h
/// @brief Extended Qt Test utilities for QGroundControl tests.
///
/// This header provides value-add utilities beyond standard Qt Test features.
/// For standard Qt Test functionality, use Qt's macros directly:
///   - QCOMPARE, QCOMPARE_EQ, QCOMPARE_NE, etc.
///   - QTRY_VERIFY, QTRY_COMPARE, etc.
///   - QVERIFY_THROWS_EXCEPTION, QVERIFY_THROWS_NO_EXCEPTION
///   - QBENCHMARK, QFETCH, QTest::newRow, etc.

#include <QtCore/QPoint>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtGui/QPointingDevice>
#include <QtGui/QWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <cmath>

// =============================================================================
// Warning/Message Handling (pattern matching adds value over Qt)
// =============================================================================

/// Fail the test if a warning matching the pattern is emitted
#define QGC_FAIL_ON_WARNING_PATTERN(pattern) \
    QTest::failOnWarning(QRegularExpression(pattern))

/// Ignore warnings matching a regex pattern (substring match)
/// Note: Qt 6's QTest::ignoreMessage with QRegularExpression requires a full match.
/// This macro wraps the pattern with .* anchors to allow substring matching,
/// which is more natural for testing log messages with category prefixes.
#define QGC_IGNORE_MESSAGE_PATTERN(msgType, pattern) \
    QTest::ignoreMessage(msgType, QRegularExpression(".*" pattern ".*"))

// =============================================================================
// Test Data Location
// =============================================================================

/// Verify test data file exists and return its path (fails if missing)
#define QGC_REQUIRE_TEST_DATA(filename) \
    [&]() { \
        QString path = QFINDTESTDATA(filename); \
        if (path.isEmpty()) { \
            QFAIL(qPrintable(QStringLiteral("Test data not found: %1").arg(filename))); \
        } \
        return path; \
    }()

// =============================================================================
// Floating Point Comparison (better error messages than QCOMPARE)
// =============================================================================

/// Compare floating point values with tolerance (default: 1e-6)
#define QGC_COMPARE_FLOAT(actual, expected, ...) \
    QGC_COMPARE_FLOAT_IMPL(actual, expected, ##__VA_ARGS__, 1e-6)

#define QGC_COMPARE_FLOAT_IMPL(actual, expected, tolerance, ...) \
    do { \
        const double _actual = static_cast<double>(actual); \
        const double _expected = static_cast<double>(expected); \
        const double _tolerance = static_cast<double>(tolerance); \
        const double _diff = std::abs(_actual - _expected); \
        if (_diff > _tolerance) { \
            QFAIL(qPrintable(QStringLiteral("Float comparison failed: %1 (%2) vs %3 (%4), diff=%5, tolerance=%6") \
                .arg(#actual).arg(_actual).arg(#expected).arg(_expected).arg(_diff).arg(_tolerance))); \
        } \
    } while (false)

/// Compare floating point values with relative tolerance (default: 1e-6)
#define QGC_COMPARE_FLOAT_RELATIVE(actual, expected, ...) \
    QGC_COMPARE_FLOAT_RELATIVE_IMPL(actual, expected, ##__VA_ARGS__, 1e-6)

#define QGC_COMPARE_FLOAT_RELATIVE_IMPL(actual, expected, relativeTolerance, ...) \
    do { \
        const double _actual = static_cast<double>(actual); \
        const double _expected = static_cast<double>(expected); \
        const double _relTol = static_cast<double>(relativeTolerance); \
        const double _maxAbs = std::max(std::abs(_actual), std::abs(_expected)); \
        const double _diff = std::abs(_actual - _expected); \
        const double _allowedDiff = _maxAbs * _relTol; \
        if (_diff > _allowedDiff && _diff > 1e-12) { \
            QFAIL(qPrintable(QStringLiteral("Relative float comparison failed: %1 (%2) vs %3 (%4), diff=%5, allowed=%6") \
                .arg(#actual).arg(_actual).arg(#expected).arg(_expected).arg(_diff).arg(_allowedDiff))); \
        } \
    } while (false)

// =============================================================================
// Collection/Container Assertions (better error messages)
// =============================================================================

/// Verify a container is empty
#ifndef QGC_VERIFY_EMPTY
#define QGC_VERIFY_EMPTY(container) \
    do { \
        if (!(container).isEmpty()) { \
            QFAIL(qPrintable(QStringLiteral("%1 is not empty (size=%2)").arg(#container).arg((container).size()))); \
        } \
    } while (false)
#endif

/// Verify a container is not empty
#define QGC_VERIFY_NOT_EMPTY(container) \
    do { \
        if ((container).isEmpty()) { \
            QFAIL(qPrintable(QStringLiteral("%1 is empty").arg(#container))); \
        } \
    } while (false)

/// Verify container size
#define QGC_COMPARE_SIZE(container, expectedSize) \
    do { \
        const auto _size = (container).size(); \
        const auto _expected = (expectedSize); \
        if (_size != _expected) { \
            QFAIL(qPrintable(QStringLiteral("%1 size mismatch: got %2, expected %3").arg(#container).arg(_size).arg(_expected))); \
        } \
    } while (false)

/// Verify container contains an element
#define QGC_VERIFY_CONTAINS(container, element) \
    do { \
        if (!(container).contains(element)) { \
            QFAIL(qPrintable(QStringLiteral("%1 does not contain expected element").arg(#container))); \
        } \
    } while (false)

// =============================================================================
// String Assertions (better error messages)
// =============================================================================

/// Verify string contains substring
#define QGC_VERIFY_STRING_CONTAINS(string, substring) \
    do { \
        if (!(string).contains(substring)) { \
            QFAIL(qPrintable(QStringLiteral("String '%1' does not contain '%2'").arg(string, substring))); \
        } \
    } while (false)

/// Verify string matches regex pattern
#define QGC_VERIFY_STRING_MATCHES(string, pattern) \
    do { \
        QRegularExpression _re(pattern); \
        if (!_re.match(string).hasMatch()) { \
            QFAIL(qPrintable(QStringLiteral("String '%1' does not match pattern '%2'").arg(string, pattern))); \
        } \
    } while (false)

/// Verify string starts with prefix
#define QGC_VERIFY_STRING_STARTS_WITH(string, prefix) \
    do { \
        if (!(string).startsWith(prefix)) { \
            QFAIL(qPrintable(QStringLiteral("String '%1' does not start with '%2'").arg(string, prefix))); \
        } \
    } while (false)

/// Verify string ends with suffix
#define QGC_VERIFY_STRING_ENDS_WITH(string, suffix) \
    do { \
        if (!(string).endsWith(suffix)) { \
            QFAIL(qPrintable(QStringLiteral("String '%1' does not end with '%2'").arg(string, suffix))); \
        } \
    } while (false)

// =============================================================================
// Coordinate/Geo Assertions (QGC-specific)
// =============================================================================

/// Compare coordinates with tolerance (default 1e-7 degrees ~ 1cm)
#define QGC_COMPARE_COORDINATE(actual, expected, ...) \
    QGC_COMPARE_COORDINATE_IMPL(actual, expected, ##__VA_ARGS__, 1e-7)

#define QGC_COMPARE_COORDINATE_IMPL(actual, expected, tolerance, ...) \
    do { \
        const auto& _qgc_coord_actual = (actual); \
        const auto& _qgc_coord_expected = (expected); \
        const double _qgc_coord_tol = (tolerance); \
        if (std::abs(_qgc_coord_actual.latitude() - _qgc_coord_expected.latitude()) > _qgc_coord_tol) { \
            QFAIL(qPrintable(QStringLiteral("Coordinate latitude mismatch: %1 vs %2 (diff=%3, tol=%4)") \
                .arg(_qgc_coord_actual.latitude()).arg(_qgc_coord_expected.latitude()) \
                .arg(std::abs(_qgc_coord_actual.latitude() - _qgc_coord_expected.latitude())).arg(_qgc_coord_tol))); \
        } \
        if (std::abs(_qgc_coord_actual.longitude() - _qgc_coord_expected.longitude()) > _qgc_coord_tol) { \
            QFAIL(qPrintable(QStringLiteral("Coordinate longitude mismatch: %1 vs %2 (diff=%3, tol=%4)") \
                .arg(_qgc_coord_actual.longitude()).arg(_qgc_coord_expected.longitude()) \
                .arg(std::abs(_qgc_coord_actual.longitude() - _qgc_coord_expected.longitude())).arg(_qgc_coord_tol))); \
        } \
        if (_qgc_coord_actual.type() == QGeoCoordinate::Coordinate3D && \
            _qgc_coord_expected.type() == QGeoCoordinate::Coordinate3D) { \
            if (std::abs(_qgc_coord_actual.altitude() - _qgc_coord_expected.altitude()) > 0.01) { \
                QFAIL(qPrintable(QStringLiteral("Coordinate altitude mismatch: %1 vs %2") \
                    .arg(_qgc_coord_actual.altitude()).arg(_qgc_coord_expected.altitude()))); \
            } \
        } \
    } while (false)

/// Verify coordinate is valid
#define QGC_VERIFY_VALID_COORDINATE(coord) \
    do { \
        if (!(coord).isValid()) { \
            QFAIL(qPrintable(QStringLiteral("%1 is not a valid coordinate").arg(#coord))); \
        } \
    } while (false)

// =============================================================================
// QSignalSpy Helpers (convenient type-safe argument extraction)
// =============================================================================
// Note: For signal verification macros, use MultiSignalSpy.h:
//   QVERIFY_SIGNAL, QVERIFY_SIGNAL_EMITTED, QVERIFY_ONLY_SIGNAL,
//   QVERIFY_NO_SIGNAL, QVERIFY_NO_SIGNALS, QVERIFY_SIGNAL_COUNT

/// Get the first argument from the first signal emission
#define QGC_SPY_ARG(spy, T) qvariant_cast<T>((spy).at(0).at(0))

/// Get argument at index from the first signal emission
#define QGC_SPY_ARG_AT(spy, index, T) qvariant_cast<T>((spy).at(0).at(index))

/// Get argument from specific emission
#define QGC_SPY_ARG_EMISSION(spy, emission, argIndex, T) \
    qvariant_cast<T>((spy).at(emission).at(argIndex))

/// Get all arguments from first emission
#define QGC_SPY_ARGS(spy) (spy).at(0)

/// Take first emission (removes it from spy)
#define QGC_SPY_TAKE_FIRST(spy) (spy).takeFirst()

/// Verify spy is valid (properly connected)
#ifndef QGC_VERIFY_SPY_VALID
#define QGC_VERIFY_SPY_VALID(spy) \
    QVERIFY2((spy).isValid(), "QSignalSpy failed to connect to signal")
#endif

// =============================================================================
// Platform Detection
// =============================================================================

namespace QGCTestPlatform {

inline bool isLinux() {
#ifdef Q_OS_LINUX
    return true;
#else
    return false;
#endif
}

inline bool isWindows() {
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

inline bool isMacOS() {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

inline bool isAndroid() {
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

inline bool isIOS() {
#ifdef Q_OS_IOS
    return true;
#else
    return false;
#endif
}

inline bool isMobile() { return isAndroid() || isIOS(); }
inline bool isDesktop() { return isLinux() || isWindows() || isMacOS(); }

inline bool isCI() {
    return !qEnvironmentVariableIsEmpty("CI") ||
           !qEnvironmentVariableIsEmpty("GITHUB_ACTIONS") ||
           !qEnvironmentVariableIsEmpty("JENKINS_URL") ||
           !qEnvironmentVariableIsEmpty("TRAVIS");
}

inline bool isWayland() {
#ifdef Q_OS_LINUX
    return qEnvironmentVariable("XDG_SESSION_TYPE") == "wayland" ||
           !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
#else
    return false;
#endif
}

inline bool hasDisplay() {
#ifdef Q_OS_LINUX
    return !qEnvironmentVariableIsEmpty("DISPLAY") ||
           !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
#else
    return true;
#endif
}

inline QString platformName() {
#if defined(Q_OS_ANDROID)
    return QStringLiteral("Android");
#elif defined(Q_OS_IOS)
    return QStringLiteral("iOS");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macOS");
#elif defined(Q_OS_WIN)
    return QStringLiteral("Windows");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("Linux");
#else
    return QStringLiteral("Unknown");
#endif
}

} // namespace QGCTestPlatform

// =============================================================================
// Platform-Specific Test Skipping
// =============================================================================

#define QGC_SKIP_ON_LINUX(reason) \
    do { if (QGCTestPlatform::isLinux()) QSKIP(qPrintable(QStringLiteral("Skipped on Linux: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_WINDOWS(reason) \
    do { if (QGCTestPlatform::isWindows()) QSKIP(qPrintable(QStringLiteral("Skipped on Windows: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_MACOS(reason) \
    do { if (QGCTestPlatform::isMacOS()) QSKIP(qPrintable(QStringLiteral("Skipped on macOS: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_ANDROID(reason) \
    do { if (QGCTestPlatform::isAndroid()) QSKIP(qPrintable(QStringLiteral("Skipped on Android: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_IOS(reason) \
    do { if (QGCTestPlatform::isIOS()) QSKIP(qPrintable(QStringLiteral("Skipped on iOS: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_MOBILE(reason) \
    do { if (QGCTestPlatform::isMobile()) QSKIP(qPrintable(QStringLiteral("Skipped on mobile: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_CI(reason) \
    do { if (QGCTestPlatform::isCI()) QSKIP(qPrintable(QStringLiteral("Skipped in CI: %1").arg(reason))); } while (false)

#define QGC_SKIP_ON_WAYLAND(reason) \
    do { if (QGCTestPlatform::isWayland()) QSKIP(qPrintable(QStringLiteral("Skipped on Wayland: %1").arg(reason))); } while (false)

#define QGC_SKIP_WITHOUT_DISPLAY(reason) \
    do { if (!QGCTestPlatform::hasDisplay()) QSKIP(qPrintable(QStringLiteral("Skipped without display: %1").arg(reason))); } while (false)

#define QGC_SKIP_UNLESS(condition, reason) \
    do { if (!(condition)) QSKIP(qPrintable(QStringLiteral("Skipped: %1").arg(reason))); } while (false)

#define QGC_SKIP_IF(condition, reason) \
    do { if (condition) QSKIP(qPrintable(QStringLiteral("Skipped: %1").arg(reason))); } while (false)

// =============================================================================
// Touch Gesture Simulation (complex gestures not easily done with Qt Test)
// =============================================================================

/// Simulate a touch swipe gesture
#define QGC_TOUCH_SWIPE(window, from, to, ...) \
    QGC_TOUCH_SWIPE_IMPL(window, from, to, ##__VA_ARGS__, 300)
#define QGC_TOUCH_SWIPE_IMPL(window, from, to, durationMs, ...) \
    do { \
        static QPointingDevice* _touchDevice = QTest::createTouchDevice(); \
        const int _steps = 10; \
        const int _stepDelay = (durationMs) / _steps; \
        const QPoint _from = (from); \
        const QPoint _to = (to); \
        QTest::touchEvent(window, _touchDevice).press(0, _from).commit(); \
        for (int _i = 1; _i <= _steps; ++_i) { \
            QPoint _pos = _from + (_to - _from) * _i / _steps; \
            QTest::touchEvent(window, _touchDevice).move(0, _pos).commit(); \
            QTest::qWait(_stepDelay); \
        } \
        QTest::touchEvent(window, _touchDevice).release(0, _to).commit(); \
    } while (false)

/// Simulate a pinch gesture (zoom in/out)
#define QGC_TOUCH_PINCH(window, center, startDistance, endDistance, ...) \
    QGC_TOUCH_PINCH_IMPL(window, center, startDistance, endDistance, ##__VA_ARGS__, 300)
#define QGC_TOUCH_PINCH_IMPL(window, center, startDistance, endDistance, durationMs, ...) \
    do { \
        static QPointingDevice* _touchDevice = QTest::createTouchDevice(); \
        const int _steps = 10; \
        const int _stepDelay = (durationMs) / _steps; \
        const QPoint _center = (center); \
        const int _startDist = (startDistance); \
        const int _endDist = (endDistance); \
        QPoint _p1 = _center - QPoint(_startDist / 2, 0); \
        QPoint _p2 = _center + QPoint(_startDist / 2, 0); \
        QTest::touchEvent(window, _touchDevice).press(0, _p1).press(1, _p2).commit(); \
        for (int _i = 1; _i <= _steps; ++_i) { \
            int _dist = _startDist + (_endDist - _startDist) * _i / _steps; \
            _p1 = _center - QPoint(_dist / 2, 0); \
            _p2 = _center + QPoint(_dist / 2, 0); \
            QTest::touchEvent(window, _touchDevice).move(0, _p1).move(1, _p2).commit(); \
            QTest::qWait(_stepDelay); \
        } \
        QTest::touchEvent(window, _touchDevice).release(0, _p1).release(1, _p2).commit(); \
    } while (false)
