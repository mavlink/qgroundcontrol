// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMICONLOADER_P_H
#define QQUICKLABSPLATFORMICONLOADER_P_H

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
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtQuick/private/qquickpixmap_p.h>

#include "qquicklabsplatformicon_p.h"

QT_BEGIN_NAMESPACE

class QObject;

class QQuickLabsPlatformIconLoader : public QQuickPixmap
{
public:
    QQuickLabsPlatformIconLoader(int slot, QObject *parent);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    QIcon toQIcon() const;

    QQuickLabsPlatformIcon icon() const;
    void setIcon(const QQuickLabsPlatformIcon &icon);

private:
    void loadIcon();

    QObject *m_parent;
    int m_slot;
    bool m_enabled;
    QQuickLabsPlatformIcon m_icon;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMICONLOADER_P_H
