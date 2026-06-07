// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGSERVICEFACTORY_P_H
#define QQMLDEBUGSERVICEFACTORY_P_H

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

#include "qqmldebugservice_p.h"

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlDebugServiceFactory : public QObject
{
    Q_OBJECT
public:
    ~QQmlDebugServiceFactory() override;
    virtual QQmlDebugService *create(const QString &key) = 0;
};

#define QQmlDebugServiceFactory_iid "org.qt-project.Qt.QQmlDebugServiceFactory"

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVICEFACTORY_P_H
