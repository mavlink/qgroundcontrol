// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCALENDAR_P_H
#define QQUICKCALENDAR_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qdatetime.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickCalendar : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Calendar)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickCalendar(QObject *parent = nullptr);

    enum Month {
        January,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December
    };
    Q_ENUM(Month)
};

QT_END_NAMESPACE

#endif // QQUICKCALENDAR_P_H
