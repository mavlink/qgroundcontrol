// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTY_P_H
#define QQMLPROPERTY_P_H

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

#include "qqmlproperty.h"

#include <private/qobject_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlpropertydata_p.h>
#include <private/qqmlpropertyindex_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtQml/qqmlengine.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlEnginePrivate;
class QQmlJavaScriptExpression;
class QQmlMetaObject;
class QQmlAbstractBinding;
class QQmlBoundSignalExpression;

class Q_QML_EXPORT QQmlPropertyPrivate final : public QQmlRefCounted<QQmlPropertyPrivate>
{
public:
    enum class InitFlag {
        None        = 0x0,
        AllowId     = 0x1,
        AllowSignal = 0x2
    };
    Q_DECLARE_FLAGS(InitFlags, InitFlag);

    // like SequencePrototype, but we want to avoid tight coupling
    enum class ListCopyResult
    {
        Copied,
        WasEqual,
        TypeMismatch
    };

    QQmlRefPointer<QQmlContextData> context;
    QPointer<QQmlEngine> engine;
    QPointer<QObject> object;

    QQmlPropertyData core;
    QQmlPropertyData valueTypeData;

    QString nameCache;

    // ### Qt7: Get rid of this.
    static bool resolveUrlsOnAssignment();

    QQmlPropertyPrivate() {}

    QQmlPropertyIndex encodedIndex() const
    { return encodedIndex(core, valueTypeData); }
    static QQmlPropertyIndex encodedIndex(const QQmlPropertyData &core, const QQmlPropertyData &valueTypeData)
    { return QQmlPropertyIndex(core.coreIndex(), valueTypeData.coreIndex()); }

    QQmlRefPointer<QQmlContextData> effectiveContext() const;

    void initProperty(QObject *obj, const QString &name, InitFlags flags = InitFlag::None);
    void initDefault(QObject *obj);

    bool isValueType() const;
    QMetaType propertyType() const;
    QQmlProperty::Type type() const;
    QQmlProperty::PropertyTypeCategory propertyTypeCategory() const;

    QVariant readValueProperty();
    bool writeValueProperty(const QVariant &, QQmlPropertyData::WriteFlags);

    static QQmlMetaObject rawMetaObjectForType(QMetaType metaType);
    static bool writeEnumProperty(const QMetaProperty &prop, int idx, QObject *object,
                                  const QVariant &value, int flags);
    static bool writeValueProperty(QObject *,
                                   const QQmlPropertyData &, const QQmlPropertyData &valueTypeData,
                                   const QVariant &, const QQmlRefPointer<QQmlContextData> &,
                                   QQmlPropertyData::WriteFlags flags = {});
    static bool resetValueProperty(QObject *,
                                   const QQmlPropertyData &, const QQmlPropertyData &valueTypeData,
                                   const QQmlRefPointer<QQmlContextData> &,
                                   QQmlPropertyData::WriteFlags flags = {});

    /*!
      \internal
      Attempts to convert \a value to a QQmlListProperty. The existing \a listProperty will be modified (so use a
      temporary one if that is not desired).
      \a listProperty is passed as a QQmlListProperty<QObject>, but might actually be a list property of a more specific
      type. The actual type of the list property is given by \a actualListType.
    */
    static ListCopyResult convertToQQmlListProperty(QQmlListProperty<QObject> *listProperty,
                                                    QMetaType actualListType,
                                                    const QVariant &value);
    static QVariant convertToWriteTargetType(const QVariant &value, QMetaType targetMetaType);
    static bool write(QObject *, const QQmlPropertyData &, const QVariant &,
                      const QQmlRefPointer<QQmlContextData> &,
                      QQmlPropertyData::WriteFlags flags = {});
    static bool reset(QObject *, const QQmlPropertyData &,
                      QQmlPropertyData::WriteFlags flags = {});
    static void findAliasTarget(QObject *, QQmlPropertyIndex, QObject **, QQmlPropertyIndex *);

    struct ResolvedAlias
    {
        QObject *targetObject;
        QQmlPropertyIndex targetIndex;
    };
    /*!
        \internal
        Given an alias property specified by \a baseObject and \a baseIndex, this function
        computes the alias target.
     */
    static ResolvedAlias findAliasTarget(QObject *baseObject, QQmlPropertyIndex baseIndex);

    enum BindingFlag {
        None = 0,
        DontEnable = 0x1,
        OverrideSticky = 0x2
    };
    Q_DECLARE_FLAGS(BindingFlags, BindingFlag)

    static void setBinding(QQmlAbstractBinding *binding, BindingFlags flags = None,
                           QQmlPropertyData::WriteFlags writeFlags = QQmlPropertyData::DontRemoveBinding);

    static bool removeBinding(const QQmlProperty &that, BindingFlags flags = None);
    static bool removeBinding(QObject *o, QQmlPropertyIndex index, BindingFlags flags = None);
    static bool removeBinding(QQmlAbstractBinding *b, QQmlPropertyPrivate::BindingFlags flags = None);
    static QQmlAbstractBinding *binding(QObject *, QQmlPropertyIndex index);

    static QQmlProperty restore(QObject *, const QQmlPropertyData &, const QQmlPropertyData *,
                                const QQmlRefPointer<QQmlContextData> &);

    int signalIndex() const;

    static inline QQmlPropertyPrivate *get(const QQmlProperty &p) { return p.d; }

    // "Public" (to QML) methods
    static QQmlAbstractBinding *binding(const QQmlProperty &that);
    static void setBinding(const QQmlProperty &that, QQmlAbstractBinding *);
    static QQmlBoundSignalExpression *signalExpression(const QQmlProperty &that);
    static void setSignalExpression(const QQmlProperty &that, QQmlBoundSignalExpression *);
    static void takeSignalExpression(const QQmlProperty &that, QQmlBoundSignalExpression *);
    static bool write(const QQmlProperty &that, const QVariant &, QQmlPropertyData::WriteFlags);
    static QQmlPropertyIndex propertyIndex(const QQmlProperty &that);
    static QMetaMethod findSignalByName(const QMetaObject *mo, const QByteArray &);
    static QMetaProperty findPropertyByName(const QMetaObject *mo, const QByteArray &);
    static bool connect(const QObject *sender, int signal_index,
                        const QObject *receiver, int method_index,
                        int type = 0, int *types = nullptr);
    static void flushSignal(const QObject *sender, int signal_index);

    static QList<QUrl> urlSequence(const QVariant &value);
    static QList<QUrl> urlSequence(
            const QVariant &value, const QQmlRefPointer<QQmlContextData> &ctxt);
    static QQmlProperty create(
            QObject *target, const QString &propertyName,
            const QQmlRefPointer<QQmlContextData> &context,
            QQmlPropertyPrivate::InitFlags flags);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlPropertyPrivate::BindingFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlPropertyPrivate::InitFlags);

QT_END_NAMESPACE

#endif // QQMLPROPERTY_P_H
