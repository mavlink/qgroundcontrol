// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLVALUETYPEPROXYBINDING_P_H
#define QQMLVALUETYPEPROXYBINDING_P_H

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

#include <private/qqmlabstractbinding_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQmlValueTypeProxyBinding : public QQmlAbstractBinding
{
public:
    QQmlValueTypeProxyBinding(QObject *o, QQmlPropertyIndex coreIndex);

    QQmlAbstractBinding *subBindings() const;
    QQmlAbstractBinding *binding(QQmlPropertyIndex targetPropertyIndex) const;
    void removeBindings(quint32 mask);

    void setEnabled(bool, QQmlPropertyData::WriteFlags) override;
    Kind kind() const final { return QQmlAbstractBinding::ValueTypeProxy; }

protected:
    ~QQmlValueTypeProxyBinding();

private:
    friend class QQmlAbstractBinding;
    Ptr m_bindings;
};

QT_END_NAMESPACE

#endif // QQMLVALUETYPEPROXYBINDING_P_H
