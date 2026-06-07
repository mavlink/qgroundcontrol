// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef DESIGNERINTROSPECTION
#define DESIGNERINTROSPECTION

#include "shared_global_p.h"
#include <abstractintrospection_p.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

struct QMetaObject;
class QWidget;

namespace qdesigner_internal {
    // Qt C++ introspection with helpers to find core and meta object for an object
    class QDESIGNER_SHARED_EXPORT QDesignerIntrospection : public QDesignerIntrospectionInterface {
    public:
        QDesignerIntrospection();
        ~QDesignerIntrospection() override;

        const QDesignerMetaObjectInterface* metaObject(const QObject *object) const override;

        const QDesignerMetaObjectInterface* metaObjectForQMetaObject(const QMetaObject *metaObject) const;

    private:
        mutable QHash<const QMetaObject *, QDesignerMetaObjectInterface *> m_metaObjectMap;
    };
}

QT_END_NAMESPACE

#endif // DESIGNERINTROSPECTION
