// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIMAGE_P_H
#define QIMAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qcolorspace.h>
#include <QtGui/qimage.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include <QtCore/qttypetraits.h>


QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcImageIo)

class QImageWriter;

struct Q_GUI_EXPORT QImageData {        // internal image data
    QImageData();
    ~QImageData();
    static QImageData *create(const QSize &size, QImage::Format format);
    static QImageData *create(uchar *data, int w, int h,  qsizetype bpl, QImage::Format format, bool readOnly, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr);

    static QImageData *get(QImage &img) noexcept { return img.d; }
    static const QImageData *get(const QImage &img) noexcept { return img.d; }

    QAtomicInt ref;

    int width;
    int height;
    int depth;
    qsizetype nbytes;               // number of bytes data
    qreal devicePixelRatio;
    QList<QRgb> colortable;
    uchar *data;
    QImage::Format format; // invariants: > Format_Invalid, < NImageFormats
    qsizetype bytes_per_line;
    int ser_no;               // serial number
    int detach_no;

    qreal  dpmx;                // dots per meter X (or 0)
    qreal  dpmy;                // dots per meter Y (or 0)
    QPoint  offset;           // offset in pixels

    uint own_data : 1;
    uint ro_data : 1;
    uint has_alpha_clut : 1;
    uint is_cached : 1;

    QImageCleanupFunction cleanupFunction;
    void* cleanupInfo;

    bool checkForAlphaPixels() const;

    // Convert the image in-place, minimizing memory reallocation
    // Return false if the conversion cannot be done in-place.
    bool convertInPlace(QImage::Format newFormat, Qt::ImageConversionFlags);

    QMap<QString, QString> text;

    bool doImageIO(const QImage *image, QImageWriter* io, int quality) const;

    QPaintEngine *paintEngine;

    QColorSpace colorSpace;

    struct ImageSizeParameters {
        qsizetype bytesPerLine;
        qsizetype totalSize;
        bool isValid() const { return bytesPerLine > 0 && totalSize > 0; }
    };
    static ImageSizeParameters calculateImageParameters(qsizetype width, qsizetype height, qsizetype depth);
};

inline QImageData::ImageSizeParameters
QImageData::calculateImageParameters(qsizetype width, qsizetype height, qsizetype depth)
{
    ImageSizeParameters invalid = { -1, -1 };
    if (height <= 0)
        return invalid;

    // calculate the size, taking care of overflows
    qsizetype bytes_per_line;
    if (qMulOverflow(width, depth, &bytes_per_line))
        return invalid;
    if (qAddOverflow(bytes_per_line, qsizetype(31), &bytes_per_line))
        return invalid;
    // bytes per scanline (must be multiple of 4)
    bytes_per_line = (bytes_per_line >> 5) << 2;    // can't overflow

    qsizetype total_size;
    if (qMulOverflow(height, bytes_per_line, &total_size))
        return invalid;
    qsizetype dummy;
    if (qMulOverflow(height, qsizetype(sizeof(uchar *)), &dummy))
        return invalid;                                 // why is this here?
    // Disallow images where width * depth calculations might overflow
    if (width > (INT_MAX - 31) / depth)
        return invalid;

    return { bytes_per_line, total_size };
}

typedef void (*Image_Converter)(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
typedef bool (*InPlace_Image_Converter)(QImageData *data, Qt::ImageConversionFlags);

extern Image_Converter qimage_converter_map[QImage::NImageFormats][QImage::NImageFormats];
extern InPlace_Image_Converter qimage_inplace_converter_map[QImage::NImageFormats][QImage::NImageFormats];

void convert_generic(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
void convert_generic_over_rgb64(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
bool convert_generic_inplace(QImageData *data, QImage::Format dst_format, Qt::ImageConversionFlags);
bool convert_generic_inplace_over_rgb64(QImageData *data, QImage::Format dst_format, Qt::ImageConversionFlags);
#if QT_CONFIG(raster_fp)
void convert_generic_over_rgba32f(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
bool convert_generic_inplace_over_rgba32f(QImageData *data, QImage::Format dst_format, Qt::ImageConversionFlags);
#endif

void dither_to_Mono(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags, bool fromalpha);

const uchar *qt_get_bitflip_array();
Q_GUI_EXPORT void qGamma_correct_back_to_linear_cs(QImage *image);

#if defined(_M_ARM) && defined(_MSC_VER) // QTBUG-42038
#pragma optimize("", off)
#endif
inline int qt_depthForFormat(QImage::Format format)
{
    int depth = 0;
    switch(format) {
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        Q_UNREACHABLE();
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        depth = 1;
        break;
    case QImage::Format_Indexed8:
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
        depth = 8;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
        depth = 32;
        break;
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_Grayscale16:
        depth = 16;
        break;
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
        depth = 24;
        break;
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
        depth = 64;
        break;
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        depth = 128;
        break;
    case QImage::Format_CMYK8888:
        depth = 32;
        break;
    }
    return depth;
}

#if defined(_M_ARM) && defined(_MSC_VER)
#pragma optimize("", on)
#endif

inline QImage::Format qt_opaqueVersion(QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB8565_Premultiplied:
        return  QImage::Format_RGB16;
    case QImage::Format_ARGB8555_Premultiplied:
        return QImage::Format_RGB555;
    case QImage::Format_ARGB6666_Premultiplied:
        return  QImage::Format_RGB666;
    case QImage::Format_ARGB4444_Premultiplied:
        return QImage::Format_RGB444;
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
        return QImage::Format_RGBX8888;
    case QImage::Format_A2BGR30_Premultiplied:
        return QImage::Format_BGR30;
    case QImage::Format_A2RGB30_Premultiplied:
        return QImage::Format_RGB30;
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
        return QImage::Format_RGBX64;
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
        return QImage::Format_RGBX16FPx4;
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return QImage::Format_RGBX32FPx4;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
        return QImage::Format_RGB32;
    case QImage::Format_RGB16:
    case QImage::Format_RGB32:
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
    case QImage::Format_RGBX8888:
    case QImage::Format_BGR30:
    case QImage::Format_RGB30:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_Grayscale8:
    case QImage::Format_Grayscale16:
    case QImage::Format_CMYK8888:
        return format;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
    case QImage::Format_Alpha8:
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        break;
    }
    return QImage::Format_RGB32;
}

inline QImage::Format qt_alphaVersion(QImage::Format format)
{
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        return QImage::Format_ARGB32_Premultiplied;
    case QImage::Format_RGB16:
        return QImage::Format_ARGB8565_Premultiplied;
    case QImage::Format_RGB555:
        return QImage::Format_ARGB8555_Premultiplied;
    case QImage::Format_RGB666:
        return QImage::Format_ARGB6666_Premultiplied;
    case QImage::Format_RGB444:
        return QImage::Format_ARGB4444_Premultiplied;
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        return QImage::Format_RGBA8888_Premultiplied;
    case QImage::Format_BGR30:
        return QImage::Format_A2BGR30_Premultiplied;
    case QImage::Format_RGB30:
        return QImage::Format_A2RGB30_Premultiplied;
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_Grayscale16:
        return QImage::Format_RGBA64_Premultiplied;
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
        return QImage::Format_RGBA16FPx4_Premultiplied;
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
        return QImage::Format_RGBA32FPx4_Premultiplied;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return format;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
    case QImage::Format_Invalid:
    case QImage::Format_CMYK8888:
    case QImage::NImageFormats:
        break;
    }
    return QImage::Format_ARGB32_Premultiplied;
}

// Returns an opaque version that is compatible with format
inline QImage::Format qt_maybeDataCompatibleOpaqueVersion(QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB6666_Premultiplied:
        return QImage::Format_RGB666;
    case QImage::Format_ARGB4444_Premultiplied:
        return QImage::Format_RGB444;
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
        return QImage::Format_RGBX8888;
    case QImage::Format_A2BGR30_Premultiplied:
        return QImage::Format_BGR30;
    case QImage::Format_A2RGB30_Premultiplied:
        return QImage::Format_RGB30;
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
        return QImage::Format_RGBX64;
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
        return QImage::Format_RGBX16FPx4;
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return QImage::Format_RGBX32FPx4;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
        return QImage::Format_RGB32;
    case QImage::Format_RGB16:
    case QImage::Format_RGB32:
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
    case QImage::Format_RGBX8888:
    case QImage::Format_BGR30:
    case QImage::Format_RGB30:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_Grayscale8:
    case QImage::Format_Grayscale16:
    case QImage::Format_CMYK8888:
        return format; // Already opaque
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_Alpha8:
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        break;
    }
    return format; // No compatible opaque versions
}

constexpr QImage::Format qt_toUnpremultipliedFormat(QImage::Format format)
{
    // Assumes input is already a premultiplied format with an unpremultiplied counterpart
    // This abuses the fact unpremultiplied formats are always before their premultiplied counterparts.
    return static_cast<QImage::Format>(qToUnderlying(format) - 1);
}

constexpr QImage::Format qt_toPremultipliedFormat(QImage::Format format)
{
    // Assumes input is already an unpremultiplied format
    // This abuses the fact unpremultiplied formats are always before their premultiplied counterparts.
    return static_cast<QImage::Format>(qToUnderlying(format) + 1);
}

inline bool qt_highColorPrecision(QImage::Format format, bool opaque = false)
{
    // Formats with higher color precision than ARGB32_Premultiplied.
    switch (format) {
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
        return !opaque;
    case QImage::Format_BGR30:
    case QImage::Format_RGB30:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_Grayscale16:
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return true;
    default:
        break;
    }
    return false;
}

inline bool qt_fpColorPrecision(QImage::Format format)
{
    switch (format) {
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return true;
    default:
        break;
    }
    return false;
}

inline QColorSpace::ColorModel qt_csColorData(QPixelFormat::ColorModel format)
{
    switch (format) {
    case QPixelFormat::ColorModel::RGB:
    case QPixelFormat::ColorModel::BGR:
    case QPixelFormat::ColorModel::Indexed:
        return QColorSpace::ColorModel::Rgb;
    case QPixelFormat::ColorModel::Alpha:
        return QColorSpace::ColorModel::Undefined; // No valid colors
    case QPixelFormat::ColorModel::Grayscale:
        return QColorSpace::ColorModel::Gray;
    case QPixelFormat::ColorModel::CMYK:
        return QColorSpace::ColorModel::Cmyk;
    default:
        break;
    }
    return QColorSpace::ColorModel::Undefined;
}

inline bool qt_compatibleColorModelBase(QPixelFormat::ColorModel data, QColorSpace::ColorModel cs)
{
    QColorSpace::ColorModel dataCs = qt_csColorData(data);

    if (data == QPixelFormat::ColorModel::Alpha)
        return true; // Alpha data has no colors and can be handled by any color space

    if (cs == QColorSpace::ColorModel::Undefined || dataCs == QColorSpace::ColorModel::Undefined)
        return false;

    return (dataCs == cs); // Matching color models
}

inline bool qt_compatibleColorModelSource(QPixelFormat::ColorModel data, QColorSpace::ColorModel cs)
{
    if (qt_compatibleColorModelBase(data, cs))
        return true;

    if (data == QPixelFormat::ColorModel::Grayscale && cs == QColorSpace::ColorModel::Rgb)
        return true; // Can apply Rgb CS to Gray input data

    return false;
}

inline bool qt_compatibleColorModelTarget(QPixelFormat::ColorModel data, QColorSpace::ColorModel cs, QColorSpace::TransformModel tm)
{
    if (qt_compatibleColorModelBase(data, cs))
        return true;

    if (data == QPixelFormat::ColorModel::Grayscale && tm == QColorSpace::TransformModel::ThreeComponentMatrix)
        return true; // Can apply three-component matrix CS to gray output

    return false;
}

inline QImage::Format qt_maybeDataCompatibleAlphaVersion(QImage::Format format)
{
    switch (format) {
    case QImage::Format_RGB32:
        return QImage::Format_ARGB32_Premultiplied;
    case QImage::Format_RGB666:
        return QImage::Format_ARGB6666_Premultiplied;
    case QImage::Format_RGB444:
        return QImage::Format_ARGB4444_Premultiplied;
    case QImage::Format_RGBX8888:
        return QImage::Format_RGBA8888_Premultiplied;
    case QImage::Format_BGR30:
        return QImage::Format_A2BGR30_Premultiplied;
    case QImage::Format_RGB30:
        return QImage::Format_A2RGB30_Premultiplied;
    case QImage::Format_RGBX64:
        return QImage::Format_RGBA64_Premultiplied;
    case QImage::Format_RGBX16FPx4:
        return QImage::Format_RGBA16FPx4_Premultiplied;
    case QImage::Format_RGBX32FPx4:
        return QImage::Format_RGBA32FPx4_Premultiplied;
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_Alpha8:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return format; // Already alpha versions
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
    case QImage::Format_Grayscale8:
    case QImage::Format_Grayscale16:
    case QImage::Format_CMYK8888:
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        break;
    }
    return format; // No data-compatible alpha version
}

inline QImage::Format qt_opaqueVersionForPainting(QImage::Format format)
{
    QImage::Format toFormat = qt_opaqueVersion(format);
    // If we are switching depth anyway upgrade to RGB32
    if (qt_depthForFormat(format) != qt_depthForFormat(toFormat) && qt_depthForFormat(toFormat) <= 32)
        toFormat = QImage::Format_RGB32;
    return toFormat;
}

inline QImage::Format qt_alphaVersionForPainting(QImage::Format format)
{
    QImage::Format toFormat = qt_alphaVersion(format);
#if defined(__ARM_NEON__) || defined(__SSE2__) || defined(QT_COMPILER_SUPPORT_LSX)
    // If we are switching depth anyway and we have optimized ARGB32PM routines, upgrade to that.
    if (qt_depthForFormat(format) != qt_depthForFormat(toFormat) && qt_depthForFormat(toFormat) <= 32)
        toFormat = QImage::Format_ARGB32_Premultiplied;
#endif
    return toFormat;
}

Q_GUI_EXPORT QMap<QString, QString> qt_getImageText(const QImage &image, const QString &description);
Q_GUI_EXPORT QMap<QString, QString> qt_getImageTextFromDescription(const QString &description);

QT_END_NAMESPACE

#endif // QIMAGE_P_H
