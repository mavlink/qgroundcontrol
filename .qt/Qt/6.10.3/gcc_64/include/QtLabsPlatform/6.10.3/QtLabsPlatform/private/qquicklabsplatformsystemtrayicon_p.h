// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMSYSTEMTRAYICON_P_H
#define QQUICKLABSPLATFORMSYSTEMTRAYICON_P_H

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

#include <QtCore/qurl.h>
#include <QtCore/qrect.h>
#include <QtGui/qpa/qplatformsystemtrayicon.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>

#include "qquicklabsplatformicon_p.h"

QT_REQUIRE_CONFIG(systemtrayicon);

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformMenu;
class QQuickLabsPlatformIconLoader;

class QQuickLabsPlatformSystemTrayIcon : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SystemTrayIcon)
    QML_EXTENDED_NAMESPACE(QPlatformSystemTrayIcon)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool available READ isAvailable CONSTANT FINAL)
    Q_PROPERTY(bool supportsMessages READ supportsMessages CONSTANT FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QString tooltip READ tooltip WRITE setTooltip NOTIFY tooltipChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenu *menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged FINAL REVISION(1, 1))
    Q_PROPERTY(QQuickLabsPlatformIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(1, 1))

public:
    explicit QQuickLabsPlatformSystemTrayIcon(QObject *parent = nullptr);
    ~QQuickLabsPlatformSystemTrayIcon();

    QPlatformSystemTrayIcon *handle() const;

    bool isAvailable() const;
    bool supportsMessages() const;

    bool isVisible() const;
    void setVisible(bool visible);

    QString tooltip() const;
    void setTooltip(const QString &tooltip);

    QQuickLabsPlatformMenu *menu() const;
    void setMenu(QQuickLabsPlatformMenu *menu);

    QRect geometry() const;

    QQuickLabsPlatformIcon icon() const;
    void setIcon(const QQuickLabsPlatformIcon &icon);

public Q_SLOTS:
    void show();
    void hide();

    void showMessage(const QString &title, const QString &message,
                     QPlatformSystemTrayIcon::MessageIcon iconType = QPlatformSystemTrayIcon::Information, int msecs = 10000);

Q_SIGNALS:
    void activated(QPlatformSystemTrayIcon::ActivationReason reason);
    void messageClicked();
    void visibleChanged();
    void tooltipChanged();
    void menuChanged();
    Q_REVISION(1, 1) void geometryChanged();
    Q_REVISION(1, 1) void iconChanged();

protected:
    void init();
    void cleanup();

    void classBegin() override;
    void componentComplete() override;

    QQuickLabsPlatformIconLoader *iconLoader() const;

private Q_SLOTS:
    void updateIcon();

private:
    bool m_complete;
    bool m_visible;
    QString m_tooltip;
    QQuickLabsPlatformMenu *m_menu;
    mutable QQuickLabsPlatformIconLoader *m_iconLoader;
    QPlatformSystemTrayIcon *m_handle;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMSYSTEMTRAYICON_P_H
