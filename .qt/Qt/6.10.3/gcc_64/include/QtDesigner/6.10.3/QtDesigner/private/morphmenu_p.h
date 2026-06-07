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

#ifndef MORPH_COMMAND_H
#define MORPH_COMMAND_H

#include "shared_global_p.h"
#include "qdesigner_formwindowcommand_p.h"

QT_BEGIN_NAMESPACE

class QAction;
class QMenu;

namespace qdesigner_internal {

/* Conveniene morph menu that acts on a single widget. */
class QDESIGNER_SHARED_EXPORT MorphMenu : public QObject {
    Q_DISABLE_COPY_MOVE(MorphMenu)
    Q_OBJECT
public:
    using ActionList = QList<QAction *>;

    explicit MorphMenu(QObject *parent = nullptr);

    void populate(QWidget *w, QDesignerFormWindowInterface *fw, ActionList& al);
    void populate(QWidget *w, QDesignerFormWindowInterface *fw, QMenu& m);

private slots:
    void slotMorph(const QString &newClassName);

private:
    bool populateMenu(QWidget *w, QDesignerFormWindowInterface *fw);

    QAction *m_subMenuAction = nullptr;
    QMenu *m_menu = nullptr;
    QWidget *m_widget = nullptr;
    QDesignerFormWindowInterface *m_formWindow = nullptr;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // MORPH_COMMAND_H
