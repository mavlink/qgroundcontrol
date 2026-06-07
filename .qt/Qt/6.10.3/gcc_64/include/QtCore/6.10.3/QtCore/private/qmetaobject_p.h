// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2014 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMETAOBJECT_P_H
#define QMETAOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of moc.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qmutex.h>
#include <QtCore/qmetaobject.h>
#ifndef QT_NO_QOBJECT
#include <private/qobject_p.h> // For QObjectPrivate::Connection
#endif
#include <QtCore/qtmocconstants.h>
#include <private/qtools_p.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE
// ### TODO - QTBUG-87869: wrap in a proper Q_NAMESPACE and use scoped enums, to avoid name clashes

using namespace QtMiscUtils;
using namespace QtMocConstants;

Q_DECLARE_FLAGS(MetaObjectFlags, MetaObjectFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(MetaObjectFlags)

Q_CORE_EXPORT int qMetaTypeTypeInternal(QByteArrayView name);

class QArgumentType
{
public:
    QArgumentType() = default;
    QArgumentType(QMetaType metaType)
        : _metaType(metaType)
    {}
    explicit QArgumentType(QByteArrayView name)
        : _metaType(QMetaType{qMetaTypeTypeInternal(name)}), _name(name)
    {}
    QMetaType metaType() const noexcept
    { return _metaType; }
    QByteArrayView name() const noexcept
    {
        if (_name.isEmpty())
            return metaType().name();
        return _name;
    }

private:
    friend bool comparesEqual(const QArgumentType &lhs,
                              const QArgumentType &rhs)
    {
        if (lhs.metaType().isValid() && rhs.metaType().isValid())
            return lhs.metaType() == rhs.metaType();
        else
            return lhs.name() == rhs.name();
    }
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QArgumentType)

    QMetaType _metaType;
    QByteArrayView _name;
};
Q_DECLARE_TYPEINFO(QArgumentType, Q_RELOCATABLE_TYPE);

typedef QVarLengthArray<QArgumentType, 10> QArgumentTypeArray;

namespace { class QMetaMethodPrivate; }
class QMetaMethodInvoker : public QMetaMethod
{
    QMetaMethodInvoker() = delete;

public:
    enum class InvokeFailReason : int {
        // negative values mean a match was found but the invocation failed
        // (and a warning has been printed)
        ReturnTypeMismatch = -1,
        DeadLockDetected = -2,
        CallViaVirtualFailed = -3,  // no warning
        ConstructorCallOnObject = -4,
        ConstructorCallWithoutResult = -5,
        ConstructorCallFailed = -6, // no warning

        CouldNotQueueParameter = -0x1000,

        // zero is success
        None = 0,

        // positive values mean the parameters did not match
        TooFewArguments,
        FormalParameterMismatch = 0x1000,
    };

    // shadows the public function
    static InvokeFailReason Q_CORE_EXPORT
    invokeImpl(QMetaMethod self, void *target, Qt::ConnectionType, qsizetype paramCount,
               const void *const *parameters, const char *const *typeNames,
               const QtPrivate::QMetaTypeInterface *const *metaTypes);
};

struct QMetaObjectPrivate
{
    enum { OutputRevision = QtMocConstants::OutputRevision }; // Used by moc, qmetaobjectbuilder and qdbus
    enum { IntsPerMethod = QMetaMethod::Data::Size };
    enum { IntsPerEnum = QMetaEnum::Data::Size };
    enum { IntsPerProperty = QMetaProperty::Data::Size };

    int revision;
    int className;
    int classInfoCount, classInfoData;
    int methodCount, methodData;
    int propertyCount, propertyData;
    int enumeratorCount, enumeratorData;
    int constructorCount, constructorData;
    int flags;
    int signalCount;

    static inline const QMetaObjectPrivate *get(const QMetaObject *metaobject)
    { return reinterpret_cast<const QMetaObjectPrivate*>(metaobject->d.data); }

    static int originalClone(const QMetaObject *obj, int local_method_index);

    static QByteArrayView decodeMethodSignature(const char *signature,
                                                QArgumentTypeArray &types);
    static int indexOfSignalRelative(const QMetaObject **baseObject,
                                     QByteArrayView name, int argc,
                                     const QArgumentType *types);
    static int indexOfSlotRelative(const QMetaObject **m,
                                   QByteArrayView name, int argc,
                                   const QArgumentType *types);
    static int indexOfSignal(const QMetaObject *m, QByteArrayView name,
                             int argc, const QArgumentType *types);
    static int indexOfSlot(const QMetaObject *m, QByteArrayView name,
                           int argc, const QArgumentType *types);
    static int indexOfMethod(const QMetaObject *m, QByteArrayView name,
                             int argc, const QArgumentType *types);
    static int indexOfConstructor(const QMetaObject *m, QByteArrayView name,
                                  int argc, const QArgumentType *types);

    enum class Which { Name, Alias };
    static int indexOfEnumerator(const QMetaObject *m, QByteArrayView name, Which which);
    static int indexOfEnumerator(const QMetaObject *m, QByteArrayView name);

    Q_CORE_EXPORT static QMetaMethod signal(const QMetaObject *m, int signal_index);
    static inline int signalOffset(const QMetaObject *m)
    {
        Q_ASSERT(m != nullptr);
        int offset = 0;
        for (m = m->d.superdata; m; m = m->d.superdata)
            offset += reinterpret_cast<const QMetaObjectPrivate *>(m->d.data)->signalCount;
        return offset;
    }
    Q_CORE_EXPORT static int absoluteSignalCount(const QMetaObject *m);
    Q_CORE_EXPORT static int signalIndex(const QMetaMethod &m);
    static bool checkConnectArgs(int signalArgc, const QArgumentType *signalTypes,
                                 int methodArgc, const QArgumentType *methodTypes);
    static bool checkConnectArgs(const QMetaMethodPrivate *signal,
                                 const QMetaMethodPrivate *method);

    static QList<QByteArray> parameterTypeNamesFromSignature(QByteArrayView sig);

#ifndef QT_NO_QOBJECT
    // defined in qobject.cpp
    enum DisconnectType { DisconnectAll, DisconnectOne };
    static void memberIndexes(const QObject *obj, const QMetaMethod &member,
                              int *signalIndex, int *methodIndex);
    static QObjectPrivate::Connection *connect(const QObject *sender, int signal_index,
                        const QMetaObject *smeta,
                        const QObject *receiver, int method_index_relative,
                        const QMetaObject *rmeta = nullptr,
                        int type = 0, int *types = nullptr);
    static bool disconnect(const QObject *sender, int signal_index,
                           const QMetaObject *smeta,
                           const QObject *receiver, int method_index, void **slot,
                           DisconnectType = DisconnectAll);
    static inline bool disconnectHelper(QObjectPrivate::ConnectionData *connections, int signalIndex,
                                        const QObject *receiver, int method_index, void **slot,
                                        QBasicMutex *senderMutex, DisconnectType = DisconnectAll);
#endif

    template<int MethodType>
    static inline int indexOfMethodRelative(const QMetaObject **baseObject,
                                            QByteArrayView name, int argc,
                                            const QArgumentType *types);

    static bool methodMatch(const QMetaObject *m, const QMetaMethod &method,
                            QByteArrayView name, int argc,
                            const QArgumentType *types);
    Q_CORE_EXPORT static QMetaMethod firstMethod(const QMetaObject *baseObject, QByteArrayView name);

};

// For meta-object generators

enum { MetaObjectPrivateFieldCount = sizeof(QMetaObjectPrivate) / sizeof(int) };

#ifndef UTILS_H
// mirrored in moc's utils.h
static inline bool is_ident_char(char s)
{
    return isAsciiLetterOrNumber(s) || s == '_';
}

static inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}
#endif

QT_END_NAMESPACE

#endif

