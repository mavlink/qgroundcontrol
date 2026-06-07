// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTINTERFACE_P_H
#define QQUICKTEXTINTERFACE_P_H

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

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickTextInterface
{
public:
    virtual void invalidate() = 0;
};

#define QQuickTextInterface_iid "org.qt-project.Qt.QQuickTextInterface"
Q_DECLARE_INTERFACE(QQuickTextInterface, QQuickTextInterface_iid)

QT_END_NAMESPACE

#endif // QQUICKTEXTINTERFACE_P_H
