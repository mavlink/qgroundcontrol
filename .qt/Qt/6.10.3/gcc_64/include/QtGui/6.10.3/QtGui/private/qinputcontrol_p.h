// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QINPUTCONTROL_P_H
#define QINPUTCONTROL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <qtguiglobal.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QKeyEvent;
class QMimeData;
class QInputMethodEvent;
class Q_GUI_EXPORT QInputControl : public QObject
{
    Q_OBJECT
public:
    enum Type {
        LineEdit,
        TextEdit
    };

    explicit QInputControl(Type type, QObject *parent = nullptr);

    bool isAcceptableInput(const QKeyEvent *event) const;
    static bool isCommonTextEditShortcut(const QKeyEvent *ke);

    static QVariant selectionWrapper(QMimeData *mimeData);
    static QMimeData *mimeDataForInputEvent(QInputMethodEvent *event);

protected:
    explicit QInputControl(Type type, QObjectPrivate &dd, QObject *parent = nullptr);

private:
    const Type m_type;
};

QT_END_NAMESPACE

#endif // QINPUTCONTROL_P_H
