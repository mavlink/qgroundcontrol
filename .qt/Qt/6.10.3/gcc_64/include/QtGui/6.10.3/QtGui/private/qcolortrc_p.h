// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRC_P_H
#define QCOLORTRC_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "qcolortransferfunction_p.h"
#include "qcolortransfergeneric_p.h"
#include "qcolortransfertable_p.h"

QT_BEGIN_NAMESPACE

// Defines a TRC (Tone Reproduction Curve)
class Q_GUI_EXPORT QColorTrc
{
public:
    QColorTrc() noexcept : m_type(Type::Uninitialized) { }
    QColorTrc(const QColorTransferFunction &fun) : m_type(Type::ParameterizedFunction), m_fun(fun) { }
    QColorTrc(const QColorTransferTable &table) : m_type(Type::Table), m_table(table) { }
    QColorTrc(const QColorTransferGenericFunction &hdr) : m_type(Type::GenericFunction), m_hdr(hdr) { }
    QColorTrc(QColorTransferFunction &&fun) noexcept : m_type(Type::ParameterizedFunction), m_fun(std::move(fun)) { }
    QColorTrc(QColorTransferTable &&table) noexcept : m_type(Type::Table), m_table(std::move(table)) { }
    QColorTrc(QColorTransferGenericFunction &&hdr) noexcept : m_type(Type::GenericFunction), m_hdr(std::move(hdr)) { }

    enum class Type {
        Uninitialized,
        ParameterizedFunction,
        GenericFunction,
        Table,
    };

    bool isIdentity() const
    {
        return (m_type == Type::ParameterizedFunction && m_fun.isIdentity())
            || (m_type == Type::Table && m_table.isIdentity());
    }
    bool isValid() const
    {
        return m_type != Type::Uninitialized;
    }
    float apply(float x) const
    {
        switch (m_type) {
        case Type::ParameterizedFunction:
            return fun().apply(x);
        case Type::GenericFunction:
            return hdr().apply(x);
        case Type::Table:
            return table().apply(x);
        default:
            break;
        }
        return x;
    }
    float applyExtended(float x) const
    {
        switch (m_type) {
        case Type::ParameterizedFunction:
            return std::copysign(fun().apply(std::abs(x)), x);
        case Type::GenericFunction:
            return hdr().apply(x);
        case Type::Table:
            return table().apply(x);
        default:
            break;
        }
        return x;
    }
    float applyInverse(float x) const
    {
        switch (m_type) {
        case Type::ParameterizedFunction:
            return fun().inverted().apply(x);
        case Type::GenericFunction:
            return hdr().applyInverse(x);
        case Type::Table:
            return table().applyInverse(x);
        default:
            break;
        }
        return x;
    }
    float applyInverseExtended(float x) const
    {
        switch (m_type) {
        case Type::ParameterizedFunction:
            return std::copysign(applyInverse(std::abs(x)), x);
        case Type::GenericFunction:
            return hdr().applyInverse(x);
        case Type::Table:
            return table().applyInverse(x);
        default:
            break;
        }
        return x;
    }

    const QColorTransferTable &table() const { return m_table; }
    const QColorTransferFunction &fun() const{ return m_fun; }
    const QColorTransferGenericFunction &hdr() const { return m_hdr; }
    Type type() const noexcept { return m_type; }

    Type m_type;

    friend inline bool comparesEqual(const QColorTrc &lhs, const QColorTrc &rhs);
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QColorTrc)

    QColorTransferFunction m_fun;
    QColorTransferTable m_table;
    QColorTransferGenericFunction m_hdr;
};

inline bool comparesEqual(const QColorTrc &o1, const QColorTrc &o2)
{
    if (o1.m_type != o2.m_type)
        return false;
    if (o1.m_type == QColorTrc::Type::ParameterizedFunction)
        return o1.m_fun == o2.m_fun;
    if (o1.m_type == QColorTrc::Type::Table)
        return o1.m_table == o2.m_table;
    if (o1.m_type == QColorTrc::Type::GenericFunction)
        return o1.m_hdr == o2.m_hdr;
    return true;
}

QT_END_NAMESPACE

#endif // QCOLORTRC
