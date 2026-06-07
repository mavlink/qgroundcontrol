// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTOOLTIP_P_H
#define QTOOLTIP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qlayout*.cpp, and qabstractlayout.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QLabel>
#include <QString>
#include <QRect>
#include <QToolTip>

QT_REQUIRE_CONFIG(tooltip);
QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QTipLabel final : public QLabel
{
    Q_OBJECT
public:
    explicit QTipLabel(const QString &text, const QPoint &pos, QWidget *w, int msecDisplayTime);
    ~QTipLabel() override;

    void adjustTooltipScreen(const QPoint &pos);
    void updateSize(const QPoint &pos);

    bool eventFilter(QObject *, QEvent *) override;

    void reuseTip(const QString &text, int msecDisplayTime, const QPoint &pos);
    void hideTip();
    void hideTipImmediately();
    void setTipRect(QWidget *w, const QRect &r);
    void restartExpireTimer(int msecDisplayTime);
    bool tipChanged(const QPoint &pos, const QString &text, QObject *o);
    void placeTip(const QPoint &pos, QWidget *w);

    static QScreen *getTipScreen(const QPoint &pos, QWidget *w);
protected:
    void timerEvent(QTimerEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

#if QT_CONFIG(style_stylesheet)
public Q_SLOTS:
    void styleSheetParentDestroyed();

private:
    QWidget *styleSheetParent;
#endif

private:
    friend class QToolTip;

    static QTipLabel *instance;
    QBasicTimer hideTimer, expireTimer;
    QWidget *widget;
    QRect rect;
    bool fadingOut;
};

QT_END_NAMESPACE

#endif // QTOOLTIP_P_H
