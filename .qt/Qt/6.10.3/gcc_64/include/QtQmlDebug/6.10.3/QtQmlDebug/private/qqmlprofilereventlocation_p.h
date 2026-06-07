// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILEREVENTLOCATION_P_H
#define QQMLPROFILEREVENTLOCATION_P_H

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qdatastream.h>
#include <QtCore/private/qglobal_p.h>

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

class QQmlProfilerEventLocation
{
public:
    QQmlProfilerEventLocation() : m_line(-1),m_column(-1) {}
    QQmlProfilerEventLocation(const QString &file, int lineNumber, int columnNumber) :
        m_filename(file), m_line(lineNumber), m_column(columnNumber)
    {}

    void clear()
    {
        m_filename.clear();
        m_line = m_column = -1;
    }

    bool isValid() const
    {
        return !m_filename.isEmpty();
    }

    QString filename() const { return m_filename; }
    int line() const { return m_line; }
    int column() const { return m_column; }

private:
    friend QDataStream &operator>>(QDataStream &stream, QQmlProfilerEventLocation &location);
    friend QDataStream &operator<<(QDataStream &stream, const QQmlProfilerEventLocation &location);

    QString m_filename;
    int m_line;
    int m_column;
};

inline bool operator==(const QQmlProfilerEventLocation &location1,
                       const QQmlProfilerEventLocation &location2)
{
    // compare filename last as it's expensive.
    return location1.line() == location2.line() && location1.column() == location2.column()
            && location1.filename() == location2.filename();
}

inline bool operator!=(const QQmlProfilerEventLocation &location1,
                       const QQmlProfilerEventLocation &location2)
{
    return !(location1 == location2);
}

inline size_t qHash(const QQmlProfilerEventLocation &location)
{
    return qHash(location.filename())
            ^ ((location.line() & 0xfff)                   // 12 bits of line number
               | ((location.column() << 16) & 0xff0000));  // 8 bits of column

}

QDataStream &operator>>(QDataStream &stream, QQmlProfilerEventLocation &location);
QDataStream &operator<<(QDataStream &stream, const QQmlProfilerEventLocation &location);

Q_DECLARE_TYPEINFO(QQmlProfilerEventLocation, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLPROFILEREVENTLOCATION_P_H
