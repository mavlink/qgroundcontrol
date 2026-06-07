// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTVARIABLEAXIS_H
#define QFONTVARIABLEAXIS_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qfont.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QFontVariableAxisPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QFontVariableAxisPrivate)

class QFontVariableAxis
{
    Q_GADGET_EXPORT(Q_GUI_EXPORT)
    Q_DECLARE_PRIVATE(QFontVariableAxis)

    Q_PROPERTY(QByteArray tag READ tagString CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(qreal minimumValue READ minimumValue CONSTANT)
    Q_PROPERTY(qreal maximumValue READ maximumValue CONSTANT)
    Q_PROPERTY(qreal defaultValue READ defaultValue CONSTANT)
public:
    Q_GUI_EXPORT QFontVariableAxis();
    QFontVariableAxis(QFontVariableAxis &&other) noexcept = default;
    Q_GUI_EXPORT QFontVariableAxis(const QFontVariableAxis &axis);
    Q_GUI_EXPORT ~QFontVariableAxis();
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QFontVariableAxis)
    void swap(QFontVariableAxis &other) noexcept
    {
        d_ptr.swap(other.d_ptr);
    }

    Q_GUI_EXPORT QFontVariableAxis &operator=(const QFontVariableAxis &axis);

    Q_GUI_EXPORT QFont::Tag tag() const;
    Q_GUI_EXPORT void setTag(QFont::Tag tag);

    Q_GUI_EXPORT QString name() const;
    Q_GUI_EXPORT void setName(const QString &name);

    Q_GUI_EXPORT qreal minimumValue() const;
    Q_GUI_EXPORT void setMinimumValue(qreal minimumValue);

    Q_GUI_EXPORT qreal maximumValue() const;
    Q_GUI_EXPORT void setMaximumValue(qreal maximumValue);

    Q_GUI_EXPORT qreal defaultValue() const;
    Q_GUI_EXPORT void setDefaultValue(qreal defaultValue);

private:
    QByteArray tagString() const { return tag().toString(); }
    void detach();

#ifndef QT_NO_DEBUG_STREAM
    Q_GUI_EXPORT friend QDebug operator<<(QDebug debug, const QFontVariableAxis &axis);
#endif

    QExplicitlySharedDataPointer<QFontVariableAxisPrivate> d_ptr;
};

Q_DECLARE_SHARED(QFontVariableAxis)

QT_END_NAMESPACE

#endif // QFONTVARIABLEAXIS_H

