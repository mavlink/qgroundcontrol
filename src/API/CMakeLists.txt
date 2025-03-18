find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml QmlIntegration)

target_sources(QGC
    PRIVATE
        QGCCorePlugin.cc
        QGCCorePlugin.h
        QGCOptions.cc
        QGCOptions.h
        QmlComponentInfo.cc
        QmlComponentInfo.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(QGC_CUSTOM_BUILD)
    target_link_libraries(QGC PRIVATE CustomPlugin)
endif()
