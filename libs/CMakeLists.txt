add_subdirectory(libevents)
add_subdirectory(qmlglsink)

qt_add_library(xz STATIC
    xz-embedded/linux/include/linux/xz.h
    xz-embedded/linux/include/linux/decompress/unxz.h
    # xz-embedded/linux/lib/decompress_unxz.c
    xz-embedded/linux/lib/xz/xz_crc32.c
    xz-embedded/linux/lib/xz/xz_crc64.c
    xz-embedded/linux/lib/xz/xz_dec_bcj.c
    xz-embedded/linux/lib/xz/xz_dec_lzma2.c
    xz-embedded/linux/lib/xz/xz_dec_stream.c
    # xz-embedded/linux/lib/xz/xz_dec_syms.c
    # xz-embedded/linux/lib/xz/xz_dec_test.c
    xz-embedded/linux/lib/xz/xz_lzma2.h
    xz-embedded/linux/lib/xz/xz_private.h
    xz-embedded/linux/lib/xz/xz_stream.h
    # xz-embedded/userspace/boottest.c
    # xz-embedded/userspace/buftest.c
    # xz-embedded/userspace/bytetest.c
    xz-embedded/userspace/xz_config.h
    # xz-embedded/userspace/xzminidec.c
)

target_include_directories(xz
    PUBLIC
        xz-embedded/linux/include
        xz-embedded/linux/include/linux
        xz-embedded/linux/include/linux/decompress
        xz-embedded/linux/lib/xz
        xz-embedded/userspace
)

target_compile_definitions(xz
    PUBLIC
        XZ_DEC_ANY_CHECK
        XZ_USE_CRC64
)
