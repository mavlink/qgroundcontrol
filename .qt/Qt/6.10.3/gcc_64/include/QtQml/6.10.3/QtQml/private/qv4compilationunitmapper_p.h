// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4COMPILATIONUNITMAPPER_H
#define QV4COMPILATIONUNITMAPPER_H

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

#include <private/qv4global_p.h>
#include <QFile>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace CompiledData {
struct Unit;
}

class CompilationUnitMapper
{
public:
    CompiledData::Unit *get(
            const QString &cacheFilePath, const QDateTime &sourceTimeStamp, QString *errorString);
    static void invalidate(const QString &cacheFilePath);

private:
    CompiledData::Unit *open(
            const QString &cacheFilePath, const QDateTime &sourceTimeStamp, QString *errorString);
    void close();

#if defined(Q_OS_UNIX)
    size_t length = 0;
#endif
    void *dataPtr = nullptr;
};

}

QT_END_NAMESPACE

#endif // QV4COMPILATIONUNITMAPPER_H
