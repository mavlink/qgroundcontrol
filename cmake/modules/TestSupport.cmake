include_guard(GLOBAL)

# Attach the shared microbenchmark framework to a test target.
function(qgc_add_benchmark_support target)
    # ----------------------------------------------------------------------------
    # nanobench (Microbenchmarking Framework)
    # ----------------------------------------------------------------------------
    CPMAddPackage(
        NAME nanobench
        GITHUB_REPOSITORY martinus/nanobench
        VERSION 4.3.11
        SYSTEM YES
    )
    qgc_disable_dependency_warnings(nanobench)
    target_link_libraries(${target} PRIVATE nanobench)

    target_sources(${target} PRIVATE ${PROJECT_SOURCE_DIR}/test/UnitTestFramework/Benchmarking/Benchmarking.cc
                                     ${PROJECT_SOURCE_DIR}/test/UnitTestFramework/Benchmarking/Benchmarking.h
    )

    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/test/UnitTestFramework/Benchmarking)
endfunction()

# Attach the shared property testing framework to a test target.
function(qgc_add_property_support target)
    # ----------------------------------------------------------------------------
    # RapidCheck (Property-Based Testing Framework)
    # ----------------------------------------------------------------------------
    CPMAddPackage(
        NAME rapidcheck
        GITHUB_REPOSITORY emil-e/rapidcheck
        GIT_TAG b96a4e626ef4c7348dcd16c500353c2f997a9f3f
        SYSTEM YES
        OPTIONS "RC_ENABLE_GTEST OFF"
                "RC_ENABLE_DOCTEST OFF"
                "RC_ENABLE_CATCH OFF"
                "RC_ENABLE_GMOCK OFF"
                "RC_ENABLE_BOOST OFF"
                "RC_ENABLE_BOOST_TEST OFF"
                "RC_ENABLE_TESTS OFF"
                "RC_ENABLE_EXAMPLES OFF"
    )

    qgc_disable_dependency_warnings(rapidcheck)
    target_link_libraries(${target} PRIVATE rapidcheck)

    target_sources(${target} PRIVATE ${PROJECT_SOURCE_DIR}/test/UnitTestFramework/PropertyTesting/PropertyTesting.h)

    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/test/UnitTestFramework/PropertyTesting)
endfunction()
