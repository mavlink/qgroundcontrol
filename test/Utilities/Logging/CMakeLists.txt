target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LogEntryTest.cc
        LogEntryTest.h
        LogTestHelpers.h
        LogFormatterTest.cc
        LogFormatterTest.h
        LogManagerTest.cc
        LogManagerTest.h
        LogModelTest.cc
        LogModelTest.h
        LoggingCategoryModelTest.cc
        LoggingCategoryModelTest.h
        LoggingQmlBindingTest.cc
        LoggingQmlBindingTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
