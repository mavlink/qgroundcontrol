# Reject fallback versions only when a native package is actually requested.
# Ordinary build and test jobs may intentionally use shallow Git checkouts.

if(NOT DEFINED QGC_NATIVE_PACKAGE_VERSION OR QGC_NATIVE_PACKAGE_VERSION STREQUAL "")
    message(FATAL_ERROR "QGC: native package version is unavailable")
endif()

if(QGC_NATIVE_PACKAGE_VERSION MATCHES "^v?0\\.0\\.0($|\\.|-)")
    message(
        FATAL_ERROR
            "QGC: native package version resolved to 0.0.0. Fetch Git history and tags before building qgc-package."
    )
endif()

message(STATUS "QGC: native package version: ${QGC_NATIVE_PACKAGE_VERSION}")
