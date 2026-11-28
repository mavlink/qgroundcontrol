#pragma once

#include <chrono>

#include <QtCore/QtEnvironmentVariables>

// ============================================================================
// Test Timeout Configuration
// ============================================================================

namespace TestTimeout {

/// Returns true when running under CI (GitHub Actions, etc.)
///
/// Evaluated on every call so tests may use EnvVarFixture to toggle the
/// CI/GITHUB_ACTIONS variables and observe the corresponding timeout changes
/// within the same process.
inline bool isCI()
{
    return qEnvironmentVariableIsSet("CI") || qEnvironmentVariableIsSet("GITHUB_ACTIONS");
}

/// Short timeout for quick operations (1s local, 2s CI)
inline std::chrono::milliseconds shortDuration()
{
    return std::chrono::milliseconds{isCI() ? 2000 : 1000};
}

/// Medium timeout for normal async operations (5s local, 10s CI)
inline std::chrono::milliseconds mediumDuration()
{
    return std::chrono::milliseconds{isCI() ? 10000 : 5000};
}

/// Long timeout for slow operations like vehicle connection (30s local, 60s CI)
inline std::chrono::milliseconds longDuration()
{
    return std::chrono::milliseconds{isCI() ? 60000 : 30000};
}

/// @name Legacy int-millisecond accessors (prefer the chrono versions above)
/// @{
inline int shortMs()
{
    return static_cast<int>(shortDuration().count());
}

inline int mediumMs()
{
    return static_cast<int>(mediumDuration().count());
}

inline int longMs()
{
    return static_cast<int>(longDuration().count());
}

/// @}

/// Iteration count for stress tests.
/// Uses QGC_TEST_STRESS_ITERATIONS when set to a positive integer.
inline int stressIterations(int localDefault, int ciDefault)
{
    bool ok = false;
    const int configured = qEnvironmentVariableIntValue("QGC_TEST_STRESS_ITERATIONS", &ok);
    if (ok && configured > 0) {
        return configured;
    }
    return isCI() ? ciDefault : localDefault;
}
}  // namespace TestTimeout
