#pragma once

/// @file QGCCompressionFuzzTargets.h
/// @brief The two fuzz targets of QGCCompression, which the libFuzzer harnesses and the smoke test run
///
/// Each aborts when an invariant of the module does not hold for its input. An input larger than 64 KiB is passed
/// over.

#include <cstddef>
#include <cstdint>

namespace CompressionFuzz {

/// The input as a single stream in memory, which touches no file: decompressData() refuses it or gives what it
/// holds within the limit, input that is not compressed comes back unchanged, and in a build with the optional
/// decoders the rules that refuse a stream without its decoder agree with libarchive (BidAgreement.h)
int fuzzStream(const uint8_t* data, size_t size);

/// The input as files: its kind, the input of decompressFile() and readFile(), which give what decompressData()
/// gives, and an archive to extract, which writes nothing outside its output directory, no link, only portable and
/// distinct names, no more than the limit, and nothing at all when it fails
int fuzzFiles(const uint8_t* data, size_t size);

}  // namespace CompressionFuzz
