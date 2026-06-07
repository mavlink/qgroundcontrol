// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTMETRICS_H
#define QQUICKTEXTMETRICS_H

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
#include <private/qquicktext_p.h>

#include <QtCore/qobject.h>

#include <QtGui/qfontmetrics.h>

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QFont;

class Q_QUICK_EXPORT QQuickTextMetrics : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(qreal advanceWidth READ advanceWidth NOTIFY metricsChanged FINAL)
    Q_PROPERTY(QRectF boundingRect READ boundingRect NOTIFY metricsChanged FINAL)
    Q_PROPERTY(qreal width READ width NOTIFY metricsChanged FINAL)
    Q_PROPERTY(qreal height READ height NOTIFY metricsChanged FINAL)
    Q_PROPERTY(QRectF tightBoundingRect READ tightBoundingRect NOTIFY metricsChanged FINAL)
    Q_PROPERTY(QString elidedText READ elidedText NOTIFY metricsChanged FINAL)
    Q_PROPERTY(Qt::TextElideMode elide READ elide WRITE setElide NOTIFY elideChanged FINAL)
    Q_PROPERTY(qreal elideWidth READ elideWidth WRITE setElideWidth NOTIFY elideWidthChanged FINAL)
    Q_PROPERTY(QQuickText::RenderType renderType READ renderType WRITE setRenderType
               NOTIFY renderTypeChanged)
    QML_NAMED_ELEMENT(TextMetrics)
    QML_ADDED_IN_VERSION(2, 4)

public:
    explicit QQuickTextMetrics(QObject *parent = nullptr);

    QFont font() const;
    void setFont(const QFont &font);

    QString text() const;
    void setText(const QString &text);

    Qt::TextElideMode elide() const;
    void setElide(Qt::TextElideMode elide);

    qreal elideWidth() const;
    void setElideWidth(qreal elideWidth);

    qreal advanceWidth() const;
    QRectF boundingRect() const;
    qreal width() const;
    qreal height() const;
    QRectF tightBoundingRect() const;
    QString elidedText() const;

    QQuickText::RenderType renderType() const;
    void setRenderType(QQuickText::RenderType renderType);

Q_SIGNALS:
    void fontChanged();
    void textChanged();
    void elideChanged();
    void elideWidthChanged();
    void metricsChanged();
    void renderTypeChanged();

private:
    QString m_text;
    QFont m_font;
    QFontMetricsF m_metrics;
    Qt::TextElideMode m_elide;
    qreal m_elideWidth;
    QQuickText::RenderType m_renderType;
};

QT_END_NAMESPACE

#endif // QQUICKTEXTMETRICS_H
