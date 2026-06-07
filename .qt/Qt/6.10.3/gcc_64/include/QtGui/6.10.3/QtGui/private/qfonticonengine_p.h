// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTICONENGINE_H
#define QFONTICONENGINE_H

#include <QtGui/qiconengine.h>

#ifndef QT_NO_ICON
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

#include <QtGui/qfont.h>

QT_BEGIN_NAMESPACE

using glyph_t = quint32;

class Q_GUI_EXPORT QFontIconEngine : public QIconEngine
{
public:
    QFontIconEngine(const QString &iconName, const QFont &font);
    ~QFontIconEngine();

    QString iconName() override;
    bool isNull() override;
    QString key() const override;
    QIconEngine *clone() const override;

    QList<QSize> availableSizes(QIcon::Mode, QIcon::State) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;

protected:
    virtual QString string() const;
    virtual glyph_t glyph() const;

private:
    static constexpr quint64 calculateCacheKey(QIcon::Mode mode, QIcon::State state)
    {
        return (quint64(mode) << 32) | state;
    }

    const QString m_iconName;
    const QFont m_iconFont;
    mutable QPixmap m_pixmap;
    mutable quint64 m_pixmapCacheKey = {};
    static constexpr glyph_t uninitializedGlyph = std::numeric_limits<glyph_t>::max();
    mutable glyph_t m_glyph = uninitializedGlyph;
};

QT_END_NAMESPACE

#endif // QT_NO_ICON

#endif // QFONTICONENGINE_H
