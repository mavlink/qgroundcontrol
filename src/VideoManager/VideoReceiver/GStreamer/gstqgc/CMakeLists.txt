target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        gstqgc.cc
        gstqgcelement.cc
        gstqgcelements.h
        gstqgcvideosinkbin.cc
        gstqgcvideosinkbin.h
)

if(WIN32)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            gstqgcvideosinkbin_d3d11.cc
            gstqgcvideosinkbin_d3d11.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
