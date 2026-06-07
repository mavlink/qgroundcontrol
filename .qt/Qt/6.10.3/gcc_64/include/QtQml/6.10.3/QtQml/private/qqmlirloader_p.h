// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLIRLOADER_P_H
#define QQMLIRLOADER_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmljsmemorypool_p.h>

QT_BEGIN_NAMESPACE

namespace QmlIR {
struct Document;
struct Object;
}

struct Q_QML_EXPORT QQmlIRLoader {
    QQmlIRLoader(const QV4::CompiledData::Unit *unit, QmlIR::Document *output);

    void load();

private:
    QmlIR::Object *loadObject(const QV4::CompiledData::Object *serializedObject);

    template <typename _Tp> _Tp *New() { return pool->New<_Tp>(); }

    const QV4::CompiledData::Unit *unit;
    QmlIR::Document *output;
    QQmlJS::MemoryPool *pool;
};

QT_END_NAMESPACE

#endif // QQMLIRLOADER_P_H
