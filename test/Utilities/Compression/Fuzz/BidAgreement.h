#pragma once

/// @file BidAgreement.h
/// @brief Compares the bid rules that QGCCompression copies from libarchive with libarchive itself
///
/// A build without the decoder of a legacy lzma, bzip2, LZ4 or Zstandard stream refuses such a stream by rules that
/// copy those of libarchive's bidders (LibArchive::undecodableStreamReason()). Only a build with the decoders can ask
/// libarchive itself, which happens here: the rules must refuse every stream that libarchive takes, and a legacy
/// lzma or bzip2 stream, which has no magic number to go by, only those.

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include "Formats.h"

namespace BidAgreement {

/// The optional filters that this build can compare: those whose decoder it has
QList<Compression::OptionalFormat> comparableFilters();

/// Whether libarchive, with only @a filter registered, takes @a input for a stream of it
bool libarchiveBids(Compression::OptionalFormat filter, const QByteArray& input);

/// Whether the copied rules refuse @a input as a stream of @a filter, in a build that has every decoder but its own
bool rulesRefuse(Compression::OptionalFormat filter, const QByteArray& input);

/// How the copied rules and libarchive disagree on @a input, for every comparable filter, or an empty string
QString disagreement(const QByteArray& input);

/// Inputs on the edges of the copied rules: properties and dictionary sizes of legacy lzma headers, bzip2 block
/// sizes and markers, and Zstandard and LZ4 frames behind skippable frames
QList<QByteArray> edgeInputs();

}  // namespace BidAgreement
