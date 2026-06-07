// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBILITYHINTS_H
#define QACCESSIBILITYHINTS_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QAccessibilityHintsPrivate;

class Q_GUI_EXPORT QAccessibilityHints : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAccessibilityHints)
    Q_PROPERTY(Qt::ContrastPreference contrastPreference READ contrastPreference NOTIFY contrastPreferenceChanged FINAL REVISION(6, 10))

public:
    explicit QAccessibilityHints(QObject *parent = nullptr);
    ~QAccessibilityHints() override;

    Qt::ContrastPreference contrastPreference() const;

Q_SIGNALS:
    void contrastPreferenceChanged(Qt::ContrastPreference contrastPreference);

protected:
    bool event(QEvent *event) override;
};

QT_END_NAMESPACE

#endif // QACCESSIBILITYHINTS_H
