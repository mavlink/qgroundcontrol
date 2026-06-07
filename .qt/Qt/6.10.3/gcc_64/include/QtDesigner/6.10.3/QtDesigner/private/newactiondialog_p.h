// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef NEWACTIONDIALOG_P_H
#define NEWACTIONDIALOG_P_H

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

#include "qdesigner_utils_p.h" //  PropertySheetIconValue

#include <QtWidgets/qdialog.h>
#include <QtGui/qkeysequence.h>
#include <QtCore/qcompare.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

namespace Ui {
    class NewActionDialog;
}

class ActionEditor;

struct ActionData {

    enum ChangeMask {
        TextChanged = 0x1, NameChanged = 0x2, ToolTipChanged = 0x4,
        IconChanged = 0x8, CheckableChanged = 0x10, KeysequenceChanged = 0x20,
        MenuRoleChanged = 0x40
    };

    // Returns a combination of ChangeMask flags
    unsigned compare(const  ActionData &rhs) const;

    QString text;
    QString name;
    QString toolTip;
    PropertySheetIconValue icon;
    bool checkable{false};
    PropertySheetKeySequenceValue keysequence;
    PropertySheetFlagValue menuRole;

    friend bool comparesEqual(const ActionData &lhs, const ActionData &rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(ActionData)
};

class NewActionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit NewActionDialog(ActionEditor *parent);
    ~NewActionDialog() override;

    ActionData actionData() const;
    void setActionData(const ActionData &d);

    QString actionText() const;
    QString actionName() const;

public slots:
    void focusName();
    void focusText();
    void focusTooltip();
    void focusShortcut();
    void focusCheckable();
    void focusMenuRole();

private slots:
    void onEditActionTextTextEdited(const QString &text);
    void onEditObjectNameTextEdited(const QString &text);

    void slotEditToolTip();
    void slotResetKeySequence();

private:
    Ui::NewActionDialog *m_ui;
    ActionEditor *m_actionEditor;
    bool m_autoUpdateObjectName;

    void updateButtons();
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // NEWACTIONDIALOG_P_H
