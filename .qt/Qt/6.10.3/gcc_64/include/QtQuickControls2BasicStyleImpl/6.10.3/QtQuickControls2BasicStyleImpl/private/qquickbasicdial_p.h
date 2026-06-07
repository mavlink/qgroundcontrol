// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDEFAULTDIAL_P_H
#define QQUICKDEFAULTDIAL_P_H

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

class QQuickBasicDial : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress FINAL)
    Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle FINAL)
    Q_PROPERTY(qreal endAngle READ endAngle WRITE setEndAngle FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    QML_NAMED_ELEMENT(DialImpl)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickBasicDial(QQuickItem *parent = nullptr);

    qreal progress() const;
    void setProgress(qreal progress);

    qreal startAngle() const;
    void setStartAngle(qreal startAngle);

    qreal endAngle() const;
    void setEndAngle(qreal endAngle);

    QColor color() const;
    void setColor(const QColor &color);

    void paint(QPainter *painter) override;

private:
    qreal m_progress = 0;
    qreal m_startAngle = -140.;
    qreal m_endAngle = 140.;
    QColor m_color = Qt::black;
};

QT_END_NAMESPACE

#endif // QQUICKDEFAULTDIAL_P_H
