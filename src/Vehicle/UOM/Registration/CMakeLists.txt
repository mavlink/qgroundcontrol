if(TARGET QGCUomRegistrationCore)
    return()
endif()

find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)

add_library(
    QGCUomRegistrationCore STATIC UomRegistrationTypes.cc UomRegistrationCrypto.cc UomRegistrationStore.cc
                                  UomRegistrationClient.cc UomRegistrationCredentials.cc UomRegistrationWorkflow.cc
)
set_target_properties(
    QGCUomRegistrationCore
    PROPERTIES
        AUTOMOC ON
)
target_compile_features(QGCUomRegistrationCore PUBLIC cxx_std_20)
target_include_directories(QGCUomRegistrationCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCUomRegistrationCore
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Sql
    PRIVATE
        Qt6::Concurrent
        OpenSSL::Crypto
)
target_sources(
    QGCUomRegistrationCore
    PUBLIC
        FILE_SET HEADERS
        FILES
            UomRegistrationTypes.h
            UomRegistrationCrypto.h
            UomRegistrationStore.h
            UomRegistrationClient.h
            UomRegistrationCredentials.h
            UomRegistrationWorkflow.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(UOM_LIBSECRET QUIET IMPORTED_TARGET libsecret-1)
        if(TARGET PkgConfig::UOM_LIBSECRET)
            target_link_libraries(QGCUomRegistrationCore PRIVATE PkgConfig::UOM_LIBSECRET)
            target_compile_definitions(QGCUomRegistrationCore PRIVATE QGC_UOM_LIBSECRET)
        endif()
    endif()
endif()
