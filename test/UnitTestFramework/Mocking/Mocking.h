#pragma once

/// @file Mocking.h
/// @brief Trompeloeil mocking support for QGroundControl unit tests
///
/// Trompeloeil is a header-only C++14 mocking framework that integrates
/// with Qt Test. It provides:
/// - MAKE_MOCK macros for declaring mock methods
/// - REQUIRE_CALL for setting expectations
/// - Sequence verification
/// - Argument matching
///
/// @see https://github.com/rollbear/trompeloeil
///
/// ## Basic Usage
///
/// ```cpp
/// #include "Mocking/Mocking.h"
///
/// // Define a mock class
/// class MockVehicle {
/// public:
///     MAKE_MOCK0(id, int());
///     MAKE_MOCK1(setArmed, void(bool));
///     MAKE_CONST_MOCK0(isArmed, bool());
/// };
///
/// // In your test
/// void MyTest::_testArming() {
///     MockVehicle vehicle;
///
///     // Set expectations
///     REQUIRE_CALL(vehicle, id()).RETURN(1);
///     REQUIRE_CALL(vehicle, setArmed(true));
///     REQUIRE_CALL(vehicle, isArmed()).RETURN(true);
///
///     // Call code under test
///     armVehicle(&vehicle);
///
///     // Expectations verified automatically at end of scope
/// }
/// ```
///
/// ## Argument Matchers
///
/// ```cpp
/// using trompeloeil::_;      // Any value
/// using trompeloeil::ne;     // Not equal
/// using trompeloeil::lt;     // Less than
/// using trompeloeil::gt;     // Greater than
/// using trompeloeil::ge;     // Greater or equal
/// using trompeloeil::le;     // Less or equal
///
/// REQUIRE_CALL(mock, setValue(_));           // Any int
/// REQUIRE_CALL(mock, setValue(gt(0)));       // Positive only
/// REQUIRE_CALL(mock, setName(ne("")));       // Non-empty string
/// ```
///
/// ## Call Counts
///
/// ```cpp
/// REQUIRE_CALL(mock, foo()).TIMES(3);        // Exactly 3 times
/// REQUIRE_CALL(mock, foo()).TIMES(1, 5);     // 1 to 5 times
/// REQUIRE_CALL(mock, foo()).TIMES(AT_LEAST(1));
/// REQUIRE_CALL(mock, foo()).TIMES(AT_MOST(3));
/// ALLOW_CALL(mock, foo());                    // Zero or more times
/// FORBID_CALL(mock, foo());                   // Must not be called
/// ```
///
/// ## Sequences
///
/// ```cpp
/// trompeloeil::sequence seq;
/// REQUIRE_CALL(mock, first()).IN_SEQUENCE(seq);
/// REQUIRE_CALL(mock, second()).IN_SEQUENCE(seq);
/// REQUIRE_CALL(mock, third()).IN_SEQUENCE(seq);
/// ```
///
/// ## Side Effects
///
/// ```cpp
/// REQUIRE_CALL(mock, getData())
///     .SIDE_EFFECT(_1 = cachedData)   // Modify out parameter
///     .RETURN(true);
///
/// REQUIRE_CALL(mock, process())
///     .THROW(std::runtime_error("failed"));
/// ```
///
/// ## Mocking Qt Signals
///
/// For Qt classes with signals, create a mock that inherits QObject:
///
/// ```cpp
/// class MockLinkInterface : public QObject {
///     Q_OBJECT
/// public:
///     MAKE_MOCK0(isConnected, bool());
///     MAKE_MOCK0(disconnect, void());
///
/// signals:
///     void connected();
///     void disconnected();
///     void bytesReceived(const QByteArray& data);
/// };
/// ```
///
/// ## Best Practices
///
/// 1. Keep mocks minimal - only mock what you need
/// 2. Use interfaces (pure virtual classes) for mockable dependencies
/// 3. Prefer REQUIRE_CALL over ALLOW_CALL for explicit expectations
/// 4. Use sequences when call order matters
/// 5. Mock at boundaries (network, file system, hardware)

#include <trompeloeil.hpp>

// Re-export commonly used matchers for convenience
using trompeloeil::_;
using trompeloeil::ge;
using trompeloeil::gt;
using trompeloeil::le;
using trompeloeil::lt;
using trompeloeil::ne;

/// Qt Test adapter for Trompeloeil violations
/// Reports mock expectation failures through Qt Test
namespace trompeloeil {

template<>
inline void reporter<specialized>::send(
    severity s,
    const char* file,
    unsigned long line,
    std::string const& msg)
{
    if (s == severity::fatal) {
        QTest::qFail(msg.c_str(), file, static_cast<int>(line));
    } else {
        QTest::qWarn(msg.c_str(), file, static_cast<int>(line));
    }
}

template<>
inline void reporter<specialized>::sendOk(const char* /*trompeloeil_mock_calls_done_correctly*/)
{
    // Success - nothing to report
}

} // namespace trompeloeil
