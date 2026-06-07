// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDCLIPBOARD_H
#define QWAYLANDCLIPBOARD_H

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

#include <qpa/qplatformclipboard.h>
#include <QtCore/QVariant>
#include <QtCore/QMimeData>

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtCore/private/qglobal_p.h>

QT_REQUIRE_CONFIG(clipboard);

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;

class Q_WAYLANDCLIENT_EXPORT QWaylandClipboard : public QPlatformClipboard
{
public:
    QWaylandClipboard(QWaylandDisplay *display);

    ~QWaylandClipboard() override;

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

private:
    QWaylandDisplay *mDisplay = nullptr;
    QMimeData m_emptyData;
    QMimeData *m_clientClipboard[2];
};

}

QT_END_NAMESPACE

#endif // QWAYLANDCLIPBOARD_H
