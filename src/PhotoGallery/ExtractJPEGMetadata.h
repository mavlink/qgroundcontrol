#pragma once

#include <map>

#include <QString>

/// Extracts metadata from given image file (assuming JPEG -- otherwise nothing
/// will be extracted). Results in key/value map for metadata tags to their
/// values.
/// Metadata is represented using e.g. "Exif:Make" for exif tags, and e.g.
/// "PX4:flight_uid" for xmp-namespaced tags as keys.
std::map<QString, QString> extractJPEGMetadata(
        const QByteArray& image_data);
