#pragma once

/// @file GzipCheck.h
/// @brief Checks the checksums of a gzip stream with zlib
/// @note Exists because libarchive does not verify the gzip trailer (libarchive/libarchive#3728)
/// @note Private to QGCCompression

#include <array>
#include <zlib.h>

#include <QtCore/QByteArray>

namespace LibArchive {

/// Checks the checksums of a gzip stream, which libarchive reads without checking them
/// It is given the bytes of the input in order and inflates them a second time, with zlib, which compares the
/// checksum and the length that end each member with what it inflated. The output is discarded.
class GzipCheck
{
    Q_DISABLE_COPY_MOVE(GzipCheck)

public:
    GzipCheck() = default;

    ~GzipCheck();

    /// Take the next bytes of the input
    void feed(const char* bytes, qint64 size);

    /// Whether the bytes given so far are whole members that match their checksums, and maybe other data after
    /// them
    bool isSound() const { return !_damaged && !_inMember; }

    /// Whether the bytes given so far end inside a member
    bool inMember() const { return _inMember && !_damaged; }

    /// Bytes that the members given so far inflate to
    qint64 inflatedSize() const { return _inflatedSize; }

private:
    static constexpr qsizetype kMemberStartSize = 4;

    bool _begin();

    /// Inflate bytes of the current member
    /// @return How many of them belong to it: the member may end before they do
    qint64 _inflate(const char* bytes, qint64 size);

    z_stream _stream = {};
    bool _initialized = false;
    bool _inMember = false;
    bool _damaged = false;
    bool _ended = false;  ///< Data that is no member has been given: the rest is not looked at
    QByteArray _start;    ///< The first bytes of what may be a member, until there are enough to tell
    qint64 _inflatedSize = 0;
    std::array<Bytef, std::size_t{32} * 1024> _discarded;
};

}  // namespace LibArchive
