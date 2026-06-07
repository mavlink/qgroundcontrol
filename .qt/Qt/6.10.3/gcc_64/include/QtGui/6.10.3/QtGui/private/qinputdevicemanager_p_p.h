// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QINPUTDEVICEMANAGER_P_P_H
#define QINPUTDEVICEMANAGER_P_P_H

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
#include <private/qobject_p.h>
#include "qinputdevicemanager_p.h"

#include <array>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QInputDeviceManagerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QInputDeviceManager)

public:
    static QInputDeviceManagerPrivate *get(QInputDeviceManager *mgr) { return mgr->d_func(); }

    int deviceCount(QInputDeviceManager::DeviceType type) const;
    void setDeviceCount(QInputDeviceManager::DeviceType type, int count);

    std::array<int, QInputDeviceManager::NumDeviceTypes> m_deviceCount = {};

    Qt::KeyboardModifiers keyboardModifiers;
};

QT_END_NAMESPACE

#endif // QINPUTDEVICEMANAGER_P_P_H
