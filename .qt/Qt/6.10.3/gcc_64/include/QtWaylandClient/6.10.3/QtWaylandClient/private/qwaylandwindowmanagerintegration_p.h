// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDWINDOWMANAGERINTEGRATION_H
#define QWAYLANDWINDOWMANAGERINTEGRATION_H

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

#include <QtCore/QObject>

#include <QtWaylandClient/private/qwayland-qt-windowmanager.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;

class QWaylandWindowManagerIntegrationPrivate;

class Q_WAYLANDCLIENT_EXPORT QWaylandWindowManagerIntegration : public QtWayland::qt_windowmanager
{

public:
    explicit QWaylandWindowManagerIntegration(QWaylandDisplay *waylandDisplay, uint id,
                                              uint version);
    ~QWaylandWindowManagerIntegration();

    void openUrl(const QUrl &url);

    bool showIsFullScreen() const;

private:
    void windowmanager_hints(int32_t showIsFullScreen) override;
    void windowmanager_quit() override;

    bool m_showIsFullScreen = false;
};

QT_END_NAMESPACE

}

#endif // QWAYLANDWINDOWMANAGERINTEGRATION_H
