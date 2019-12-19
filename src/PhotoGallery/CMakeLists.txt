add_library(PhotoGallery
    AbstractPhotoTrigger.cc
    AsyncDownloadPhotoTrigger.cc
    ExtractJPEGMetadata.cc
    JPEGSegmentParser.cc
    PhotoFileStore.cc
    PhotoFileStoreInterface.cc
    PhotoGalleryModel.cc
    PhotoGalleryVehicleGlue.cc
    PhotoGalleryView.cc
    easyexif.cpp
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

