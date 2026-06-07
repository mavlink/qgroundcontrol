// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4SERIALIZE_P_H
#define QV4SERIALIZE_P_H

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

#include <QtCore/qbytearray.h>
#include <private/qv4value_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

class Serialize {
public:

    static QByteArray serialize(const Value &, ExecutionEngine *);
    static ReturnedValue deserialize(const QByteArray &, ExecutionEngine *);

private:
    static void serialize(QByteArray &, const Value &, ExecutionEngine *);
    static ReturnedValue deserialize(const char *&, ExecutionEngine *);
};

}

QT_END_NAMESPACE

#endif // QV8WORKER_P_H
