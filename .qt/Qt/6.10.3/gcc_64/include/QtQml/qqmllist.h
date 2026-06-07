// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLIST_H
#define QQMLLIST_H

#include <QtQml/qtqmlglobal.h>

#include <QtCore/qcontainerinfo.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QObject;
struct QMetaObject;

#define QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_APPEND Q_CLASSINFO("QML.ListPropertyAssignBehavior", "Append")
#define QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT Q_CLASSINFO("QML.ListPropertyAssignBehavior", "ReplaceIfNotDefault")
#define QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE Q_CLASSINFO("QML.ListPropertyAssignBehavior", "Replace")

template<typename T>
class QQmlListProperty {
public:
    using value_type = T*;

    using AppendFunction = void (*)(QQmlListProperty<T> *, T *);
    using CountFunction = qsizetype (*)(QQmlListProperty<T> *);
    using AtFunction = T *(*)(QQmlListProperty<T> *, qsizetype);
    using ClearFunction = void (*)(QQmlListProperty<T> *);
    using ReplaceFunction = void (*)(QQmlListProperty<T> *, qsizetype, T *);
    using RemoveLastFunction = void (*)(QQmlListProperty<T> *);

    QQmlListProperty() = default;

    QQmlListProperty(QObject *o, QList<T *> *list)
        : object(o), data(list), append(qlist_append), count(qlist_count), at(qlist_at),
          clear(qlist_clear), replace(qlist_replace), removeLast(qlist_removeLast)
    {}

    QQmlListProperty(QObject *o, void *d, AppendFunction a, CountFunction c, AtFunction t,
                    ClearFunction r )
        : object(o),
          data(d),
          append(a),
          count(c),
          at(t),
          clear(r),
          replace((a && c && t && r) ? qslow_replace : nullptr),
          removeLast((a && c && t && r) ? qslow_removeLast : nullptr)
    {}

    QQmlListProperty(QObject *o, void *d, AppendFunction a, CountFunction c, AtFunction t,
                     ClearFunction r, ReplaceFunction s, RemoveLastFunction p)
        : object(o),
          data(d),
          append(a),
          count(c),
          at(t),
          clear((!r && p && c) ? qslow_clear : r),
          replace((!s && a && c && t && (r || p)) ? qslow_replace : s),
          removeLast((!p && a && c && t && r) ? qslow_removeLast : p)
    {}

    QQmlListProperty(QObject *o, void *d, CountFunction c, AtFunction a)
        : object(o), data(d), count(c), at(a)
    {}

    bool operator==(const QQmlListProperty &o) const {
        return object == o.object &&
               data == o.data &&
               append == o.append &&
               count == o.count &&
               at == o.at &&
               clear == o.clear &&
               replace == o.replace &&
               removeLast == o.removeLast;
    }

    QObject *object = nullptr;
    void *data = nullptr;

    AppendFunction append = nullptr;
    CountFunction count = nullptr;
    AtFunction at = nullptr;
    ClearFunction clear = nullptr;
    ReplaceFunction replace = nullptr;
    RemoveLastFunction removeLast = nullptr;

    template<typename List>
    List toList()
    {
        if constexpr (std::is_same_v<List, QList<T *>>) {
            if (append == qlist_append)
                return *static_cast<QList<T *> *>(data);
        }
        return toList_impl<List>();
    }

private:
    template<typename List>
    List toList_impl()
    {
        const qsizetype size = count(this);

        List result;
        if constexpr (QContainerInfo::has_reserve_v<List>)
            result.reserve(size);

        static_assert(QContainerInfo::has_push_back_v<List>);
        for (qsizetype i = 0; i < size; ++i)
            result.push_back(at(this, i));

        return result;
    }

private:
    static void qlist_append(QQmlListProperty *p, T *v) {
        static_cast<QList<T *> *>(p->data)->append(v);
    }
    static qsizetype qlist_count(QQmlListProperty *p) {
        return static_cast<QList<T *> *>(p->data)->size();
    }
    static T *qlist_at(QQmlListProperty *p, qsizetype idx) {
        return static_cast<QList<T *> *>(p->data)->at(idx);
    }
    static void qlist_clear(QQmlListProperty *p) {
        return static_cast<QList<T *> *>(p->data)->clear();
    }
    static void qlist_replace(QQmlListProperty *p, qsizetype idx, T *v) {
        return static_cast<QList<T *> *>(p->data)->replace(idx, v);
    }
    static void qlist_removeLast(QQmlListProperty *p) {
        return static_cast<QList<T *> *>(p->data)->removeLast();
    }

    static void qslow_replace(QQmlListProperty<T> *list, qsizetype idx, T *v)
    {
        const qsizetype length = list->count(list);
        if (idx < 0 || idx >= length)
            return;

        QVector<T *> stash;
        if (list->clear != qslow_clear) {
            stash.reserve(length);
            for (qsizetype i = 0; i < length; ++i)
                stash.append(i == idx ? v : list->at(list, i));
            list->clear(list);
            for (T *item : std::as_const(stash))
                list->append(list, item);
        } else {
            stash.reserve(length - idx - 1);
            for (qsizetype i = length - 1; i > idx; --i) {
                stash.append(list->at(list, i));
                list->removeLast(list);
            }
            list->removeLast(list);
            list->append(list, v);
            while (!stash.isEmpty())
                list->append(list, stash.takeLast());
        }
    }

    static void qslow_clear(QQmlListProperty<T> *list)
    {
        for (qsizetype i = 0, end = list->count(list); i < end; ++i)
            list->removeLast(list);
    }

    static void qslow_removeLast(QQmlListProperty<T> *list)
    {
        const qsizetype length = list->count(list) - 1;
        if (length < 0)
            return;
        QVector<T *> stash;
        stash.reserve(length);
        for (qsizetype i = 0; i < length; ++i)
            stash.append(list->at(list, i));
        list->clear(list);
        for (T *item : std::as_const(stash))
            list->append(list, item);
    }
};

class QQmlEngine;
class QQmlListReferencePrivate;
class Q_QML_EXPORT QQmlListReference
{
public:
    QQmlListReference();

#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_X("Drop the QQmlEngine* argument")
    QQmlListReference(const QVariant &variant, [[maybe_unused]] QQmlEngine *engine);

    QT_DEPRECATED_X("Drop the QQmlEngine* argument")
    QQmlListReference(QObject *o, const char *property, [[maybe_unused]] QQmlEngine *engine);
#endif

    explicit QQmlListReference(const QVariant &variant);
    QQmlListReference(QObject *o, const char *property);
    QQmlListReference(const QQmlListReference &);
    QQmlListReference &operator=(const QQmlListReference &);
    ~QQmlListReference();

    bool isValid() const;

    QObject *object() const;
    const QMetaObject *listElementType() const;

    bool canAppend() const;
    bool canAt() const;
    bool canClear() const;
    bool canCount() const;
    bool canReplace() const;
    bool canRemoveLast() const;

    bool isManipulable() const;
    bool isReadable() const;

    bool append(QObject *) const;
    QObject *at(qsizetype) const;
    bool clear() const;
    qsizetype count() const;
    qsizetype size() const { return count(); }
    bool replace(qsizetype, QObject *) const;
    bool removeLast() const;
    bool operator==(const QQmlListReference &other) const {return d == other.d;}

private:
    friend class QQmlListReferencePrivate;
    QQmlListReferencePrivate* d;
};

namespace QtPrivate {
template<typename T>
inline constexpr bool IsQmlListType<QQmlListProperty<T>> = true;
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlListReference)

#endif // QQMLLIST_H
