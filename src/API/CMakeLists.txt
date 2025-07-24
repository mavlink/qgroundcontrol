target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCCorePlugin.cc
        QGCCorePlugin.h
        QGCOptions.cc
        QGCOptions.h
        QmlComponentInfo.cc
        QmlComponentInfo.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
