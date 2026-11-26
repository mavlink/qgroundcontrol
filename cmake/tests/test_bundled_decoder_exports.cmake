cmake_minimum_required(VERSION 3.25)
# The Linux executable must not export the symbols of the decompression libraries it links statically
# (BundledDecoderExports.cmake), or the libraries loaded with it call them in place of their own zlib.
include("${CMAKE_CURRENT_LIST_DIR}/../GStreamer/tests/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../BundledDecoderExports.cmake")

# What QGroundControl exported when its zlib was linked without --exclude-libs, with the symbols of the application
set(_exported
    "0000000005e7dddb T crc32\n0000000005e84949 T inflate\n0000000005e844bf T inflateInit2_\n"
    "0000000005e7e595 T deflateInit_\n0000000000001000 T main\n0000000000002000 W _ZN7QObject5eventEP6QEvent\n"
    "0000000000003000 T lzma_code\n0000000000004000 T archive_read_new\n0000000000005000 T ZSTD_decompressStream\n"
    "0000000000006000 T LZ4F_decompress\n0000000000007000 T BZ2_bzDecompress\n0000000000008000 T gzopen@@ZLIB_1.2.0\n"
)
string(CONCAT _exported ${_exported})
qgc_bundled_decoder_exports("${_exported}" _found)
set(_decoder_symbols
    crc32
    inflate
    inflateInit2_
    deflateInit_
    lzma_code
    archive_read_new
    ZSTD_decompressStream
    LZ4F_decompress
    BZ2_bzDecompress
    gzopen
)
qgc_test_assert_streq("decoder symbols" "${_decoder_symbols}" "${_found}")

# The application's own symbols, also those whose names contain a decoder's name
set(_own "0000000000001000 T main\n0000000000002000 T qgcInflateTiles\n0000000000003000 T _Z8compressv\n")
qgc_bundled_decoder_exports("${_own}" _found)
qgc_test_assert_streq("application symbols" "" "${_found}")

qgc_test_pass("decoder export check finds the symbols of the bundled decompression libraries")
