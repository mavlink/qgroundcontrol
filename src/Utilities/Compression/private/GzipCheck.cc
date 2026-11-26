#include "GzipCheck.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace LibArchive {

GzipCheck::~GzipCheck()
{
    if (_initialized) {
        inflateEnd(&_stream);
    }
}

void GzipCheck::feed(const char* bytes, qint64 size)
{
    while ((size > 0) && !_damaged && !_ended) {
        if (_inMember) {
            const qint64 used = _inflate(bytes, size);
            if ((used == 0) && _inMember) {
                // zlib takes input as long as a member lasts
                _damaged = true;
            }
            bytes += used;
            size -= used;
            continue;
        }

        // A member starts with the magic number. Data after a member that does not is not gzip data, and ends the
        // stream as it does for libarchive. Data that does is a member, which must be whole and sound, also where
        // libarchive takes its header, truncated or with another method or reserved flags, for the end of the
        // stream: zlib refuses such a header
        const qint64 wanted = std::min<qint64>(size, kMemberStartSize - _start.size());
        _start.append(bytes, static_cast<qsizetype>(wanted));
        bytes += wanted;
        size -= wanted;
        if (_start.size() < kMemberStartSize) {
            return;
        }
        const QByteArray start = std::exchange(_start, {});
        const bool isMember = (start.at(0) == '\x1F') && (start.at(1) == '\x8B');
        if (!isMember || !_begin()) {
            _ended = true;
            return;
        }
        _inMember = true;
        (void) _inflate(start.constData(), start.size());
    }
}

bool GzipCheck::_begin()
{
    if (!_initialized) {
        // 16 is added to the window size for a gzip stream
        _initialized = (inflateInit2(&_stream, 16 + MAX_WBITS) == Z_OK);
    }
    return _initialized;
}

qint64 GzipCheck::_inflate(const char* bytes, qint64 size)
{
    const uInt given = static_cast<uInt>(std::min<qint64>(size, std::numeric_limits<uInt>::max()));
    _stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(bytes));
    _stream.avail_in = given;
    while (true) {
        _stream.next_out = _discarded.data();
        _stream.avail_out = static_cast<uInt>(_discarded.size());
        const int result = inflate(&_stream, Z_NO_FLUSH);
        _inflatedSize += static_cast<qint64>(_discarded.size()) - _stream.avail_out;
        if (result == Z_STREAM_END) {
            // zlib has compared the checksum and the length. Another member may follow
            _inMember = false;
            (void) inflateReset(&_stream);
            break;
        }
        if ((result != Z_OK) && (result != Z_BUF_ERROR)) {
            _damaged = true;
            break;
        }
        // More output is waiting only if this pass filled the buffer
        if ((_stream.avail_in == 0) && ((_stream.avail_out != 0) || (result == Z_BUF_ERROR))) {
            break;
        }
    }
    return given - _stream.avail_in;
}

}  // namespace LibArchive
