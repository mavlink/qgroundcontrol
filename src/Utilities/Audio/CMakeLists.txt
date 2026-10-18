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

# Uses a system-installed Piper if present. Piper is GPL and is run as a separate process, never bundled.
if(LINUX)
    qt_add_plugin(QGCPiperTts
        STATIC
        CLASS_NAME PiperTtsPlugin
        PiperTtsEngine.cc
        PiperTtsEngine.h
        PiperTtsPlugin.cc
        PiperTtsPlugin.h
    )

    target_link_libraries(QGCPiperTts
        PUBLIC
            Qt6::Core
            Qt6::TextToSpeech
        PRIVATE
            Qt6::Multimedia
            QGCLogging
    )

    target_include_directories(QGCPiperTts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(QGCAudio PRIVATE QGCPiperTts)
    target_compile_definitions(QGCAudio PRIVATE QGC_PIPER_TTS)
endif()
