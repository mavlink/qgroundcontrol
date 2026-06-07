// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTUREFILEDATA_P_H
#define QTEXTUREFILEDATA_P_H

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

#include <QtGui/qtguiglobal.h>
#include <QSharedDataPointer>
#include <QLoggingCategory>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QDebug;

Q_DECLARE_LOGGING_CATEGORY(lcQtGuiTextureIO)

class QTextureFileDataPrivate;

class Q_GUI_EXPORT QTextureFileData
{
public:
    enum Mode { ByteArrayMode, ImageMode };

    QTextureFileData(Mode mode = ByteArrayMode);
    QTextureFileData(const QTextureFileData &other);
    QTextureFileData &operator=(const QTextureFileData &other);
    ~QTextureFileData();

    bool isNull() const;
    bool isValid() const;

    void clear();

    QByteArray data() const;
    void setData(const QByteArray &data);
    void setData(const QImage &image, int level = 0, int face = 0);

    int dataOffset(int level = 0, int face = 0) const;
    void setDataOffset(int offset, int level = 0, int face = 0);

    int dataLength(int level = 0, int face = 0) const;
    void setDataLength(int length, int level = 0, int face = 0);

    QByteArrayView getDataView(int level = 0, int face = 0) const;

    int numLevels() const;
    void setNumLevels(int num);

    int numFaces() const;
    void setNumFaces(int num);

    QSize size() const;
    void setSize(const QSize &size);

    quint32 glFormat() const;
    void setGLFormat(quint32 format);

    quint32 glInternalFormat() const;
    void setGLInternalFormat(quint32 format);

    quint32 glBaseInternalFormat() const;
    void setGLBaseInternalFormat(quint32 format);

    QByteArray logName() const;
    void setLogName(const QByteArray &name);

    QMap<QByteArray, QByteArray> keyValueMetadata() const;
    void setKeyValueMetadata(const QMap<QByteArray, QByteArray> &keyValues);

private:
    QSharedDataPointer<QTextureFileDataPrivate> d;
    friend Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QTextureFileData &d);
};

Q_DECLARE_TYPEINFO(QTextureFileData, Q_RELOCATABLE_TYPE);

Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QTextureFileData &d);

QT_END_NAMESPACE

#endif // QABSTRACTLAYOUTSTYLEINFO_P_H
