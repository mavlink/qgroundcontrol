// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKNATIVEICONLOADER_P_H
#define QQUICKNATIVEICONLOADER_P_H

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
#include <QtQuickTemplates2/private/qquickicon_p.h>

#include "qquicknativeicon_p.h"

QT_BEGIN_NAMESPACE

class QObject;

class QQuickNativeIconLoader : public QQuickPixmap
{
public:
    QQuickNativeIconLoader(int slot, QObject *parent);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    QIcon toQIcon() const;

    QQuickIcon icon() const;
    void setIcon(const QQuickIcon &icon);

private:
    void loadIcon();

    QObject *m_parent;
    int m_slot;
    bool m_enabled;
    QQuickIcon m_icon;
};

QT_END_NAMESPACE

#endif // QQUICKNATIVEICONLOADER_P_H
