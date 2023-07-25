
set(XZ_EMBEDDED_DIR ${CMAKE_SOURCE_DIR}/libs/xz-embedded)

add_library(compression
	QGCLZMA.cc
	QGCLZMA.h
	QGCZlib.cc
	QGCZlib.h
	
	${XZ_EMBEDDED_DIR}/linux/include/linux/xz.h
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_lzma2.h
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_private.h
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_stream.h
	${XZ_EMBEDDED_DIR}/userspace/xz_config.h
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_crc32.c
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_crc64.c
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_dec_lzma2.c
	${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_dec_stream.c
)

target_compile_definitions(compression PRIVATE XZ_DEC_ANY_CHECK XZ_USE_CRC64)

target_link_libraries(compression
	Qt5::Core

	qgc
)

target_include_directories(compression
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
		${XZ_EMBEDDED_DIR}/userspace
		${XZ_EMBEDDED_DIR}/linux/include/linux
)

