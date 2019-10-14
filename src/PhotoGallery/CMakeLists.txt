add_library(PhotoGallery
    AbstractPhotoTrigger.cc
    AsyncDownloadPhotoTrigger.cc
    PhotoFileStore.cc
    PhotoGalleryModel.cc
    PhotoGalleryVehicleGlue.cc
    PhotoGalleryView.cc
)

add_custom_target(
    PhotoGalleryQml
SOURCES
    PhotoGalleryView.qml
)

target_link_libraries(PhotoGallery
    PUBLIC
        ui
)

target_include_directories(PhotoGallery
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

