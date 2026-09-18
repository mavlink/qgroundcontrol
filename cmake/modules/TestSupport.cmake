include_guard(GLOBAL)

# Catch dependencies hidden by application-wide includes and links.
function(qgc_check_library_consumer target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "SOURCE;TIMEOUT" "LABELS")
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_check_library_consumer: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_check_library_consumer: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "qgc_check_library_consumer: target does not exist: ${target}")
    endif()
    if(NOT ARG_SOURCE)
        message(FATAL_ERROR "qgc_check_library_consumer: SOURCE is required")
    endif()
    if(NOT DEFINED ARG_TIMEOUT)
        set(ARG_TIMEOUT 30)
    endif()
    if(NOT ARG_TIMEOUT MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "qgc_check_library_consumer: TIMEOUT must be a positive integer")
    endif()
    if(NOT ARG_LABELS)
        set(ARG_LABELS Unit)
    endif()

    set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        ${target}HeaderChecks ALL
        DEPENDS ${target}_verify_interface_header_sets
        COMMENT "Verify ${target} public headers in isolation"
    )
    add_executable(${target}Consumer "${ARG_SOURCE}")
    set_target_properties(
        ${target}Consumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(${target}Consumer PRIVATE ${target})
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "${ARG_LABELS}" TIMEOUT "${ARG_TIMEOUT}")
endfunction()

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
