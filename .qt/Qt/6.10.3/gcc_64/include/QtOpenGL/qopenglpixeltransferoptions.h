// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLPIXELUPLOADOPTIONS_H
#define QOPENGLPIXELUPLOADOPTIONS_H

#include <QtOpenGL/qtopenglglobal.h>

#if !defined(QT_NO_OPENGL)

#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE

class QOpenGLPixelTransferOptionsData;

class Q_OPENGL_EXPORT QOpenGLPixelTransferOptions
{
public:
    QOpenGLPixelTransferOptions();
    QOpenGLPixelTransferOptions(const QOpenGLPixelTransferOptions &);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QOpenGLPixelTransferOptions)
    QOpenGLPixelTransferOptions &operator=(const QOpenGLPixelTransferOptions &);
    ~QOpenGLPixelTransferOptions();

    void swap(QOpenGLPixelTransferOptions &other) noexcept
    { data.swap(other.data); }

    void setAlignment(int alignment);
    int alignment() const;

    void setSkipImages(int skipImages);
    int skipImages() const;

    void setSkipRows(int skipRows);
    int skipRows() const;

    void setSkipPixels(int skipPixels);
    int skipPixels() const;

    void setImageHeight(int imageHeight);
    int imageHeight() const;

    void setRowLength(int rowLength);
    int rowLength() const;

    void setLeastSignificantByteFirst(bool lsbFirst);
    bool isLeastSignificantBitFirst() const;

    void setSwapBytesEnabled(bool swapBytes);
    bool isSwapBytesEnabled() const;

private:
    QSharedDataPointer<QOpenGLPixelTransferOptionsData> data;
};

Q_DECLARE_SHARED(QOpenGLPixelTransferOptions)

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLPIXELUPLOADOPTIONS_H
