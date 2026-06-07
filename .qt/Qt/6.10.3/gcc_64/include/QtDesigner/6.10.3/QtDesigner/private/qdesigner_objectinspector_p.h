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

#ifndef DESIGNEROBJECTINSPECTOR_H
#define DESIGNEROBJECTINSPECTOR_H

#include "shared_global_p.h"
#include <QtDesigner/abstractobjectinspector.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QDesignerDnDItemInterface;

namespace qdesigner_internal {

struct QDESIGNER_SHARED_EXPORT Selection {
    bool empty() const;
    void clear();

    // Merge all lists
    QObjectList selection() const;

    // Selection in cursor (managed widgets)
    QWidgetList managed;
    // Unmanaged widgets
    QWidgetList unmanaged;
    // Remaining selected objects (non-widgets)
    QObjectList objects;
};

// Extends the QDesignerObjectInspectorInterface by functionality
// to access the selection

class QDESIGNER_SHARED_EXPORT QDesignerObjectInspector: public QDesignerObjectInspectorInterface
{
    Q_OBJECT
public:
    explicit QDesignerObjectInspector(QWidget *parent = nullptr, Qt::WindowFlags flags = {});

    // Select a qobject unmanaged by form window
    virtual bool selectObject(QObject *o) = 0;
    virtual void getSelection(Selection &s) const = 0;
    virtual void clearSelection() = 0;

public slots:
    virtual void mainContainerChanged();
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // DESIGNEROBJECTINSPECTOR_H
