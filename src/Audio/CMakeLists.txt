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
