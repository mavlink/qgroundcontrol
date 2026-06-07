// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSSLNAMESPACE_P_H
#define QQMLSSLNAMESPACE_P_H

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

#include <qtqmlnetworkexports.h>

#include <QtCore/QMetaType>
#include <QtQml/qqml.h>
#include <QtNetwork/QSsl>

QT_BEGIN_NAMESPACE

namespace QSslForeignNamespace
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QSsl)
    QML_NAMED_ELEMENT(Ssl)
    QML_ADDED_IN_VERSION(6, 7)
};

QT_END_NAMESPACE

#endif // QQMLSSLNAMESPACE_P_H
