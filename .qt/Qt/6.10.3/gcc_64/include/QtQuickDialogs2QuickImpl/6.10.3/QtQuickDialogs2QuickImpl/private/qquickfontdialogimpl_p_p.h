// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFONTDIALOGIMPL_P_P_H
#define QQUICKFONTDIALOGIMPL_P_P_H

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

#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

#include "qquickfontdialogimpl_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickFontDialogImplPrivate : public QQuickDialogPrivate
{
    Q_DECLARE_PUBLIC(QQuickFontDialogImpl)
public:
    QQuickFontDialogImplPrivate();

    static QQuickFontDialogImplPrivate *get(QQuickFontDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickFontDialogImplAttached *attachedOrWarn();

    void updateEnabled();

    void handleAccept() override;
    void handleClick(QQuickAbstractButton *button) override;

    QSharedPointer<QFontDialogOptions> options;

    QFont currentFont;
};

class QQuickFontDialogImplAttachedPrivate : public QObjectPrivate
{
    void currentFontChanged(const QFont &font);

public:
    Q_DECLARE_PUBLIC(QQuickFontDialogImplAttached)

    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickListView> familyListView;
    QPointer<QQuickListView> styleListView;
    QPointer<QQuickListView> sizeListView;
    QPointer<QQuickTextEdit> sampleEdit;
    QPointer<QQuickComboBox> writingSystemComboBox;
    QPointer<QQuickCheckBox> underlineCheckBox;
    QPointer<QQuickCheckBox> strikeoutCheckBox;
    QPointer<QQuickTextField> familyEdit;
    QPointer<QQuickTextField> styleEdit;
    QPointer<QQuickTextField> sizeEdit;
};

QT_END_NAMESPACE

#endif // QQUICKFONTDIALOGIMPL_P_P_H
