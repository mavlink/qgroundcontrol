// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4PERSISTENT_H
#define QV4PERSISTENT_H

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

#include "qv4value_p.h"
#include "qv4managed_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_EXPORT PersistentValueStorage
{
    PersistentValueStorage(ExecutionEngine *engine);
    ~PersistentValueStorage();

    Value *allocate();
    static void free(Value *v)
    {
        if (v)
            freeUnchecked(v);
    }

    void mark(MarkStack *markStack);

    struct Iterator {
        Iterator(void *p, int idx);
        Iterator(const Iterator &o);
        Iterator & operator=(const Iterator &o);
        ~Iterator();
        void *p;
        int index;
        Iterator &operator++();
        bool operator !=(const Iterator &other) {
            return p != other.p || index != other.index;
        }
        Value &operator *();
    };
    Iterator begin() { return Iterator(firstPage, 0); }
    Iterator end() { return Iterator(nullptr, 0); }

    void clearFreePageHint();

    static ExecutionEngine *getEngine(const Value *v);

    ExecutionEngine *engine;
    void *firstPage;
    void *freePageHint = nullptr;
private:
    static void freeUnchecked(Value *v);
    static void freePage(void *page);
};

class Q_QML_EXPORT PersistentValue
{
public:
    constexpr PersistentValue() noexcept = default;
    PersistentValue(const PersistentValue &other);
    PersistentValue &operator=(const PersistentValue &other);

    PersistentValue(PersistentValue &&other) noexcept : val(std::exchange(other.val, nullptr)) {}
    void swap(PersistentValue &other) noexcept { qt_ptr_swap(val, other.val); }
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(PersistentValue)
    ~PersistentValue() { PersistentValueStorage::free(val); }

    PersistentValue &operator=(const WeakValue &other);
    PersistentValue &operator=(Object *object);

    PersistentValue(ExecutionEngine *engine, const Value &value);
    PersistentValue(ExecutionEngine *engine, ReturnedValue value);
    PersistentValue(ExecutionEngine *engine, Object *object);

    void set(ExecutionEngine *engine, const Value &value);
    void set(ExecutionEngine *engine, ReturnedValue value);
    void set(ExecutionEngine *engine, Heap::Base *obj);

    ReturnedValue value() const {
        return (val ? val->asReturnedValue() : Encode::undefined());
    }
    Value *valueRef() const {
        return val;
    }
    Managed *asManaged() const {
        if (!val)
            return nullptr;
        return val->managed();
    }
    template<typename T>
    T *as() const {
        if (!val)
            return nullptr;
        return val->as<T>();
    }

    ExecutionEngine *engine() const {
        if (!val)
            return nullptr;
        return PersistentValueStorage::getEngine(val);
    }

    bool isUndefined() const { return !val || val->isUndefined(); }
    bool isNullOrUndefined() const { return !val || val->isNullOrUndefined(); }
    void clear() {
        PersistentValueStorage::free(val);
        val = nullptr;
    }
    bool isEmpty() { return !val; }

private:
    Value *val = nullptr;
};

class Q_QML_EXPORT WeakValue
{
public:
    WeakValue() {}
    WeakValue(const WeakValue &other);
    WeakValue(ExecutionEngine *engine, const Value &value);
    WeakValue &operator=(const WeakValue &other);
    ~WeakValue();

    void set(ExecutionEngine *engine, const Value &value);

    void set(ExecutionEngine *engine, ReturnedValue value);

    void set(ExecutionEngine *engine, Heap::Base *obj);

    ReturnedValue value() const {
        return (val ? val->asReturnedValue() : Encode::undefined());
    }
    Value *valueRef() const {
        return val;
    }
    Managed *asManaged() const {
        if (!val)
            return nullptr;
        return val->managed();
    }
    template <typename T>
    T *as() const {
        if (!val)
            return nullptr;
        return val->as<T>();
    }

    ExecutionEngine *engine() const {
        if (!val)
            return nullptr;
        return PersistentValueStorage::getEngine(val);
    }

    bool isUndefined() const { return !val || val->isUndefined(); }
    bool isNullOrUndefined() const { return !val || val->isNullOrUndefined(); }
    void clear() { free(); }

    void markOnce(MarkStack *markStack);

private:
    Value *val = nullptr;

private:
    Q_NEVER_INLINE void allocVal(ExecutionEngine *engine);

    void free();
};

} // namespace QV4

QT_END_NAMESPACE

#endif
