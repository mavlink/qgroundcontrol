find_package(Qt6 COMPONENTS REQUIRED Core Network)

set(EXTRA_SRC)

if (ANDROID)
    list(APPEND EXTRA_SRC
        MobileScreenMgr.cc
        MobileScreenMgr.h
    )
endif ()

qt_add_library(Tools STATIC
    QGCCachedFileDownload.cc
    QGCCachedFileDownload.h
    QGCFileDownload.cc
    QGCFileDownload.h
    QGCLoggingCategory.cc
    QGCLoggingCategory.h
    QGCTemporaryFile.cc
    QGCTemporaryFile.h
    ${EXTRA_SRC}
)

target_link_libraries(Tools
    PUBLIC
        Qt6::Core
        Qt6::Network
)

target_include_directories(Tools
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

