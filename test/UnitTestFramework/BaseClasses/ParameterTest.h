#pragma once

#include "VehicleTest.h"

class ParameterManager;
class Fact;

/// @file
/// @brief Base class for parameter manager tests

/// Test fixture for parameter manager tests. Automatically waits for
/// parameters to be ready before running tests.
///
/// Example usage:
/// @code
/// class MyParameterTest : public ParameterTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testParameterRead() {
///         Fact* fact = getFact("SYS_AUTOSTART");
///         QVERIFY(fact);
///     }
/// };
/// @endcode
class ParameterTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit ParameterTest(QObject* parent = nullptr);
    ~ParameterTest() override = default;

    /// Returns the parameter manager
    ParameterManager* parameterManager() const;

    /// Gets a parameter fact by name
    /// @param name Parameter name
    /// @param componentId Component ID (-1 = default component)
    /// @return Fact pointer or nullptr if not found
    Fact* getFact(const QString& name, int componentId = -1) const;

    /// Checks if a parameter exists
    bool parameterExists(const QString& name, int componentId = -1) const;

protected:
    /// Waits for a specific parameter to be updated
    bool waitForParameterUpdate(const QString& name, int timeoutMs = 0);
};
