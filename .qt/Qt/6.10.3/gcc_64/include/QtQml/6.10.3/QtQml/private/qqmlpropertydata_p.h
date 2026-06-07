// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYDATA_P_H
#define QQMLPROPERTYDATA_P_H

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

#include <private/qobject_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyCacheMethodArguments;
class QQmlPropertyData
{
public:
    enum WriteFlag {
        BypassInterceptor = 0x01,
        DontRemoveBinding = 0x02,
        RemoveBindingOnAliasWrite = 0x04,
        HasInternalIndex = 0x8,
    };
    Q_DECLARE_FLAGS(WriteFlags, WriteFlag)

    typedef QObjectPrivate::StaticMetaCallFunction StaticMetaCallFunction;

    struct Flags {
        friend class QQmlPropertyData;
        enum Type {
            OtherType            = 0,
            FunctionType         = 1, // Is an invokable
            QObjectDerivedType   = 2, // Property type is a QObject* derived type
            EnumType             = 3, // Property type is an enum
            QListType            = 4, // Property type is a QML list
            VarPropertyType      = 5, // Property type is a "var" property of VMEMO
            QVariantType         = 6, // Property is a QVariant
            // One spot left for an extra type in the 3 bits used to store this.
        };

    private:
        // The _otherBits (which "pad" the Flags struct to align it nicely) are used
        // to store the relative property index. It will only get used when said index fits. See
        // trySetStaticMetaCallFunction for details.
        // (Note: this padding is done here, because certain compilers have surprising behavior
        // when an enum is declared in-between two bit fields.)
        enum { BitsLeftInFlags = 16 };
        unsigned otherBits       : BitsLeftInFlags; // align to 32 bits

        // Members of the form aORb can only be a when type is not FunctionType, and only be
        // b when type equals FunctionType. For that reason, the semantic meaning of the bit is
        // overloaded, and the accessor functions are used to get the correct value
        //
        // Moreover, isSignalHandler, isOverridableSignal and isCloned make only sense
        // for functions, too (and could at a later point be reused for flags that only make sense
        // for non-functions)
        //
        // Lastly, isDirect and isOverridden apply to both functions and non-functions
        unsigned isConst                       : 1; // Property: has CONST flag/Method: is const
        unsigned isDeepAliasORisVMEFunction    : 1; // Alias points into value type OR Function was added by QML
        unsigned isWritableORhasArguments      : 1; // Has WRITE function OR Function takes arguments
        unsigned isResettableORisSignal        : 1; // Has RESET function OR Function is a signal
        unsigned isAliasORisVMESignal          : 1; // Is a QML alias to another property OR Signal was added by QML
        unsigned isFinalORisV4Function         : 1; // Has FINAL flag OR Function takes QQmlV4FunctionPtr args
        unsigned isSignalHandler               : 1; // Function is a signal handler

        // TODO: Remove this once we can. Signals should not be overridable.
        unsigned isOverridableSignal           : 1; // Function is an overridable signal

        unsigned isRequiredORisCloned          : 1; // Has REQUIRED flag OR The function was marked as cloned
        unsigned isConstructorORisBindable     : 1; // The function was marked is a constructor OR property is backed by QProperty<T>
        unsigned isOverridden                  : 1; // Is overridden by a extension property
        unsigned hasMetaObject                 : 1;
        unsigned type                          : 3; // stores an entry of Types

        // Internal QQmlPropertyCache flags
        unsigned overrideIndexIsProperty       : 1;

    public:
        inline Flags();
        inline bool operator==(const Flags &other) const;
        inline void copyPropertyTypeFlags(Flags from);

        void setIsConstant(bool b) {
            isConst = b;
        }

        void setIsWritable(bool b) {
            Q_ASSERT(type != FunctionType);
            isWritableORhasArguments = b;
        }

        void setIsResettable(bool b) {
            Q_ASSERT(type != FunctionType);
            isResettableORisSignal = b;
        }

        void setIsAlias(bool b) {
            Q_ASSERT(type != FunctionType);
            isAliasORisVMESignal = b;
        }

        void setIsDeepAlias(bool b) {
            Q_ASSERT(type != FunctionType);
            isDeepAliasORisVMEFunction = b;
        }

        void setIsFinal(bool b) {
            Q_ASSERT(type != FunctionType);
            isFinalORisV4Function = b;
        }

        void setIsOverridden(bool b) {
            isOverridden = b;
        }

        void setIsBindable(bool b) {
            Q_ASSERT(type != FunctionType);
            isConstructorORisBindable = b;
        }

        void setIsRequired(bool b) {
            Q_ASSERT(type != FunctionType);
            isRequiredORisCloned = b;
        }

        void setIsVMEFunction(bool b) {
            Q_ASSERT(type == FunctionType);
            isDeepAliasORisVMEFunction = b;
        }

        void setHasArguments(bool b) {
            Q_ASSERT(type == FunctionType);
            isWritableORhasArguments = b;
        }
        void setIsSignal(bool b) {
            Q_ASSERT(type == FunctionType);
            isResettableORisSignal = b;
        }
        void setIsVMESignal(bool b) {
            Q_ASSERT(type == FunctionType);
            isAliasORisVMESignal = b;
        }

        void setIsV4Function(bool b) {
            Q_ASSERT(type == FunctionType);
            isFinalORisV4Function = b;
        }

        void setIsSignalHandler(bool b) {
            Q_ASSERT(type == FunctionType);
            isSignalHandler = b;
        }

        // TODO: Remove this once we can. Signals should not be overridable.
        void setIsOverridableSignal(bool b) {
            Q_ASSERT(type == FunctionType);
            Q_ASSERT(isResettableORisSignal);
            isOverridableSignal = b;
        }

        void setIsCloned(bool b) {
            Q_ASSERT(type == FunctionType);
            isRequiredORisCloned = b;
        }

        void setIsConstructor(bool b) {
            Q_ASSERT(type == FunctionType);
            isConstructorORisBindable = b;
        }

        void setHasMetaObject(bool b) {
            hasMetaObject = b;
        }

        void setType(Type newType) {
            type = newType;
        }
    };


    inline bool operator==(const QQmlPropertyData &) const;

    Flags flags() const { return m_flags; }
    void setFlags(Flags f)
    {
        unsigned otherBits = m_flags.otherBits;
        m_flags = f;
        m_flags.otherBits = otherBits;
    }

    bool isValid() const { return coreIndex() != -1; }

    bool isConstant() const { return m_flags.isConst; }
    bool isWritable() const { return !isFunction() && m_flags.isWritableORhasArguments; }
    void setWritable(bool onoff) { Q_ASSERT(!isFunction()); m_flags.isWritableORhasArguments = onoff; }
    bool isResettable() const { return !isFunction() && m_flags.isResettableORisSignal; }
    bool isAlias() const { return !isFunction() && m_flags.isAliasORisVMESignal; }
    bool isFinal() const { return !isFunction() && m_flags.isFinalORisV4Function; }
    bool isOverridden() const { return m_flags.isOverridden; }
    bool isRequired() const { return !isFunction() && m_flags.isRequiredORisCloned; }
    bool isFunction() const { return m_flags.type == Flags::FunctionType; }
    bool isQObject() const { return m_flags.type == Flags::QObjectDerivedType; }
    bool isEnum() const { return m_flags.type == Flags::EnumType; }
    bool isQList() const { return m_flags.type == Flags::QListType; }
    bool isVarProperty() const { return m_flags.type == Flags::VarPropertyType; }
    bool isQVariant() const { return m_flags.type == Flags::QVariantType; }
    bool isVMEFunction() const { return isFunction() && m_flags.isDeepAliasORisVMEFunction; }
    bool hasArguments() const { return isFunction() && m_flags.isWritableORhasArguments; }
    bool isSignal() const { return isFunction() && m_flags.isResettableORisSignal; }
    bool isVMESignal() const { return isFunction() && m_flags.isAliasORisVMESignal; }
    bool isV4Function() const { return isFunction() && m_flags.isFinalORisV4Function; }
    bool isSignalHandler() const { return m_flags.isSignalHandler; }
    bool hasMetaObject() const { return m_flags.hasMetaObject; }

    bool hasStaticMetaCallFunction() const
    {
        return !isAlias() && staticMetaCallFunction() != nullptr;
    }

    // TODO: Remove this once we can. Signals should not be overridable.
    bool isOverridableSignal() const { return m_flags.isOverridableSignal; }

    bool isCloned() const { return isFunction() && m_flags.isRequiredORisCloned; }
    bool isConstructor() const { return isFunction() && m_flags.isConstructorORisBindable; }

    bool notifiesViaBindable() const { return !isFunction() && m_flags.isConstructorORisBindable; }
    bool acceptsQBinding() const { return notifiesViaBindable() && !m_flags.isDeepAliasORisVMEFunction; }

    bool hasOverride() const { return overrideIndex() >= 0; }
    bool hasRevision() const { return revision() != QTypeRevision::zero(); }

    QMetaType propType() const { return m_propType; }
    void setPropType(QMetaType pt)
    {
        m_propType = pt;
    }

    int notifyIndex() const { return m_notifyIndex; }
    void setNotifyIndex(int idx)
    {
        Q_ASSERT(idx >= std::numeric_limits<qint16>::min());
        Q_ASSERT(idx <= std::numeric_limits<qint16>::max());
        m_notifyIndex = qint16(idx);
    }

    bool overrideIndexIsProperty() const { return m_flags.overrideIndexIsProperty; }
    void setOverrideIndexIsProperty(bool onoff) { m_flags.overrideIndexIsProperty = onoff; }

    int overrideIndex() const { return m_overrideIndex; }
    void setOverrideIndex(int idx)
    {
        Q_ASSERT(idx >= std::numeric_limits<qint16>::min());
        Q_ASSERT(idx <= std::numeric_limits<qint16>::max());
        m_overrideIndex = qint16(idx);
    }

    int coreIndex() const { return m_coreIndex; }
    void setCoreIndex(int idx)
    {
        Q_ASSERT(idx >= std::numeric_limits<qint16>::min());
        Q_ASSERT(idx <= std::numeric_limits<qint16>::max());
        m_coreIndex = qint16(idx);
    }

    int aliasTarget() const
    {
        Q_ASSERT(isAlias());
        return m_encodedAliasTargetIndex;
    }
    void setAliasTarget(int target)
    {
        Q_ASSERT(isAlias());
        m_encodedAliasTargetIndex = target;
    }

    QTypeRevision revision() const { return m_revision; }
    void setRevision(QTypeRevision revision) { m_revision = revision; }

    /* If a property is a C++ type, then we store the minor
     * version of this type.
     * This is required to resolve property or signal revisions
     * if this property is used as a grouped property.
     *
     * Test.qml
     * property TextEdit someTextEdit: TextEdit {}
     *
     * Test {
     *   someTextEdit.preeditText: "test" //revision 7
     *   someTextEdit.onEditingFinished: console.log("test") //revision 6
     * }
     *
     * To determine if these properties with revisions are available we need
     * the minor version of TextEdit as imported in Test.qml.
     *
     */

    QTypeRevision typeVersion() const { return m_typeVersion; }
    void setTypeVersion(QTypeRevision typeVersion) { m_typeVersion = typeVersion; }

    QQmlPropertyCacheMethodArguments *arguments() const
    {
        Q_ASSERT(!hasMetaObject());
        return m_arguments;
    }
    void setArguments(QQmlPropertyCacheMethodArguments *args)
    {
        Q_ASSERT(!hasMetaObject());
        m_arguments = args;
    }

    const QMetaObject *metaObject() const
    {
        Q_ASSERT(hasMetaObject());
        return m_metaObject;
    }

    void setMetaObject(const QMetaObject *metaObject)
    {
        Q_ASSERT(!hasArguments() || !m_arguments);
        m_flags.setHasMetaObject(true);
        m_metaObject = metaObject;
    }

    QMetaMethod metaMethod() const
    {
        Q_ASSERT(hasMetaObject());
        Q_ASSERT(isFunction());
        return m_metaObject->method(m_coreIndex);
    }

    int metaObjectOffset() const { return m_metaObjectOffset; }
    void setMetaObjectOffset(int off)
    {
        Q_ASSERT(off >= std::numeric_limits<qint16>::min());
        Q_ASSERT(off <= std::numeric_limits<qint16>::max());
        m_metaObjectOffset = qint16(off);
    }

    StaticMetaCallFunction staticMetaCallFunction() const
    {
        Q_ASSERT(!isFunction() && !isAlias());
        return m_staticMetaCallFunction;
    }

    void trySetStaticMetaCallFunction(StaticMetaCallFunction f, unsigned relativePropertyIndex)
    {
        Q_ASSERT(!isFunction() && !isAlias());
        if (relativePropertyIndex < (1 << Flags::BitsLeftInFlags) - 1) {
            m_flags.otherBits = relativePropertyIndex;
            m_staticMetaCallFunction = f;
        }
    }
    quint16 relativePropertyIndex() const { Q_ASSERT(hasStaticMetaCallFunction()); return m_flags.otherBits; }

    static Flags flagsForProperty(const QMetaProperty &);
    void load(const QMetaProperty &);
    void load(const QMetaMethod &);

    QString name(QObject *object) const { return object ? name(object->metaObject()) : QString(); }
    QString name(const QMetaObject *metaObject) const
    {
        if (!metaObject || m_coreIndex == -1)
            return QString();

        return QString::fromUtf8(isFunction()
            ? metaObject->method(m_coreIndex).name().constData()
            : metaObject->property(m_coreIndex).name());
    }

    bool markAsOverrideOf(QQmlPropertyData *predecessor);

    inline void readProperty(QObject *target, void *property) const
    {
        void *args[] = { property, nullptr };
        readPropertyWithArgs(target, args);
    }

    // This is the same as QMetaObject::metacall(), but inlined here to avoid a function call.
    // And we ignore the return value.
    template<QMetaObject::Call call>
    void doMetacall(QObject *object, int idx, void **argv) const
    {
        if (QDynamicMetaObjectData *dynamicMetaObject = QObjectPrivate::get(object)->metaObject)
            dynamicMetaObject->metaCall(object, call, idx, argv);
        else
            object->qt_metacall(call, idx, argv);
    }

    void readPropertyWithArgs(QObject *target, void *args[]) const
    {
        if (hasStaticMetaCallFunction())
            staticMetaCallFunction()(target, QMetaObject::ReadProperty, relativePropertyIndex(), args);
        else
            doMetacall<QMetaObject::ReadProperty>(target, coreIndex(), args);
    }

    bool writeProperty(QObject *target, void *value, WriteFlags flags) const
    {
        int status = -1;
        void *argv[] = { value, nullptr, &status, &flags };
        if (flags.testFlag(BypassInterceptor) && hasStaticMetaCallFunction())
            staticMetaCallFunction()(target, QMetaObject::WriteProperty, relativePropertyIndex(), argv);
        else
            doMetacall<QMetaObject::WriteProperty>(target, coreIndex(), argv);
        return true;
    }

    bool resetProperty(QObject *target, WriteFlags flags) const
    {
        if (flags.testFlag(BypassInterceptor) && hasStaticMetaCallFunction())
            staticMetaCallFunction()(target, QMetaObject::ResetProperty, relativePropertyIndex(), nullptr);
        else
            doMetacall<QMetaObject::ResetProperty>(target, coreIndex(), nullptr);
        return true;
    }

    QUntypedBindable propertyBindable(QObject *target) const
    {
        QUntypedBindable result;
        void *argv[] = { &result };
        if (hasStaticMetaCallFunction())
            staticMetaCallFunction()(target, QMetaObject::BindableProperty, relativePropertyIndex(), argv);
        else
            doMetacall<QMetaObject::BindableProperty>(target, coreIndex(), argv);
        return result;
    }

    static Flags defaultSignalFlags()
    {
        Flags f;
        f.setType(Flags::FunctionType);
        f.setIsSignal(true);
        f.setIsVMESignal(true);
        return f;
    }

    static Flags defaultSlotFlags()
    {
        Flags f;
        f.setType(Flags::FunctionType);
        f.setIsVMEFunction(true);
        return f;
    }

private:
    friend class QQmlPropertyCache;

    Flags m_flags;
    qint16 m_coreIndex = -1;

    // The notify index is in the range returned by QObjectPrivate::signalIndex().
    // This is different from QMetaMethod::methodIndex().
    qint16 m_notifyIndex = -1;
    qint16 m_overrideIndex = -1;

    qint16 m_metaObjectOffset = -1;

    QTypeRevision m_revision = QTypeRevision::zero();
    QTypeRevision m_typeVersion = QTypeRevision::zero();

    QMetaType m_propType = {};

    union {
        // only for methods (if stored in property cache)
        QQmlPropertyCacheMethodArguments *m_arguments = nullptr;

        // only for C++-declared properties
        StaticMetaCallFunction m_staticMetaCallFunction;

        // only for methods (if stored in lookups)
        const QMetaObject *m_metaObject;

        // only for aliases
        int m_encodedAliasTargetIndex;
    };
};

#if QT_POINTER_SIZE == 4
    Q_STATIC_ASSERT(sizeof(QQmlPropertyData) == 24);
#else // QT_POINTER_SIZE == 8
    Q_STATIC_ASSERT(sizeof(QQmlPropertyData) == 32);
#endif

static_assert(std::is_trivially_copyable<QQmlPropertyData>::value);

bool QQmlPropertyData::operator==(const QQmlPropertyData &other) const
{
    return flags() == other.flags() &&
            propType() == other.propType() &&
            coreIndex() == other.coreIndex() &&
            notifyIndex() == other.notifyIndex() &&
            revision() == other.revision();
}

QQmlPropertyData::Flags::Flags()
    : otherBits(0)
    , isConst(false)
    , isDeepAliasORisVMEFunction(false)
    , isWritableORhasArguments(false)
    , isResettableORisSignal(false)
    , isAliasORisVMESignal(false)
    , isFinalORisV4Function(false)
    , isSignalHandler(false)
    , isOverridableSignal(false)
    , isRequiredORisCloned(false)
    , isConstructorORisBindable(false)
    , isOverridden(false)
    , hasMetaObject(false)
    , type(OtherType)
    , overrideIndexIsProperty(false)
{
}

bool QQmlPropertyData::Flags::operator==(const QQmlPropertyData::Flags &other) const
{
    return isConst == other.isConst &&
            isDeepAliasORisVMEFunction == other.isDeepAliasORisVMEFunction &&
            isWritableORhasArguments == other.isWritableORhasArguments &&
            isResettableORisSignal == other.isResettableORisSignal &&
            isAliasORisVMESignal == other.isAliasORisVMESignal &&
            isFinalORisV4Function == other.isFinalORisV4Function &&
            isOverridden == other.isOverridden &&
            isSignalHandler == other.isSignalHandler &&
            isRequiredORisCloned == other.isRequiredORisCloned &&
            hasMetaObject == other.hasMetaObject &&
            type == other.type &&
            isConstructorORisBindable == other.isConstructorORisBindable &&
            overrideIndexIsProperty == other.overrideIndexIsProperty;
}

void QQmlPropertyData::Flags::copyPropertyTypeFlags(QQmlPropertyData::Flags from)
{
    switch (from.type) {
    case QObjectDerivedType:
    case EnumType:
    case QListType:
    case QVariantType:
        type = from.type;
    }
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlPropertyData::WriteFlags)

QT_END_NAMESPACE

#endif // QQMLPROPERTYDATA_P_H
