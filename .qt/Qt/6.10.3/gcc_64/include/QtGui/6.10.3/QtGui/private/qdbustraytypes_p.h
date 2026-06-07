// Copyright (C) 2009 Marco Martin <notmart@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSTRAYTYPES_P_H
#define QDBUSTRAYTYPES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

QT_REQUIRE_CONFIG(systemtrayicon);

#include <QObject>
#include <QString>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QPixmap>

QT_BEGIN_NAMESPACE

// Custom message type to send icons across D-Bus
struct QXdgDBusImageStruct
{
    QXdgDBusImageStruct() { }
    QXdgDBusImageStruct(int w, int h)
        : width(w), height(h), data(width * height * 4, 0) { }
    int width;
    int height;
    QByteArray data;
};
Q_DECLARE_TYPEINFO(QXdgDBusImageStruct, Q_RELOCATABLE_TYPE);

using QXdgDBusImageVector = QList<QXdgDBusImageStruct>;

QXdgDBusImageVector iconToQXdgDBusImageVector(const QIcon &icon);

// Custom message type to send tooltips across D-Bus
struct QXdgDBusToolTipStruct
{
    QString icon;
    QXdgDBusImageVector image;
    QString title;
    QString subTitle;
};
Q_DECLARE_TYPEINFO(QXdgDBusToolTipStruct, Q_RELOCATABLE_TYPE);

const QDBusArgument &operator<<(QDBusArgument &argument, const QXdgDBusImageStruct &icon);
const QDBusArgument &operator>>(const QDBusArgument &argument, QXdgDBusImageStruct &icon);

const QDBusArgument &operator<<(QDBusArgument &argument, const QXdgDBusImageVector &iconVector);
const QDBusArgument &operator>>(const QDBusArgument &argument, QXdgDBusImageVector &iconVector);

const QDBusArgument &operator<<(QDBusArgument &argument, const QXdgDBusToolTipStruct &toolTip);
const QDBusArgument &operator>>(const QDBusArgument &argument, QXdgDBusToolTipStruct &toolTip);

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QXdgDBusImageStruct, Q_GUI_EXPORT)
QT_DECL_METATYPE_EXTERN(QXdgDBusImageVector, Q_GUI_EXPORT)
QT_DECL_METATYPE_EXTERN(QXdgDBusToolTipStruct, Q_GUI_EXPORT)

#endif // QDBUSTRAYTYPES_P_H
