find_package(Qt6 REQUIRED COMPONENTS Core Qml)

target_sources(QGC
    PRIVATE
        FactPanelController.cc
        FactPanelController.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
