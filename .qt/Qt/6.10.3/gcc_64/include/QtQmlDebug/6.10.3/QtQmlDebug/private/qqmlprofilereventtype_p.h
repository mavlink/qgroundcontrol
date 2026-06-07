// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILEREVENTTYPE_P_H
#define QQMLPROFILEREVENTTYPE_P_H

#include "qqmlprofilereventlocation_p.h"
#include "qqmlprofilerclientdefinitions_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qhash.h>

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

class QQmlProfilerEventType {
public:
    QQmlProfilerEventType(Message message = MaximumMessage, RangeType rangeType = MaximumRangeType,
                          int detailType = -1,
                          const QQmlProfilerEventLocation &location = QQmlProfilerEventLocation(),
                          const QString &data = QString(), const QString displayName = QString()) :
        m_displayName(displayName), m_data(data), m_location(location), m_message(message),
        m_rangeType(rangeType), m_detailType(detailType)
    {}

    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setData(const QString &data) { m_data = data; }
    void setLocation(const QQmlProfilerEventLocation &location) { m_location = location; }

    ProfileFeature feature() const;
    QString displayName() const { return m_displayName; }
    QString data() const { return m_data; }
    QQmlProfilerEventLocation location() const { return m_location; }
    Message message() const { return m_message; }
    RangeType rangeType() const { return m_rangeType; }
    int detailType() const { return m_detailType; }

private:
    friend QDataStream &operator>>(QDataStream &stream, QQmlProfilerEventType &type);
    friend QDataStream &operator<<(QDataStream &stream, const QQmlProfilerEventType &type);

    QString m_displayName;
    QString m_data;
    QQmlProfilerEventLocation m_location;
    Message m_message;
    RangeType m_rangeType;
    int m_detailType; // can be EventType, BindingType, PixmapEventType or SceneGraphFrameType
};

QDataStream &operator>>(QDataStream &stream, QQmlProfilerEventType &type);
QDataStream &operator<<(QDataStream &stream, const QQmlProfilerEventType &type);

inline size_t qHash(const QQmlProfilerEventType &type)
{
    return qHash(type.location())
            ^ (((type.message() << 12) & 0xf000)                               // 4 bits message
               | ((type.rangeType() << 24) & 0xf000000)                        // 4 bits rangeType
               | ((static_cast<uint>(type.detailType()) << 28) & 0xf0000000)); // 4 bits detailType
}

inline bool operator==(const QQmlProfilerEventType &type1, const QQmlProfilerEventType &type2)
{
    return type1.message() == type2.message() && type1.rangeType() == type2.rangeType()
            && type1.detailType() == type2.detailType() && type1.location() == type2.location();
}

inline bool operator!=(const QQmlProfilerEventType &type1, const QQmlProfilerEventType &type2)
{
    return !(type1 == type2);
}

Q_DECLARE_TYPEINFO(QQmlProfilerEventType, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlProfilerEventType)

#endif // QQMLPROFILEREVENTTYPE_P_H
