// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINFO_H
#define QQMLINFO_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtQml/qqmlerror.h>

QT_BEGIN_NAMESPACE

class QQmlInfo;

Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me);
Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me, const QQmlError &error);
Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me, const QList<QQmlError> &errors);

Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me);
Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me, const QQmlError &error);
Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me, const QList<QQmlError> &errors);

Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me);
Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me, const QQmlError &error);
Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me, const QList<QQmlError> &errors);

class QQmlInfoPrivate;
class Q_QML_EXPORT QQmlInfo : public QDebug
{
public:
    QQmlInfo(const QQmlInfo &);
    ~QQmlInfo();

    inline QQmlInfo &operator<<(QChar t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(bool t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(char t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(signed short t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(unsigned short t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(signed int t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(unsigned int t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(signed long t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(unsigned long t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(qint64 t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(quint64 t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(float t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(double t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(const char* t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(const QString & t) { QDebug::operator<<(t.toLocal8Bit().constData()); return *this; }
    inline QQmlInfo &operator<<(QStringView t) { return operator<<(t.toString()); }
    inline QQmlInfo &operator<<(const QLatin1String &t) { QDebug::operator<<(t.latin1()); return *this; }
    inline QQmlInfo &operator<<(const QByteArray & t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(const void * t) { QDebug::operator<<(t); return *this; }
    inline QQmlInfo &operator<<(QTextStreamFunction f) { QDebug::operator<<(f); return *this; }
    inline QQmlInfo &operator<<(QTextStreamManipulator m) { QDebug::operator<<(m); return *this; }
#ifndef QT_NO_DEBUG_STREAM
    inline QQmlInfo &operator<<(const QUrl &t) { static_cast<QDebug &>(*this) << t; return *this; }
#endif

private:
    friend Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me);
    friend Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me, const QQmlError &error);
    friend Q_QML_EXPORT QQmlInfo qmlDebug(const QObject *me, const QList<QQmlError> &errors);
    friend Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me);
    friend Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me, const QQmlError &error);
    friend Q_QML_EXPORT QQmlInfo qmlInfo(const QObject *me, const QList<QQmlError> &errors);
    friend Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me);
    friend Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me, const QQmlError &error);
    friend Q_QML_EXPORT QQmlInfo qmlWarning(const QObject *me, const QList<QQmlError> &errors);

    QQmlInfo(QQmlInfoPrivate *);
    QQmlInfoPrivate *d;
};

QT_END_NAMESPACE

#endif // QQMLINFO_H
