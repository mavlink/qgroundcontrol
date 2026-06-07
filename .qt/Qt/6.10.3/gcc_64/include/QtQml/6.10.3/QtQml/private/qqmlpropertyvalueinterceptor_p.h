// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYVALUEINTERCEPTOR_P_H
#define QQMLPROPERTYVALUEINTERCEPTOR_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmlpropertyindex_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>

QT_BEGIN_NAMESPACE

class QQmlProperty;
class Q_QML_EXPORT QQmlPropertyValueInterceptor
{
public:
    QQmlPropertyValueInterceptor();
    virtual ~QQmlPropertyValueInterceptor();
    virtual void setTarget(const QQmlProperty &property) = 0;
    virtual void write(const QVariant &value) = 0;
    virtual bool bindable(QUntypedBindable *bindable, QUntypedBindable target);

private:
    friend class QQmlInterceptorMetaObject;

    QQmlPropertyIndex m_propertyIndex;
    QQmlPropertyValueInterceptor *m_next;
};

#define QQmlPropertyValueInterceptor_iid "org.qt-project.Qt.QQmlPropertyValueInterceptor"

Q_DECLARE_INTERFACE(QQmlPropertyValueInterceptor, QQmlPropertyValueInterceptor_iid)

QT_END_NAMESPACE

#endif // QQMLPROPERTYVALUEINTERCEPTOR_P_H
