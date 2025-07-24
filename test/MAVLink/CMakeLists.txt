target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        StatusTextHandlerTest.cc
        StatusTextHandlerTest.h
        SigningTest.cc
        SigningTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
