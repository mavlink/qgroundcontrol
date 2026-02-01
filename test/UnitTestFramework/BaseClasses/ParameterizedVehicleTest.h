#pragma once

#include <QtCore/QDebug>

#include "VehicleTestManualConnect.h"

/// Test fixture for parameterized vehicle tests with test case arrays.
///
/// Provides infrastructure for tests that iterate through arrays of test cases,
/// with automatic logging and result tracking.
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
///         runTestCases(cases, std::size(cases));
///     }
///
/// protected:
///     template<typename T>
///     void runSingleTestCase(const T& testCase, int index) override {
///         const auto& tc = static_cast<const TestCase&>(testCase);
///         logTestCase(index, tc.name);
///         // ... test logic ...
///     }
/// };
/// @endcode
class ParameterizedVehicleTest : public VehicleTestManualConnect
{
    Q_OBJECT

public:
    explicit ParameterizedVehicleTest(QObject* parent = nullptr)
        : VehicleTestManualConnect(parent) {}

protected:
    /// Runs all test cases in the array
    /// @tparam T Test case struct type
    /// @param cases Array of test cases
    /// @param count Number of test cases
    template <typename T>
    void runTestCases(const T* cases, size_t count)
    {
        _totalCases = static_cast<int>(count);
        _passedCases = 0;
        _failedCases = 0;
        _currentCase = 0;

        for (size_t i = 0; i < count; ++i) {
            _currentCase = static_cast<int>(i);
            bool passed = true;

            try {
                runSingleTestCase(cases[i], static_cast<int>(i));
                _passedCases++;
            } catch (...) {
                passed = false;
                _failedCases++;
            }

            // Ensure clean state between test cases
            if (_mockLink) {
                _disconnectMockLink();
            }
        }

        if (_failedCases > 0) {
            qWarning() << "Test cases:" << _passedCases << "passed," << _failedCases << "failed";
        }
    }

    /// Override in derived class to run a single test case
    /// @param testCase The test case data
    /// @param index Zero-based index of the test case
    template <typename T>
    void runSingleTestCase(const T& testCase, int index)
    {
        Q_UNUSED(testCase);
        Q_UNUSED(index);
        QFAIL("runSingleTestCase must be implemented in derived class");
    }

    /// Logs the start of a test case
    void logTestCase(int index, const char* name)
    {
        qDebug() << "Test case" << (index + 1) << "/" << _totalCases << ":" << name;
    }

    /// Logs the start of a test case with QString name
    void logTestCase(int index, const QString& name)
    {
        logTestCase(index, qPrintable(name));
    }

    /// Returns the current test case index
    int currentCaseIndex() const { return _currentCase; }

    /// Returns total number of test cases
    int totalCases() const { return _totalCases; }

    /// Returns number of passed test cases so far
    int passedCases() const { return _passedCases; }

    /// Returns number of failed test cases so far
    int failedCases() const { return _failedCases; }

private:
    int _totalCases = 0;
    int _passedCases = 0;
    int _failedCases = 0;
    int _currentCase = 0;
};
