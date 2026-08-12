# Volador Branding
# Phase 1
# Volador GCS Core Architecture & Branding CMake Build Configuration

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/IVoladorService.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/VoladorCore.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/VoladorCore.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/VoladorStartupController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/VoladorStartupController.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/VoladorVersion.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Database/VoladorDatabaseService.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Database/VoladorDatabaseService.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/Security/VoladorSecurityService.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Security/VoladorSecurityService.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/VoladorAuthViewModel.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/VoladorAuthViewModel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/volador.qrc
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Database
        ${CMAKE_CURRENT_SOURCE_DIR}/Security
        ${CMAKE_CURRENT_SOURCE_DIR}/UI
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Concurrent
)
