// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLGLOBAL_H
#define QQMLGLOBAL_H

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

#include <private/qmetaobject_p.h>
#include <private/qqmlmetaobject_p.h>
#include <private/qqmltype_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

inline bool qmlConvertBoolConfigOption(const char *v)
{
    return v != nullptr && qstrcmp(v, "0") != 0 && qstrcmp(v, "false") != 0;
}

template<typename T, T(*Convert)(const char *)>
T qmlGetConfigOption(const char *var)
{
    if (Q_UNLIKELY(!qEnvironmentVariableIsEmpty(var)))
        return Convert(qgetenv(var));
    return Convert(nullptr);
}

#define DEFINE_BOOL_CONFIG_OPTION(name, var) \
    static bool name() \
    { \
        static const bool result = qmlGetConfigOption<bool, qmlConvertBoolConfigOption>(#var); \
        return result; \
    }

/*!
    Connect \a Signal of \a Sender to \a Method of \a Receiver.  \a Signal must be
    of type \a SenderType and \a Receiver of type \a ReceiverType.

    Unlike QObject::connect(), this macro caches the lookup of the signal and method
    indexes.  It also does not require lazy QMetaObjects to be built so should be
    preferred in all QML code that might interact with QML built objects.

    \code
        QQuickTextControl *control;
        QQuickTextEdit *textEdit;
        qmlobject_connect(control, QQuickTextControl, SIGNAL(updateRequest(QRectF)),
                          textEdit, QQuickTextEdit, SLOT(updateDocument()));
    \endcode
*/
#define qmlobject_connect(Sender, SenderType, Signal, Receiver, ReceiverType, Method) \
do { \
    SenderType *sender = (Sender); \
    ReceiverType *receiver = (Receiver); \
    const char *signal = (Signal); \
    const char *method = (Method); \
    static int signalIdx = -1; \
    static int methodIdx = -1; \
    if (signalIdx < 0) { \
        Q_ASSERT((int(*signal) - '0') == QSIGNAL_CODE); \
        signalIdx = SenderType::staticMetaObject.indexOfSignal(signal+1); \
    } \
    if (methodIdx < 0) { \
        int code = (int(*method) - '0'); \
        Q_ASSERT(code == QSLOT_CODE || code == QSIGNAL_CODE); \
        if (code == QSLOT_CODE) \
            methodIdx = ReceiverType::staticMetaObject.indexOfSlot(method+1); \
        else \
            methodIdx = ReceiverType::staticMetaObject.indexOfSignal(method+1); \
    } \
    Q_ASSERT(signalIdx != -1 && methodIdx != -1); \
    QMetaObject::connect(sender, signalIdx, receiver, methodIdx, Qt::DirectConnection); \
} while (0)

/*!
    Disconnect \a Signal of \a Sender from \a Method of \a Receiver.  \a Signal must be
    of type \a SenderType and \a Receiver of type \a ReceiverType.

    Unlike QObject::disconnect(), this macro caches the lookup of the signal and method
    indexes.  It also does not require lazy QMetaObjects to be built so should be
    preferred in all QML code that might interact with QML built objects.

    \code
        QQuickTextControl *control;
        QQuickTextEdit *textEdit;
        qmlobject_disconnect(control, QQuickTextControl, SIGNAL(updateRequest(QRectF)),
                             textEdit, QQuickTextEdit, SLOT(updateDocument()));
    \endcode
*/
#define qmlobject_disconnect(Sender, SenderType, Signal, Receiver, ReceiverType, Method) \
do { \
    SenderType *sender = (Sender); \
    ReceiverType *receiver = (Receiver); \
    const char *signal = (Signal); \
    const char *method = (Method); \
    static int signalIdx = -1; \
    static int methodIdx = -1; \
    if (signalIdx < 0) { \
        Q_ASSERT((int(*signal) - '0') == QSIGNAL_CODE); \
        signalIdx = SenderType::staticMetaObject.indexOfSignal(signal+1); \
    } \
    if (methodIdx < 0) { \
        int code = (int(*method) - '0'); \
        Q_ASSERT(code == QSLOT_CODE || code == QSIGNAL_CODE); \
        if (code == QSLOT_CODE) \
            methodIdx = ReceiverType::staticMetaObject.indexOfSlot(method+1); \
        else \
            methodIdx = ReceiverType::staticMetaObject.indexOfSignal(method+1); \
    } \
    Q_ASSERT(signalIdx != -1 && methodIdx != -1); \
    QMetaObject::disconnect(sender, signalIdx, receiver, methodIdx); \
} while (0)

Q_QML_EXPORT bool qmlobject_can_cpp_cast(QObject *object, const QMetaObject *mo);
Q_QML_EXPORT bool qmlobject_can_qml_cast(QObject *object, const QQmlType &type);

/*!
    This method is identical to qobject_cast<T>() except that it does not require lazy
    QMetaObjects to be built, so should be preferred in all QML code that might interact
    with QML built objects.

    \code
        QObject *object;
        if (QQuickTextEdit *textEdit = qmlobject_cast<QQuickTextEdit *>(object)) {
            // ...Do something...
        }
    \endcode
*/
template<class T>
T qmlobject_cast(QObject *object)
{
    if (!object)
        return nullptr;
    if (qmlobject_can_cpp_cast(object, &(std::remove_pointer_t<T>::staticMetaObject)))
        return static_cast<T>(object);
    else
        return nullptr;
}

class QQuickItem;
template<>
inline QQuickItem *qmlobject_cast<QQuickItem *>(QObject *object)
{
    if (!object || !object->isQuickItemType())
        return nullptr;
    // QQuickItem is incomplete here -> can't use static_cast
    // but we don't need any pointer adjustment, so reinterpret is safe
    return reinterpret_cast<QQuickItem *>(object);
}

#define IS_SIGNAL_CONNECTED(Sender, SenderType, Name, Arguments) \
do { \
    QObject *sender = (Sender); \
    void (SenderType::*signal)Arguments = &SenderType::Name; \
    static QMetaMethod method = QMetaMethod::fromSignal(signal); \
    static int signalIdx = QMetaObjectPrivate::signalIndex(method); \
    return QObjectPrivate::get(sender)->isSignalConnected(signalIdx); \
} while (0)

/*!
    Returns true if the case of \a fileName is equivalent to the file case of
    \a fileName on disk, and false otherwise.

    This is used to ensure that the behavior of QML on a case-insensitive file
    system is the same as on a case-sensitive file system.  This function
    performs a "best effort" attempt to determine the real case of the file.
    It may have false positives (say the case is correct when it isn't), but it
    should never have a false negative (say the case is incorrect when it is
    correct).

    Length specifies specifies the number of characters to be checked from
    behind. That is, if a file name results from a relative path specification
    like "foo/bar.qml" and is made absolute, the original length (11) should
    be passed indicating that only the last part of the relative path should
    be checked.

*/
bool QQml_isFileCaseCorrect(const QString &fileName, int length = -1);

/*!
    Makes the \a object a child of \a parent.  Note that when using this method,
    neither \a parent nor the object's previous parent (if it had one) will
    receive ChildRemoved or ChildAdded events.
*/
inline void QQml_setParent_noEvent(QObject *object, QObject *parent)
{
    QObjectPrivate *d_ptr = QObjectPrivate::get(object);
    bool sce = d_ptr->sendChildEvents;
    d_ptr->sendChildEvents = false;
    object->setParent(parent);
    d_ptr->sendChildEvents = sce;
}

class QQmlValueTypeProvider
{
public:
    static bool populateValueType(
            QMetaType targetMetaType, void *target, const QV4::Value &source,
            QV4::ExecutionEngine *engine);
    static bool populateValueType(
            QMetaType targetMetaType, void *target, QMetaType sourceMetaType, void *source,
            QV4::ExecutionEngine *engine);

    static Q_QML_EXPORT void *heapCreateValueType(
            const QQmlType &targetType, const QV4::Value &source, QV4::ExecutionEngine *engine);
    static QVariant constructValueType(
            QMetaType targetMetaType, const QMetaObject *targetMetaObject,
            int ctorIndex, void **args);

    static QVariant createValueType(const QJSValue &, QMetaType);
    static QVariant createValueType(const QString &, QMetaType);
    static QVariant createValueType(const QV4::Value &, QMetaType, QV4::ExecutionEngine *);
    static QVariant Q_AUTOTEST_EXPORT createValueType(const QVariant &, QMetaType, QV4::ExecutionEngine *);
};

class Q_QML_EXPORT QQmlColorProvider
{
public:
    virtual ~QQmlColorProvider();
    virtual QVariant colorFromString(const QString &, bool *);
    virtual unsigned rgbaFromString(const QString &, bool *);

    virtual QVariant fromRgbF(double, double, double, double);
    virtual QVariant fromHslF(double, double, double, double);
    virtual QVariant fromHsvF(double, double, double, double);
    virtual QVariant lighter(const QVariant &, qreal);
    virtual QVariant darker(const QVariant &, qreal);
    virtual QVariant alpha(const QVariant &, qreal);
    virtual QVariant tint(const QVariant &, const QVariant &);
};

Q_QML_EXPORT QQmlColorProvider *QQml_setColorProvider(QQmlColorProvider *);
Q_QML_EXPORT QQmlColorProvider *QQml_colorProvider();

class QQmlApplication;
class Q_QML_EXPORT QQmlGuiProvider
{
public:
    virtual ~QQmlGuiProvider();
    virtual QQmlApplication *application(QObject *parent);
    virtual QObject *inputMethod();
    virtual QObject *styleHints();
    virtual QStringList fontFamilies();
    virtual bool openUrlExternally(const QUrl &);
    virtual QString pluginName() const;
};

Q_QML_EXPORT QQmlGuiProvider *QQml_setGuiProvider(QQmlGuiProvider *);
Q_AUTOTEST_EXPORT QQmlGuiProvider *QQml_guiProvider();

class QQmlApplicationPrivate;

class Q_QML_EXPORT QQmlApplication : public QObject
{
    //Application level logic, subclassed by Qt Quick if available via QQmlGuiProvider
    Q_OBJECT
    Q_PROPERTY(QStringList arguments READ args CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)
    Q_PROPERTY(QString organization READ organization WRITE setOrganization NOTIFY organizationChanged)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    QML_ANONYMOUS
public:
    QQmlApplication(QObject* parent=nullptr);

    QStringList args();

    QString name() const;
    QString version() const;
    QString organization() const;
    QString domain() const;

public Q_SLOTS:
    void setName(const QString &arg);
    void setVersion(const QString &arg);
    void setOrganization(const QString &arg);
    void setDomain(const QString &arg);

Q_SIGNALS:
    void aboutToQuit();

    void nameChanged();
    void versionChanged();
    void organizationChanged();
    void domainChanged();

protected:
    QQmlApplication(QQmlApplicationPrivate &dd, QObject* parent=nullptr);

private:
    Q_DISABLE_COPY(QQmlApplication)
    Q_DECLARE_PRIVATE(QQmlApplication)
};

class QQmlApplicationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlApplication)
public:
    QQmlApplicationPrivate() {
        argsInit = false;
    }

    bool argsInit;
    QStringList args;
};

struct QQmlSourceLocation
{
    QQmlSourceLocation() {}
    QQmlSourceLocation(const QString &sourceFile, quint16 line, quint16 column)
        : sourceFile(sourceFile), line(line), column(column) {}
    QString sourceFile;
    quint16 line = 0;
    quint16 column = 0;
};

QT_END_NAMESPACE

#endif // QQMLGLOBAL_H
