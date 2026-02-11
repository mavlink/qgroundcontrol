# ----------------------------------------------------------------------------
# ApprovalTests.cpp (Snapshot/Golden Master Testing)
# ----------------------------------------------------------------------------
CPMAddPackage(
    NAME ApprovalTests
    GITHUB_REPOSITORY approvals/ApprovalTests.cpp
    GIT_TAG v.10.13.0
)
qgc_disable_dependency_warnings(ApprovalTests)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ApprovalTests::ApprovalTests)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ApprovalTesting.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
