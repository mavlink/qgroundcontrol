# Ordinary build and test jobs may intentionally use shallow Git checkouts, so
# reject fallback versions only when a native package is explicitly requested.
# qgc-package passes QGC_NATIVE_PACKAGE_VERSION; direct CPack/package presets
# expose the configured version as CPACK_PACKAGE_VERSION.

if((NOT DEFINED QGC_NATIVE_PACKAGE_VERSION OR QGC_NATIVE_PACKAGE_VERSION STREQUAL "")
   AND DEFINED CPACK_PACKAGE_VERSION
   AND NOT CPACK_PACKAGE_VERSION STREQUAL ""
)
    set(QGC_NATIVE_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
endif()

if(NOT DEFINED QGC_NATIVE_PACKAGE_VERSION OR QGC_NATIVE_PACKAGE_VERSION STREQUAL "")
    message(FATAL_ERROR "QGC: native package version is unavailable")
endif()

if(QGC_NATIVE_PACKAGE_VERSION MATCHES "^v?0\\.0\\.0($|[^0-9])")
    message(FATAL_ERROR "QGC: native package version resolved to 0.0.0. "
                        "Fetch Git history and tags before building a native package."
    )
endif()

message(STATUS "QGC: native package version: ${QGC_NATIVE_PACKAGE_VERSION}")
