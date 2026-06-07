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

#ifndef QDESIGNER_FORMWINDOMANAGER_H
#define QDESIGNER_FORMWINDOMANAGER_H

#include "shared_global_p.h"
#include <QtDesigner/abstractformwindowmanager.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class PreviewManager;

//
// Convenience methods to manage form previews (ultimately forwarded to PreviewManager).
//
class QDESIGNER_SHARED_EXPORT QDesignerFormWindowManager
    : public QDesignerFormWindowManagerInterface
{
    Q_OBJECT
public:
    explicit QDesignerFormWindowManager(QObject *parent = nullptr);
    ~QDesignerFormWindowManager() override;

    virtual PreviewManager *previewManager() const = 0;

    void showPluginDialog() override;

private:
    void *m_unused = nullptr;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_FORMWINDOMANAGER_H
