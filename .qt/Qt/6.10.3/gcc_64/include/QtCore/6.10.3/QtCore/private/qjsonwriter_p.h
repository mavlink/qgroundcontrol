// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QJSONWRITER_P_H
#define QJSONWRITER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <qjsonvalue.h>

QT_BEGIN_NAMESPACE

namespace QJsonPrivate
{

class Writer
{
public:
    static void objectToJson(const QCborContainerPrivate *o, QByteArray &json, int indent, bool compact = false);
    static void arrayToJson(const QCborContainerPrivate *a, QByteArray &json, int indent, bool compact = false);
    static void valueToJson(const QCborValue &v, QByteArray &json, int indent, bool compact = false);
};

}

QT_END_NAMESPACE

#endif
