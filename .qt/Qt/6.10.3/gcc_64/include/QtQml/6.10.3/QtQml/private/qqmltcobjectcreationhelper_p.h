// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTCOBJECTCREATIONHELPER_P_H
#define QQMLTCOBJECTCREATIONHELPER_P_H

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

#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qversionnumber.h>
#include <private/qtqmlglobal_p.h>
#include <private/qqmltype_p.h>

#include <array>

QT_BEGIN_NAMESPACE

/*!
    \internal

    (Kind of) type-erased  object creation utility that can be used throughout
    the generated C++ code. By nature it shows relative data to the current QML
    document and allows to get and set object pointers.
 */
class QQmltcObjectCreationHelper
{
    QObject **m_data = nullptr; // QObject* array
    const qsizetype m_size = 0; // size of m_data array, exists for bounds checking
    const qsizetype m_offset = 0; // global offset into m_data array

    qsizetype offset() const { return m_offset; }

public:
    /*!
        Constructs initial "view" from basic data. Supposed to only be called
        once from QQmltcObjectCreationBase.
    */
    QQmltcObjectCreationHelper(QObject **data, qsizetype size) : m_data(data), m_size(size)
    {
        Q_UNUSED(m_size);
    }

    /*!
        Constructs new "view" from \a base view, adding \a localOffset to the
        offset of that base.
    */
    QQmltcObjectCreationHelper(const QQmltcObjectCreationHelper *base, qsizetype localOffset)
        : m_data(base->m_data), m_size(base->m_size), m_offset(base->m_offset + localOffset)
    {
    }

    template<typename T>
    T *get(qsizetype i) const
    {
        Q_ASSERT(m_data);
        Q_ASSERT(i >= 0 && i + offset() < m_size);
        Q_ASSERT(qobject_cast<T *>(m_data[i + offset()]) != nullptr);
        // Note: perform cheap cast as we know *exactly* the real type of the
        // object
        return static_cast<T *>(m_data[i + offset()]);
    }

    void set(qsizetype i, QObject *object)
    {
        Q_ASSERT(m_data);
        Q_ASSERT(i >= 0 && i + offset() < m_size);
        Q_ASSERT(m_data[i + offset()] == nullptr); // prevent accidental resets
        m_data[i + offset()] = object;
    }

    template<typename T>
    static constexpr uint typeCount() noexcept
    {
        return T::q_qmltc_typeCount();
    }
};

/*!
    \internal

    Base helper for qmltc-generated types that linearly stores pointers to all
    the to-be-created objects for fast access during object creation.
 */
template<typename QmltcGeneratedType>
class QQmltcObjectCreationBase
{
    // Note: +1 for the document root itself
    std::array<QObject *, QmltcGeneratedType::q_qmltc_typeCount() + 1> m_objects = {};

public:
    QQmltcObjectCreationHelper view()
    {
        return QQmltcObjectCreationHelper(m_objects.data(), m_objects.size());
    }
};

struct QmltcTypeData
{
    QQmlType::RegistrationType regType = QQmlType::CppType;
    int allocationSize = 0;
    const QMetaObject *metaObject = nullptr;

    template<typename QmltcGeneratedType>
    QmltcTypeData(QmltcGeneratedType *)
        : allocationSize(sizeof(QmltcGeneratedType)),
          metaObject(&QmltcGeneratedType::staticMetaObject)
    {
    }
};

Q_QML_EXPORT void qmltcCreateDynamicMetaObject(QObject *object, const QmltcTypeData &data);

QT_END_NAMESPACE

#endif // QQMLTCOBJECTCREATIONHELPER_P_H
