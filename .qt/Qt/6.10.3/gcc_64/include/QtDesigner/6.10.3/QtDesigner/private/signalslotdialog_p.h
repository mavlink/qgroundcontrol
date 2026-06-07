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

#ifndef _SIGNALSLOTDIALOG_H
#define _SIGNALSLOTDIALOG_H

#include "shared_global_p.h"
#include <QtCore/qstringlist.h>
#include <QtWidgets/qdialog.h>
#include <QtGui/qstandarditemmodel.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;
class QDesignerDialogGuiInterface;
class QDesignerMemberSheet;
class QListView;
class QToolButton;
class QItemSelection;

namespace Ui {
    class SignalSlotDialogClass;
}

namespace qdesigner_internal {

// Dialog data
struct SignalSlotDialogData {
    void clear();
    QStringList m_existingMethods;
    QStringList m_fakeMethods;
};

// Internal helper class: A model for signatures that allows for verifying duplicates
// (checking signals versus slots and vice versa).
class SignatureModel : public QStandardItemModel {
    Q_OBJECT

public:
    SignatureModel(QObject *parent = nullptr);
    bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

signals:
    void checkSignature(const QString &signature, bool *ok);
};

// Internal helper class: Panel for editing method signatures. List view with validator,
// add and remove button
class SignaturePanel  : public QObject {
     Q_OBJECT

public:
    SignaturePanel(QObject *parent, QListView *listView, QToolButton *addButton, QToolButton *removeButton, const QString &newPrefix);

    QStringList fakeMethods() const;
    void setData(const SignalSlotDialogData &d);
    int count(const QString &signature) const;

signals:
    void checkSignature(const QString &signature, bool *ok);

private slots:
    void slotAdd();
    void slotRemove();
    void slotSelectionChanged(const QItemSelection &, const QItemSelection &);

private:
    void closeEditor();

    const QString m_newPrefix;
    SignatureModel *m_model;
    QListView *m_listView;
    QToolButton *m_removeButton;
};

// Dialog for  editing signals and slots.
// Provides static convenience function
// to modify fake signals and slots. They are
// handled in 2 ways:
// 1) For the MainContainer: Fake signals and slots are stored
//    in the meta database (per-instance)
// 2) For promoted widgets: Fake signals and slots are stored
//    in the widget database (per-class)
// Arguably, we could require the MainContainer to be promoted for that, too,
// but that would require entering a header.

class QDESIGNER_SHARED_EXPORT SignalSlotDialog : public QDialog {
    Q_OBJECT

public:
    enum FocusMode { FocusSlots, FocusSignals };

    explicit SignalSlotDialog(QDesignerDialogGuiInterface *dialogGui, QWidget *parent = nullptr, FocusMode m = FocusSlots);
    ~SignalSlotDialog() override;

    DialogCode showDialog(SignalSlotDialogData &slotData, SignalSlotDialogData &signalData);

    // Edit fake methods stored in MetaDataBase (per instance, used for main containers)
    static bool editMetaDataBase(QDesignerFormWindowInterface *fw, QObject *object, QWidget *parent = nullptr, FocusMode m = FocusSlots);

    // Edit fake methods of a promoted class stored in WidgetDataBase (synthesizes a widget to obtain existing members).
    static bool editPromotedClass(QDesignerFormEditorInterface *core, const QString &promotedClassName, QWidget *parent = nullptr, FocusMode m = FocusSlots);
    // Edit fake methods of a promoted class stored in WidgetDataBase on a base class instance.
    static bool editPromotedClass(QDesignerFormEditorInterface *core, QObject *baseObject, QWidget *parent = nullptr, FocusMode m = FocusSlots);

    static void fakeMethodsFromMetaDataBase(QDesignerFormEditorInterface *core, QObject *o,
                                            QStringList &slotList, QStringList &signalList);
    static void fakeMethodsToMetaDataBase(QDesignerFormEditorInterface *core, QObject *o,
                                          const QStringList &slotList, const QStringList &signalList);
    static void existingMethodsFromMemberSheet(QDesignerFormEditorInterface *core, QObject *o,
                                               QStringList &slotList, QStringList &signalList);


private slots:
    void slotCheckSignature(const QString &signature, bool *ok);

private:
    // Edit fake methods of a promoted class stored in WidgetDataBase using an instance of the base class.
    static bool editPromotedClass(QDesignerFormEditorInterface *core, const QString &promotedClassName, QObject *baseObject, QWidget *parent, FocusMode m);

    const FocusMode m_focusMode;
    QT_PREPEND_NAMESPACE(Ui)::SignalSlotDialogClass *m_ui;
    QDesignerDialogGuiInterface *m_dialogGui;
    SignaturePanel *m_slotPanel;
    SignaturePanel *m_signalPanel;
};
}

QT_END_NAMESPACE

#endif
