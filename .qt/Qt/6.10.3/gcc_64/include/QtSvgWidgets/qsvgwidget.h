// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGWIDGET_H
#define QSVGWIDGET_H

#include <QtSvgWidgets/qtsvgwidgetsglobal.h>
#include <QtWidgets/qwidget.h>


QT_BEGIN_NAMESPACE


class QSvgWidgetPrivate;
class QPaintEvent;
class QSvgRenderer;

class Q_SVGWIDGETS_EXPORT QSvgWidget : public QWidget
{
    Q_OBJECT
public:
    QSvgWidget(QWidget *parent = nullptr);
    QSvgWidget(const QString &file, QWidget *parent = nullptr);
    ~QSvgWidget();

    QSvgRenderer *renderer() const;

    QSize sizeHint() const override;

    QtSvg::Options options() const;
    void setOptions(QtSvg::Options options);
public Q_SLOTS:
    void load(const QString &file);
    void load(const QByteArray &contents);
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Q_DISABLE_COPY(QSvgWidget)
    Q_DECLARE_PRIVATE(QSvgWidget)
};

QT_END_NAMESPACE

#endif // QSVGWIDGET_H
