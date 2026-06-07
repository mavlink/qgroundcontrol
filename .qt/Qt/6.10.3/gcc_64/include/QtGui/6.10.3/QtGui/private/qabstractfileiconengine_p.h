// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTFILEICONENGINE_P_H
#define QABSTRACTFILEICONENGINE_P_H

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

#include <QtCore/qfileinfo.h>
#include <private/qicon_p.h>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAbstractFileIconEngine : public QPixmapIconEngine
{
public:
    explicit QAbstractFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts)
        : QPixmapIconEngine(), m_fileInfo(info), m_options(opts) {}

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State, qreal scale) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    bool isNull() override { return false; }

    QFileInfo fileInfo() const { return m_fileInfo; }
    QPlatformTheme::IconOptions options() const { return m_options; }

    // Helper to convert a sequence of ints to a list of QSize
    template <class It> static QList<QSize> toSizeList(It i1, It i2);

protected:
    virtual QPixmap filePixmap(const QSize &size, QIcon::Mode mode, QIcon::State) = 0;
    virtual QString cacheKey() const;

private:
    const QFileInfo m_fileInfo;
    const QPlatformTheme::IconOptions m_options;
};

template <class It>
inline QList<QSize> QAbstractFileIconEngine::toSizeList(It i1, It i2)
{
    QList<QSize> result;
    result.reserve(int(i2 - i1));
    for ( ; i1 != i2; ++i1)
        result.append(QSize(*i1, *i1));
    return result;
}

QT_END_NAMESPACE

#endif // QABSTRACTFILEICONENGINE_P_H
