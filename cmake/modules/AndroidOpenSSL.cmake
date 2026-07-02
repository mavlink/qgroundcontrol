include_guard(GLOBAL)
include(Download)

set(QGC_ANDROID_OPENSSL_VERSION "3.5.6" CACHE STRING "OpenSSL version built from source for Android")
set(QGC_ANDROID_OPENSSL_SHA256 "deae7c80cba99c4b4f940ecadb3c3338b13cb77418409238e57d7f31f2a3b736"
    CACHE STRING "SHA256 of the OpenSSL source tarball for QGC_ANDROID_OPENSSL_VERSION")

set(_android_openssl_rev b71f1470962019bd89534a2919f5925f93bc5779)
set(_android_openssl_hash SHA256=9277d62ecdb4809801e2c369e0a639c154e0d9137e8d60863b44bf07d16ed5b3)
if(CPM_SOURCE_CACHE)
    set(_android_openssl_dl_dir "${CPM_SOURCE_CACHE}/android_openssl")
else()
    set(_android_openssl_dl_dir "${CMAKE_BINARY_DIR}/_deps/android_openssl-dl")
endif()
qgc_resilient_download(
    FILENAME "android_openssl-${_android_openssl_rev}.zip"
    DESTINATION_DIR "${_android_openssl_dl_dir}"
    RESULT_VAR _android_openssl_archive
    URLS "https://github.com/KDAB/android_openssl/archive/${_android_openssl_rev}.zip"
    EXPECTED_HASH "${_android_openssl_hash}"
    LOG_TAG "Android OpenSSL"
)
CPMAddPackage(
    NAME android_openssl
    URL "${_android_openssl_archive}"
    URL_HASH "${_android_openssl_hash}"
)

qgc_require_cpm_added(android_openssl)

file(REMOVE_RECURSE
    "${android_openssl_SOURCE_DIR}/ssl_1.1"
    "${android_openssl_SOURCE_DIR}/no-asm/ssl_1.1"
    "${android_openssl_SOURCE_DIR}/ssl_1.patch"
)

if(QT_ANDROID_ABIS)
    set(_qgc_target_abis ${QT_ANDROID_ABIS})
else()
    set(_qgc_target_abis "${CMAKE_ANDROID_ARCH_ABI}")
endif()

set(_ossl_stamp "${android_openssl_SOURCE_DIR}/.qgc_openssl_version")
set(_ossl_have_version "")
if(EXISTS "${_ossl_stamp}")
    file(STRINGS "${_ossl_stamp}" _ossl_have_version LIMIT_COUNT 1)
endif()
set(_ossl_build_archs "")
foreach(_abi IN LISTS _qgc_target_abis)
    if(_ossl_have_version STREQUAL QGC_ANDROID_OPENSSL_VERSION
       AND EXISTS "${android_openssl_SOURCE_DIR}/ssl_3/${_abi}/libssl_3.so"
       AND EXISTS "${android_openssl_SOURCE_DIR}/no-asm/ssl_3/${_abi}/libssl_3.so")
        continue()
    endif()
    if(_abi STREQUAL "arm64-v8a")
        list(APPEND _ossl_build_archs "arm64")
    elseif(_abi STREQUAL "armeabi-v7a")
        list(APPEND _ossl_build_archs "arm")
    elseif(_abi STREQUAL "x86_64")
        list(APPEND _ossl_build_archs "x86_64")
    elseif(_abi STREQUAL "x86")
        list(APPEND _ossl_build_archs "x86")
    else()
        message(FATAL_ERROR "QGC: Unsupported Android ABI for OpenSSL: ${_abi}")
    endif()
endforeach()
list(REMOVE_DUPLICATES _ossl_build_archs)

if(CMAKE_HOST_WIN32)
    message(WARNING "QGC: Windows build host — using KDAB's bundled OpenSSL (EOL 3.1). "
                    "The published APK is built on the Linux host, which uses OpenSSL "
                    "${QGC_ANDROID_OPENSSL_VERSION}; rebuild there for the updated library.")
elseif(_ossl_build_archs)
    if(CPM_SOURCE_CACHE)
        set(_ossl_dl_dir "${CPM_SOURCE_CACHE}/openssl-src")
    else()
        set(_ossl_dl_dir "${CMAKE_BINARY_DIR}/_deps/openssl-src")
    endif()
    qgc_resilient_download(
        FILENAME "openssl-${QGC_ANDROID_OPENSSL_VERSION}.tar.gz"
        DESTINATION_DIR "${_ossl_dl_dir}"
        RESULT_VAR _ossl_tarball
        URLS
            "https://github.com/openssl/openssl/releases/download/openssl-${QGC_ANDROID_OPENSSL_VERSION}/openssl-${QGC_ANDROID_OPENSSL_VERSION}.tar.gz"
            "https://www.openssl.org/source/openssl-${QGC_ANDROID_OPENSSL_VERSION}.tar.gz"
        EXPECTED_HASH "SHA256=${QGC_ANDROID_OPENSSL_SHA256}"
        LOG_TAG "Android OpenSSL"
    )
    list(JOIN _ossl_build_archs " " _ossl_build_archs_str)
    message(STATUS "QGC: Building OpenSSL ${QGC_ANDROID_OPENSSL_VERSION} for Android (${_ossl_build_archs_str}) — first configure only, this is slow")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/setup/build_android_openssl.py"
            --version "${QGC_ANDROID_OPENSSL_VERSION}"
            --tarball "${_ossl_tarball}"
            --ndk "${CMAKE_ANDROID_NDK}"
            --patch "${android_openssl_SOURCE_DIR}/ssl_3.patch"
            --output-root "${android_openssl_SOURCE_DIR}"
            --build-dir "${CMAKE_BINARY_DIR}/_deps/openssl-build"
            --api "${QGC_QT_ANDROID_MIN_SDK_VERSION}"
            --archs ${_ossl_build_archs}
        RESULT_VARIABLE _ossl_build_result
    )
    if(NOT _ossl_build_result EQUAL 0)
        message(FATAL_ERROR "QGC: OpenSSL ${QGC_ANDROID_OPENSSL_VERSION} Android build failed (exit ${_ossl_build_result}). "
                            "Ensure perl and nasm (for x86 asm) are installed.")
    endif()
    file(WRITE "${_ossl_stamp}" "${QGC_ANDROID_OPENSSL_VERSION}\n")
endif()

include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
add_android_openssl_libraries(${CMAKE_PROJECT_NAME})
message(STATUS "QGC: Android OpenSSL libraries added")
