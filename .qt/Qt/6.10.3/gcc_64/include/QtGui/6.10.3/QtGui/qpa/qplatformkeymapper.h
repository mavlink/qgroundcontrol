// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMKEYMAPPER_P
#define QPLATFORMKEYMAPPER_P

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQpaKeyMapper, Q_GUI_EXPORT)

class QKeyEvent;

class Q_GUI_EXPORT QPlatformKeyMapper
{
public:
    virtual ~QPlatformKeyMapper();

    virtual QList<QKeyCombination> possibleKeyCombinations(const QKeyEvent *event) const;
    virtual Qt::KeyboardModifiers queryKeyboardModifiers() const;
};

QT_END_NAMESPACE

#endif // QPLATFORMKEYMAPPER_P
