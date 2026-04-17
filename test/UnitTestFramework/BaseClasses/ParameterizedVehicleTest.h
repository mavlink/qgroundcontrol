#pragma once

#include <functional>

#include "VehicleTestManualConnect.h"

/// Test fixture for parameterized vehicle tests with test case arrays.
///
/// Provides infrastructure for tests that iterate through arrays of test cases,
/// with automatic logging and per-case cleanup.
///
/// Example:
/// @code
/// class MyParameterizedTest : public ParameterizedVehicleTest
/// {
///     Q_OBJECT
///
///     struct TestCase {
///         const char* name;
///         int param1;
///         bool expectSuccess;
///     };
///
/// private slots:
///     void _runAllCases() {
///         static const TestCase cases[] = {
///             {"normal", 1, true},
///             {"edge", 0, false},
///         };
///         runTestCases<TestCase>(cases, std::size(cases), [this](const TestCase& tc, int i) {
///             logTestCase(i, tc.name);
///             // ... test logic ...
///         });
///     }
/// };
/// @endcode
class ParameterizedVehicleTest : public VehicleTestManualConnect
{
    Q_OBJECT

public:
    explicit ParameterizedVehicleTest(QObject* parent = nullptr) : VehicleTestManualConnect(parent)
    {
    }

protected:
    /// Runs all test cases, invoking @a testBody for each one.
    /// @tparam T Test case struct type
    /// @param cases Array of test cases
    /// @param count Number of test cases
    /// @param testBody Callback invoked with (const T& testCase, int index)
    template <typename T>
    void runTestCases(const T* cases, size_t count, std::function<void(const T&, int)> testBody)
    {
        _totalCases = static_cast<int>(count);
        _executedCases = 0;
        _currentCase = 0;

        for (size_t i = 0; i < count; ++i) {
            _currentCase = static_cast<int>(i);
            testBody(cases[i], static_cast<int>(i));
            _executedCases++;

            // Stop iterating if the current test function has already failed
            if (QTest::currentTestFailed()) {
                break;
            }

            // Ensure clean state between test cases
            if (_mockLink) {
                _disconnectMockLink();
            }
        }
    }

    /// Logs the start of a test case
    void logTestCase(int index, const char* name)
    {
        TEST_DEBUG(QStringLiteral("Test case %1/%2: %3").arg(index + 1).arg(_totalCases).arg(QString::fromLatin1(name)));
    }

    /// Logs the start of a test case with QString name
    void logTestCase(int index, const QString& name)
    {
        logTestCase(index, qPrintable(name));
    }

    /// Returns the current test case index
    int currentCaseIndex() const
    {
        return _currentCase;
    }

    /// Returns total number of test cases
    int totalCases() const
    {
        return _totalCases;
    }

    /// Returns number of executed test cases so far
    int passedCases() const
    {
        return _executedCases;
    }

private:
    int _totalCases = 0;
    int _executedCases = 0;
    int _currentCase = 0;
};
