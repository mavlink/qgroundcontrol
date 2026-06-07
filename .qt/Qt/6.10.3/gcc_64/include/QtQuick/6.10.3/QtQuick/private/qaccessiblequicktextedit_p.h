// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLEQUICKTEXTEDIT_H
#define QACCESSIBLEQUICKTEXTEDIT_H

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

#include "qaccessiblequickitem_p.h"

#include <QtQuick/private/qquicktextedit_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class Q_QUICK_EXPORT QAccessibleQuickTextEdit : public QAccessibleQuickItem
{
public:
    QAccessibleQuickTextEdit(QQuickTextEdit *textEdit);

    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;

private:
    QQuickTextEdit *textEdit() const { return static_cast<QQuickTextEdit *>(item()); }
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKTEXTEDIT_H
