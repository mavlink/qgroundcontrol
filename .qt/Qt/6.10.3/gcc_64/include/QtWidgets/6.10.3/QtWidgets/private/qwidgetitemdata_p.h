// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGETITEMDATA_P_H
#define QWIDGETITEMDATA_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qvariant.h>

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

QT_BEGIN_NAMESPACE

class QWidgetItemData
{
public:
    inline QWidgetItemData() : role(-1) {}
    inline QWidgetItemData(int r, const QVariant &v) : role(r), value(v) {}
    int role;
    QVariant value;
    inline bool operator==(const QWidgetItemData &other) const { return role == other.role && value == other.value; }
};
Q_DECLARE_TYPEINFO(QWidgetItemData, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DATASTREAM

inline QDataStream &operator>>(QDataStream &in, QWidgetItemData &data)
{
    in >> data.role;
    in >> data.value;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const QWidgetItemData &data)
{
    out << data.role;
    out << data.value;
    return out;
}

#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QWIDGETITEMDATA_P_H
