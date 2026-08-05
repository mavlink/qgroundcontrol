# ============================================================================
# Parsing Utilities
# EXIF, APMDataFlash (ArduPilot DataFlash binary log), PX4ULog (PX4 ULog), JSON
# ============================================================================

qt_add_library(QGCParsing STATIC
    APMDataFlash/APMDataFlashUtility.cc
    APMDataFlash/APMDataFlashUtility.h
    Exif/ExifUtility.cc
    Exif/ExifUtility.h
    Json/JsonParsing.cc
    Json/JsonParsing.h
    Json/JsonSchemaValidator.cc
    Json/JsonSchemaValidator.h
    PX4ULog/PX4ULogUtility.cc
    PX4ULog/PX4ULogUtility.h
)

# ============================================================================
# libexif (vendored - autotools, built directly from source)
# ============================================================================

set(QGC_LIBEXIF_VERSION 0.6.26)
CPMAddPackage(
    NAME libexif
    VERSION ${QGC_LIBEXIF_VERSION}
    GITHUB_REPOSITORY libexif/libexif
    GIT_TAG v${QGC_LIBEXIF_VERSION}
    DOWNLOAD_ONLY YES
)

qgc_require_cpm_added(libexif)

add_library(exif STATIC
    ${libexif_SOURCE_DIR}/libexif/exif-byte-order.c
    ${libexif_SOURCE_DIR}/libexif/exif-content.c
    ${libexif_SOURCE_DIR}/libexif/exif-data.c
    ${libexif_SOURCE_DIR}/libexif/exif-entry.c
    ${libexif_SOURCE_DIR}/libexif/exif-format.c
    ${libexif_SOURCE_DIR}/libexif/exif-gps-ifd.c
    ${libexif_SOURCE_DIR}/libexif/exif-ifd.c
    ${libexif_SOURCE_DIR}/libexif/exif-loader.c
    ${libexif_SOURCE_DIR}/libexif/exif-log.c
    ${libexif_SOURCE_DIR}/libexif/exif-mem.c
    ${libexif_SOURCE_DIR}/libexif/exif-mnote-data.c
    ${libexif_SOURCE_DIR}/libexif/exif-tag.c
    ${libexif_SOURCE_DIR}/libexif/exif-utils.c
    ${libexif_SOURCE_DIR}/libexif/canon/exif-mnote-data-canon.c
    ${libexif_SOURCE_DIR}/libexif/canon/mnote-canon-entry.c
    ${libexif_SOURCE_DIR}/libexif/canon/mnote-canon-tag.c
    ${libexif_SOURCE_DIR}/libexif/fuji/exif-mnote-data-fuji.c
    ${libexif_SOURCE_DIR}/libexif/fuji/mnote-fuji-entry.c
    ${libexif_SOURCE_DIR}/libexif/fuji/mnote-fuji-tag.c
    ${libexif_SOURCE_DIR}/libexif/olympus/exif-mnote-data-olympus.c
    ${libexif_SOURCE_DIR}/libexif/olympus/mnote-olympus-entry.c
    ${libexif_SOURCE_DIR}/libexif/olympus/mnote-olympus-tag.c
    ${libexif_SOURCE_DIR}/libexif/pentax/exif-mnote-data-pentax.c
    ${libexif_SOURCE_DIR}/libexif/pentax/mnote-pentax-entry.c
    ${libexif_SOURCE_DIR}/libexif/pentax/mnote-pentax-tag.c
)

target_include_directories(exif
    PUBLIC
        ${libexif_SOURCE_DIR}
    PRIVATE
        ${libexif_SOURCE_DIR}/libexif
)

set(LIBEXIF_CONFIG_H "${CMAKE_CURRENT_BINARY_DIR}/Exif/config.h")
file(WRITE ${LIBEXIF_CONFIG_H} "
#pragma once
#define PACKAGE \"libexif\"
#define PACKAGE_VERSION \"${QGC_LIBEXIF_VERSION}\"
#define VERSION \"${QGC_LIBEXIF_VERSION}\"
#define GETTEXT_PACKAGE \"libexif-12\"
")

set(LIBEXIF_STDINT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Exif/libexif")
file(MAKE_DIRECTORY ${LIBEXIF_STDINT_DIR})
file(WRITE "${LIBEXIF_STDINT_DIR}/_stdint.h" "
#pragma once
#include <stdint.h>

/* ssize_t is POSIX, not available on Windows/MSVC */
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif
")
target_include_directories(exif PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/Exif)

target_compile_definitions(exif PRIVATE HAVE_CONFIG_H)
set_target_properties(exif
    PROPERTIES
        C_STANDARD 99
        POSITION_INDEPENDENT_CODE ON
)

qgc_disable_dependency_warnings(exif)

# ============================================================================
# ulog_cpp (PX4 ULog parsing)
# ============================================================================

CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG v1.0.1
)

qgc_disable_dependency_warnings(ulog_cpp)

# ============================================================================
# valijson (header-only JSON-Schema validator, native Qt-JSON adapter)
# ============================================================================

CPMAddPackage(
    NAME valijson
    GITHUB_REPOSITORY tristanpenman/valijson
    # Pinned to 1.1.0: 1.1.1 calls std::from_chars(double) unguarded, which AppleClang's
    # libc++ (macos-15/ios CI, pre-LLVM 20) lacks — valijson#239, fixed in 1.1.2.
    GIT_TAG v1.1.0
    OPTIONS
        "valijson_BUILD_TESTS OFF"
        "valijson_BUILD_EXAMPLES OFF"
        "valijson_INSTALL OFF"
    # QtJsonAdapter isInteger()/getInteger() use 32-bit toInt(), so integral values
    # > INT_MAX (e.g. PX4 uint32 fileCrc) fail "type": "integer" checks or truncate.
    PATCHES valijson-qtjson-int64.patch
)

target_link_libraries(QGCParsing
    PUBLIC
        Qt6::Core
        exif
        ulog_cpp::ulog_cpp
    PRIVATE
        QGCCompression
        QGCLogging
        ValiJSON::valijson
)

# valijson defaults to abort() on malformed schema (it only enables exceptions when building
# its own tests); JsonSchemaValidator catches to stay warn-only, so force exceptions on here.
target_compile_definitions(QGCParsing PRIVATE VALIJSON_USE_EXCEPTIONS=1)

target_include_directories(QGCParsing SYSTEM PUBLIC ${ulog_cpp_SOURCE_DIR})

target_include_directories(QGCParsing
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/APMDataFlash
        ${CMAKE_CURRENT_SOURCE_DIR}/Exif
        ${CMAKE_CURRENT_SOURCE_DIR}/Json
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4ULog
)

target_precompile_headers(QGCParsing PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QByteArray>
    <QtCore/QJsonObject>
    <QtCore/QJsonArray>
    <QtCore/QJsonDocument>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCParsing)
