// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGGEOMETRY_P_H
#define QSGGEOMETRY_P_H

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

#include "qsggeometry.h"
#include "private/qglobal_p.h"

QT_BEGIN_NAMESPACE

class QSGGeometryData
{
public:
    virtual ~QSGGeometryData() {}

    static inline QSGGeometryData *data(const QSGGeometry *g) {
        return g->m_server_data;
    }

    static inline void install(const QSGGeometry *g, QSGGeometryData *data) {
        Q_ASSERT(!g->m_server_data);
        const_cast<QSGGeometry *>(g)->m_server_data = data;
    }

    static bool inline hasDirtyVertexData(const QSGGeometry *g) { return g->m_dirty_vertex_data; }
    static void inline clearDirtyVertexData(const QSGGeometry *g) { const_cast<QSGGeometry *>(g)->m_dirty_vertex_data = false; }

    static bool inline hasDirtyIndexData(const QSGGeometry *g) { return g->m_dirty_index_data; }
    static void inline clearDirtyIndexData(const QSGGeometry *g) { const_cast<QSGGeometry *>(g)->m_dirty_index_data = false; }

};

QT_END_NAMESPACE

#endif // QSGGEOMETRY_P_H
