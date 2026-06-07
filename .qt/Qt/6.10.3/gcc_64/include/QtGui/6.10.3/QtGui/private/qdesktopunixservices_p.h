// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:execute-external-code

#ifndef QDESKTOPUNIXSERVICES_H
#define QDESKTOPUNIXSERVICES_H

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

#include <qpa/qplatformservices.h>

#include <QtCore/qpointer.h>
#include <QtCore/QString>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QDBusPendingCallWatcher;
class QWindow;
class QDBusServiceWatcher;

class Q_GUI_EXPORT QDesktopUnixServices : public QPlatformServices
{
public:
    QDesktopUnixServices();
    ~QDesktopUnixServices();

    QByteArray desktopEnvironment() const override;

    bool hasCapability(Capability capability) const override;
    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;
    QPlatformServiceColorPicker *colorPicker(QWindow *parent = nullptr) override;

    void setApplicationBadge(qint64 number);
    virtual QString portalWindowIdentifier(QWindow *window);

    virtual void registerDBusMenuForWindow(QWindow *window, const QString &service, const QString &path);
    virtual void unregisterDBusMenuForWindow(QWindow *window);

private:
    QString m_webBrowser;
    QString m_documentLauncher;
#if QT_CONFIG(dbus)
    QPointer<QDBusPendingCallWatcher> m_watcher = nullptr;
    std::unique_ptr<QDBusServiceWatcher> m_portalWatcher;
#endif
    bool m_hasScreenshotPortalWithColorPicking = false;
};

QT_END_NAMESPACE

#endif // QDESKTOPUNIXSERVICES_H
