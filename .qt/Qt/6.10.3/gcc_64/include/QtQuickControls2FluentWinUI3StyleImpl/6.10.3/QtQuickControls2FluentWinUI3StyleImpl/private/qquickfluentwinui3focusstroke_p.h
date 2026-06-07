
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQUICKFLUENTWINUI3FOCUSSTROKE_P_H
#define QQUICKFLUENTWINUI3FOCUSSTROKE_P_H
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
#include <QtGui/qcolor.h>
#include <QtQuick/qquickpainteditem.h>
#include <QtCore/private/qglobal_p.h>
QT_BEGIN_NAMESPACE
class QQuickFluentWinUI3FocusStroke : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    Q_PROPERTY(int radius READ radius WRITE setRadius FINAL)
    QML_NAMED_ELEMENT(FocusStroke)
    QML_ADDED_IN_VERSION(6, 8)
public:
    explicit QQuickFluentWinUI3FocusStroke(QQuickItem *parent = nullptr);
    int radius() const;
    void setRadius(int radius);
    QColor color() const;
    void setColor(const QColor &color);
    void paint(QPainter *painter) override;
private:
    QColor m_color = Qt::white;
    int m_radius;
};
QT_END_NAMESPACE
#endif // QQUICKFLUENTWINUI3FOCUSSTROKE_P_H
