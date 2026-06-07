// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSTRAYICON_H
#define QDBUSTRAYICON_H

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

#include <QtGui/private/qtguiglobal_p.h>

QT_REQUIRE_CONFIG(systemtrayicon);

#include <QIcon>
#include <QTemporaryFile>
#include <QTimer>
#include "QtGui/qpa/qplatformsystemtrayicon.h"
#include "private/qdbusmenuconnection_p.h"

QT_BEGIN_NAMESPACE

class QStatusNotifierItemAdaptor;
class QDBusMenuAdaptor;
class QDBusPlatformMenu;
class QXdgNotificationInterface;

class QDBusTrayIcon: public QPlatformSystemTrayIcon
{
    Q_OBJECT
    Q_PROPERTY(QString category READ category NOTIFY categoryChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString tooltip READ tooltip NOTIFY tooltipChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconChanged)
    Q_PROPERTY(QIcon icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(bool isRequestingAttention READ isRequestingAttention NOTIFY attention)
    Q_PROPERTY(QString attentionTitle READ attentionTitle NOTIFY attention)
    Q_PROPERTY(QString attentionMessage READ attentionMessage NOTIFY attention)
    Q_PROPERTY(QString attentionIconName READ attentionIconName NOTIFY attention)
    Q_PROPERTY(QIcon attentionIcon READ attentionIcon NOTIFY attention)
    Q_PROPERTY(QDBusPlatformMenu *menu READ menu NOTIFY menuChanged)
    Q_MOC_INCLUDE(<private/qdbusplatformmenu_p.h>)

public:
    QDBusTrayIcon();

    virtual ~QDBusTrayIcon();

    QDBusMenuConnection * dBusConnection();

    void init() override;
    void cleanup() override;
    void updateIcon(const QIcon &icon) override;
    void updateToolTip(const QString &tooltip) override;
    void updateMenu(QPlatformMenu *menu) override;
    QPlatformMenu *createMenu() const override;
    void showMessage(const QString &title, const QString &msg,
                     const QIcon &icon, MessageIcon iconType, int msecs) override;

    bool isSystemTrayAvailable() const override;
    bool supportsMessages() const override { return true; }
    QRect geometry() const override { return QRect(); }

    QString category() const { return m_category; }
    QString status() const { return m_status; }
    QString tooltip() const { return m_tooltip; }

    QString iconName() const { return m_iconName; }
    const QIcon & icon() const { return m_icon; }

    bool isRequestingAttention() const { return m_attentionTimer.isActive(); }
    QString attentionTitle() const { return m_messageTitle; }
    QString attentionMessage() const { return m_message; }
    QString attentionIconName() const { return m_attentionIconName; }
    const QIcon & attentionIcon() const { return m_attentionIcon; }

    QString instanceId() const { return m_instanceId; }

    QDBusPlatformMenu *menu() { return m_menu; }

signals:
    void categoryChanged();
    void statusChanged(QString arg);
    void tooltipChanged();
    void iconChanged();
    void attention();
    void menuChanged();

private Q_SLOTS:
    void attentionTimerExpired();
    void actionInvoked(uint id, const QString &action);
    void notificationClosed(uint id, uint reason);
    void watcherServiceRegistered(const QString &serviceName);

private:
    void setStatus(const QString &status);
    QTemporaryFile *tempIcon(const QIcon &icon);

private:
    QDBusMenuConnection* m_dbusConnection;
    QStatusNotifierItemAdaptor *m_adaptor;
    QDBusMenuAdaptor *m_menuAdaptor;
    QDBusPlatformMenu *m_menu;
    QXdgNotificationInterface *m_notifier;
    QString m_instanceId;
    QString m_category;
    QString m_defaultStatus;
    QString m_status;
    QString m_tooltip;
    QString m_messageTitle;
    QString m_message;
    QIcon m_icon;
    QTemporaryFile *m_tempIcon;
    QString m_iconName;
    QIcon m_attentionIcon;
    QTemporaryFile *m_tempAttentionIcon;
    QString m_attentionIconName;
    QTimer m_attentionTimer;
    bool m_registered;
};

QT_END_NAMESPACE

#endif // QDBUSTRAYICON_H
