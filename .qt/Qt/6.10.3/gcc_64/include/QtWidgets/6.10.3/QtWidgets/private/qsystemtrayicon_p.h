// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSYSTEMTRAYICON_P_H
#define QSYSTEMTRAYICON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qsystemtrayicon.h"
#include "private/qobject_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

#if QT_CONFIG(menu)
#include "QtWidgets/qmenu.h"
#endif
#include "QtWidgets/qwidget.h"
#include "QtGui/qpixmap.h"
#include <qpa/qplatformsystemtrayicon.h>
#include "QtCore/qbasictimer.h"
#include "QtCore/qstring.h"
#include "QtCore/qpointer.h"

QT_BEGIN_NAMESPACE

class QSystemTrayIconSys;
class QSystemTrayWatcher;
class QPlatformSystemTrayIcon;
class QToolButton;
class QLabel;

class QSystemTrayIconPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSystemTrayIcon)

public:
    QSystemTrayIconPrivate();
    ~QSystemTrayIconPrivate();

    void install_sys();
    void remove_sys();
    void updateIcon_sys();
    void updateToolTip_sys();
    void updateMenu_sys();
    QRect geometry_sys() const;
    void showMessage_sys(const QString &title, const QString &msg, const QIcon &icon,
                         QSystemTrayIcon::MessageIcon msgIcon, int msecs);

    void destroyIcon();

    static bool isSystemTrayAvailable_sys();
    static bool supportsMessages_sys();

    void _q_emitActivated(QPlatformSystemTrayIcon::ActivationReason reason);

    QPointer<QMenu> menu;
    QIcon icon;
    QString toolTip;
    QSystemTrayIconSys *sys;
    QPlatformSystemTrayIcon *qpa_sys;
    bool visible;
    QSystemTrayWatcher *trayWatcher;

private:
    void install_sys_qpa();
    void remove_sys_qpa();

    void addPlatformMenu(QMenu *menu) const;
};

class QBalloonTip : public QWidget
{
    Q_OBJECT
public:
    static void showBalloon(const QIcon &icon, const QString &title,
                            const QString &msg, QSystemTrayIcon *trayIcon,
                            const QPoint &pos, int timeout, bool showArrow = true);
    static void hideBalloon();
    static bool isBalloonVisible();
    static void updateBalloonPosition(const QPoint& pos);

private:
    QBalloonTip(const QIcon &icon, const QString &title,
                const QString &msg, QSystemTrayIcon *trayIcon);
    ~QBalloonTip();
    void balloon(const QPoint&, int, bool);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void timerEvent(QTimerEvent *e) override;

private:
    QSystemTrayIcon *trayIcon;
    QPixmap pixmap;
    QBasicTimer timer;
    bool showArrow;
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON

#endif // QSYSTEMTRAYICON_P_H

