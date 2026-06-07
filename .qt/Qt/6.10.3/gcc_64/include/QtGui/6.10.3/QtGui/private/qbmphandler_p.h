// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QBMPHANDLER_P_H
#define QBMPHANDLER_P_H

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
#include "QtGui/qimageiohandler.h"

#ifndef QT_NO_IMAGEFORMAT_BMP

QT_BEGIN_NAMESPACE

struct BMP_FILEHDR {                     // BMP file header
    char   bfType[2];                    // "BM"
    qint32  bfSize;                      // size of file
    qint16  bfReserved1;
    qint16  bfReserved2;
    qint32  bfOffBits;                   // pointer to the pixmap bits
};

struct BMP_INFOHDR {                     // BMP information header
    qint32  biSize;                      // size of this struct
    qint32  biWidth;                     // pixmap width
    qint32  biHeight;                    // pixmap height
    qint16  biPlanes;                    // should be 1
    qint16  biBitCount;                  // number of bits per pixel
    qint32  biCompression;               // compression method
    qint32  biSizeImage;                 // size of image
    qint32  biXPelsPerMeter;             // horizontal resolution
    qint32  biYPelsPerMeter;             // vertical resolution
    qint32  biClrUsed;                   // number of colors used
    qint32  biClrImportant;              // number of important colors
    // V4:
    quint32 biRedMask;
    quint32 biGreenMask;
    quint32 biBlueMask;
    quint32 biAlphaMask;
    qint32 biCSType;
    qint32 biEndpoints[9];
    qint32 biGammaRed;
    qint32 biGammaGreen;
    qint32 biGammaBlue;
    // V5:
    qint32 biIntent;
    qint32 biProfileData;
    qint32 biProfileSize;
    qint32 biReserved;
};

// BMP-Handler, which is also able to read and write the DIB
// (Device-Independent-Bitmap) format used internally in the Windows operating
// system for OLE/clipboard operations. DIB is a subset of BMP (without file
// header). The Windows platform plugin accesses the DIB-functionality.

class QBmpHandler : public QImageIOHandler
{
public:
    enum InternalFormat {
        DibFormat,
        BmpFormat
    };

    explicit QBmpHandler(InternalFormat fmt = BmpFormat);
    bool canRead() const override;
    bool read(QImage *image) override;
    bool write(const QImage &image) override;

    static bool canRead(QIODevice *device);

    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant &value) override;
    bool supportsOption(ImageOption option) const override;

private:
    bool readHeader();
    inline QByteArray formatName() const;

    enum State {
        Ready,
        ReadHeader,
        Error
    };

    const InternalFormat m_format;

    State state;
    BMP_FILEHDR fileHeader;
    BMP_INFOHDR infoHeader;
    qint64 startpos;
};

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_BMP

#endif // QBMPHANDLER_P_H
