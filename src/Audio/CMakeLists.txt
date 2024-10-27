find_package(Qt6 REQUIRED COMPONENTS Core TextToSpeech)

qt_add_library(Audio STATIC
    AudioOutput.cc
    AudioOutput.h
)

target_link_libraries(Audio
    PRIVATE
        Qt6::TextToSpeech
        FactSystem
        Utilities
    PUBLIC
        Qt6::Core
)

target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(LINUX)
    find_package(SpeechDispatcher)
    if(TARGET SpeechDispatcher::SpeechDispatcher)
        target_link_libraries(Audio PRIVATE SpeechDispatcher::SpeechDispatcher)
    endif()

    find_package(Flite)
    if(TARGET Flite::Flite)
        target_link_libraries(Audio PRIVATE Flite::Flite)
    endif()
endif()
