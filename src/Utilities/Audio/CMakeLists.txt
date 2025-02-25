find_package(Qt6 REQUIRED COMPONENTS Core TextToSpeech)

target_sources(Utilities
    PRIVATE
        AudioOutput.cc
        AudioOutput.h
)

target_link_libraries(Utilities
    PRIVATE
        Qt6::TextToSpeech
        FactSystem
    PUBLIC
        Qt6::Core
)

target_include_directories(Utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
