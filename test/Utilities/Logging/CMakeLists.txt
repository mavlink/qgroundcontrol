target_sources(
    QGCUnitTests
    PRIVATE
        LogTestHelpers.h
)
add_qgc_tests(
    LogEntryTest
    LogFormatterTest
    LogManagerTest
    LogModelTest
    LoggingCategoryModelTest
    LoggingQmlBindingTest
    LABELS Unit Utilities
)
