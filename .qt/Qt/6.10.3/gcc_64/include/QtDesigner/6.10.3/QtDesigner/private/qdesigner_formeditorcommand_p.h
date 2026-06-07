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

#ifndef QDESIGNER_FORMEDITORCOMMAND_H
#define QDESIGNER_FORMEDITORCOMMAND_H

#include "shared_global_p.h"

#include <QtGui/qundostack.h>

#include <QtCore/qpointer.h>


QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT QDesignerFormEditorCommand: public QUndoCommand
{

public:
    QDesignerFormEditorCommand(const QString &description, QDesignerFormEditorInterface *core);

protected:
    QDesignerFormEditorInterface *core() const;

private:
    QPointer<QDesignerFormEditorInterface> m_core;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_FORMEDITORCOMMAND_H
