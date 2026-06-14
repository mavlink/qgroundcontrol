qt_add_library(QGCAudio STATIC
    AudioOutput.cc
    AudioOutput.h
)

target_link_libraries(QGCAudio
    PUBLIC
        Qt6::Core
        Qt6::TextToSpeech
    PRIVATE
        QGCLogging
)

target_include_directories(QGCAudio
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/FactSystem
        ${CMAKE_SOURCE_DIR}/src/Utilities
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCAudio)

target_precompile_headers(QGCAudio PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtTextToSpeech/QTextToSpeech>
)
