#include "GstCapsToMediaMetaData.h"

#include <QtCore/QSize>
#include <QtMultimedia/QMediaFormat>
#include <gst/gst.h>

// ─── Codec name table ────────────────────────────────────────────────────────
// Maps GstStructure media type prefix → QMediaFormat::VideoCodec

static constexpr struct
{
    const char* gstName;
    QMediaFormat::VideoCodec qtCodec;
} kCodecMap[] = {
    {"video/x-h264", QMediaFormat::VideoCodec::H264},
    {"video/x-h265", QMediaFormat::VideoCodec::H265},
    {"video/x-av1",  QMediaFormat::VideoCodec::AV1},
    {"video/x-vp8",  QMediaFormat::VideoCodec::VP8},
    {"video/x-vp9",  QMediaFormat::VideoCodec::VP9},
    {"video/mpeg",   QMediaFormat::VideoCodec::MPEG4},
    {"image/jpeg",   QMediaFormat::VideoCodec::MotionJPEG},
};

QMediaMetaData gstStructureToMediaMetaData(const GstStructure* structure)
{
    QMediaMetaData meta;
    if (!structure)
        return meta;

    // ── Resolution ───────────────────────────────────────────────────
    gint width = 0, height = 0;
    if (gst_structure_get_int(structure, "width", &width) &&
        gst_structure_get_int(structure, "height", &height) &&
        width > 0 && height > 0) {
        meta.insert(QMediaMetaData::Resolution, QSize(width, height));
    }

    // ── Frame rate ───────────────────────────────────────────────────
    gint fpsN = 0, fpsD = 1;
    if (gst_structure_get_fraction(structure, "framerate", &fpsN, &fpsD) &&
        fpsD > 0 && fpsN > 0) {
        meta.insert(QMediaMetaData::VideoFrameRate,
                    static_cast<qreal>(fpsN) / static_cast<qreal>(fpsD));
    }

    // ── Video codec ──────────────────────────────────────────────────
    const gchar* mediaType = gst_structure_get_name(structure);
    if (mediaType) {
        for (const auto& entry : kCodecMap) {
            if (g_str_has_prefix(mediaType, entry.gstName)) {
                meta.insert(QMediaMetaData::VideoCodec,
                            QVariant::fromValue(entry.qtCodec));
                break;
            }
        }
    }

    return meta;
}
