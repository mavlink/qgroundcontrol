// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWIDGETPLATFORMSYSTEMTRAYICON_P_H
#define QWIDGETPLATFORMSYSTEMTRAYICON_P_H

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

#include <QtGui/qpa/qplatformsystemtrayicon.h>
#include <QtCore/private/qglobal_p.h>

QT_REQUIRE_CONFIG(systemtrayicon);

QT_BEGIN_NAMESPACE

class QSystemTrayIcon;

class QWidgetPlatformSystemTrayIcon : public QPlatformSystemTrayIcon
{
    Q_OBJECT

public:
    explicit QWidgetPlatformSystemTrayIcon(QObject *parent = nullptr);
    ~QWidgetPlatformSystemTrayIcon();

    void init() override;
    void cleanup() override;
    void updateIcon(const QIcon &icon) override;
    void updateToolTip(const QString &tooltip) override;
    void updateMenu(QPlatformMenu *menu) override;
    QRect geometry() const override;
    void showMessage(const QString &title, const QString &msg,
                     const QIcon &icon, MessageIcon iconType, int msecs) override;

    bool isSystemTrayAvailable() const override;
    bool supportsMessages() const override;

    QPlatformMenu *createMenu() const override;

private:
    QScopedPointer<QSystemTrayIcon> m_systray;
};

QT_END_NAMESPACE

#endif // QWIDGETPLATFORMSYSTEMTRAYICON_P_H
