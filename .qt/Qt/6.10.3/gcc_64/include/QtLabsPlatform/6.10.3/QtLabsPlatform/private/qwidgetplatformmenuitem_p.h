// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWIDGETPLATFORMMENUITEM_P_H
#define QWIDGETPLATFORMMENUITEM_P_H

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

#include <QtGui/qpa/qplatformmenu.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QAction;

class QWidgetPlatformMenuItem : public QPlatformMenuItem
{
    Q_OBJECT

public:
    explicit QWidgetPlatformMenuItem(QObject *parent = nullptr);
    ~QWidgetPlatformMenuItem();

    QAction *action() const;

    void setText(const QString &text) override;
    void setIcon(const QIcon &icon) override;
    void setMenu(QPlatformMenu *menu) override;
    void setVisible(bool visible) override;
    void setIsSeparator(bool separator) override;
    void setFont(const QFont &font) override;
    void setRole(MenuRole role) override;
    void setCheckable(bool checkable) override;
    void setChecked(bool checked) override;
#if QT_CONFIG(shortcut)
    void setShortcut(const QKeySequence& shortcut) override;
#endif
    void setEnabled(bool enabled) override;
    void setIconSize(int size) override;

private:
    QScopedPointer<QAction> m_action;
};

QT_END_NAMESPACE

#endif // QWIDGETPLATFORMMENUITEM_P_H
