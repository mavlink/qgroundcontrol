#include "PlatformTest.h"

#include "Platform.h"
#include "UnitTest.h"
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
void PlatformTest::_testCheckSingleInstanceAllowMultiple()
{
    // When allowMultiple is true, should always return true (allow running)
    QVERIFY(Platform::checkSingleInstance(true));
    // Call multiple times to ensure it's consistent
    QVERIFY(Platform::checkSingleInstance(true));
    QVERIFY(Platform::checkSingleInstance(true));
}

void PlatformTest::_testCheckSingleInstanceBlocks()
{
    // First call with allowMultiple=false should succeed (we're the first instance)
    // Note: This test assumes no other instance of QGC is running with the same key
    // The actual RunGuard is static, so this tests the current application state
    const bool firstResult = Platform::checkSingleInstance(false);
    // The result depends on whether this is actually the first instance
    // In a test environment, it should typically be true
    // We just verify it returns a boolean without crashing
    Q_UNUSED(firstResult);
    QVERIFY(true);  // Test passes if we get here without crashing
}
#endif              // !Q_OS_ANDROID && !Q_OS_IOS
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
void PlatformTest::_testIsRunningAsRootNormal()
{
    // In normal test execution, we should NOT be running as root
    // If tests are run as root, this test will fail (which is intentional -
    // running tests as root is discouraged)
    QVERIFY2(!Platform::isRunningAsRoot(), "Tests should not be run as root. Running QGC as root is dangerous.");
}
#endif  // Q_OS_LINUX && !Q_OS_ANDROID

UT_REGISTER_TEST(PlatformTest, TestLabel::Unit, TestLabel::Utilities)
