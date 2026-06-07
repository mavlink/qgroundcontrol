// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFONTMETRICS_H
#define QQUICKFONTMETRICS_H

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

#include <private/qtquickglobal_p.h>

#include <QtQml/qqml.h>

#include <QtGui/qfontmetrics.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QFont;

class Q_QUICK_EXPORT QQuickFontMetrics : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(qreal ascent READ ascent NOTIFY fontChanged)
    Q_PROPERTY(qreal descent READ descent NOTIFY fontChanged)
    Q_PROPERTY(qreal height READ height NOTIFY fontChanged)
    Q_PROPERTY(qreal leading READ leading NOTIFY fontChanged)
    Q_PROPERTY(qreal lineSpacing READ lineSpacing NOTIFY fontChanged)
    Q_PROPERTY(qreal minimumLeftBearing READ minimumLeftBearing NOTIFY fontChanged)
    Q_PROPERTY(qreal minimumRightBearing READ minimumRightBearing NOTIFY fontChanged)
    Q_PROPERTY(qreal maximumCharacterWidth READ maximumCharacterWidth NOTIFY fontChanged)
    Q_PROPERTY(qreal xHeight READ xHeight NOTIFY fontChanged)
    Q_PROPERTY(qreal averageCharacterWidth READ averageCharacterWidth NOTIFY fontChanged)
    Q_PROPERTY(qreal underlinePosition READ underlinePosition NOTIFY fontChanged)
    Q_PROPERTY(qreal overlinePosition READ overlinePosition NOTIFY fontChanged)
    Q_PROPERTY(qreal strikeOutPosition READ strikeOutPosition NOTIFY fontChanged)
    Q_PROPERTY(qreal lineWidth READ lineWidth NOTIFY fontChanged)
    Q_PROPERTY(qreal capitalHeight READ capitalHeight NOTIFY fontChanged REVISION (6,9))
    QML_NAMED_ELEMENT(FontMetrics)
    QML_ADDED_IN_VERSION(2, 4)
public:
    explicit QQuickFontMetrics(QObject *parent = nullptr);

    QFont font() const;
    void setFont(const QFont &font);

    qreal ascent() const;
    qreal descent() const;
    qreal capitalHeight() const;
    qreal height() const;
    qreal leading() const;
    qreal lineSpacing() const;
    qreal minimumLeftBearing() const;
    qreal minimumRightBearing() const;
    qreal maximumCharacterWidth() const;

    qreal xHeight() const;
    qreal averageCharacterWidth() const;

    qreal underlinePosition() const;
    qreal overlinePosition() const;
    qreal strikeOutPosition() const;
    qreal lineWidth() const;

    Q_INVOKABLE qreal advanceWidth(const QString &text) const;
    Q_INVOKABLE QRectF boundingRect(const QString &text) const;
    Q_INVOKABLE QRectF tightBoundingRect(const QString &text) const;
    Q_INVOKABLE QString elidedText(const QString &text, Qt::TextElideMode mode, qreal width, int flags = 0) const;

Q_SIGNALS:
    void fontChanged(const QFont &font);

private:
    QFont m_font;
    QFontMetricsF m_metrics;
};

QT_END_NAMESPACE

#endif // QQUICKFONTMETRICS_H
