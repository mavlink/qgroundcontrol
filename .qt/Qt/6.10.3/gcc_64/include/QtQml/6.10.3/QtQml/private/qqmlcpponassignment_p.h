// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCPPONASSIGNMENT_P_H
#define QQMLCPPONASSIGNMENT_P_H

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

#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <QtQml/qqmlpropertyvaluesource.h>

QT_BEGIN_NAMESPACE

/*! \internal

    Helper class that provides setTarget() functionality for both value
    interceptors and value sources.

    Property value sources could be problematic because QQuickAbstractAnimation
    changes access specifier of QQmlPropertyValueSource::setTarget() to private
    (unintentionally?). This API allows to avoid manual casts to base types as
    the C++ compiler would implicitly cast derived classes in this case.
*/
struct Q_QML_EXPORT QQmlCppOnAssignmentHelper
{
    // TODO: in theory, this API might just accept QObject * and int that would
    // give the QMetaProperty. using the meta property, one could create
    // QQmlProperty with a call to QQmlProperty::restore() (if there's an
    // overload that takes QMetaProperty instead of QQmlPropertyData - which is
    // also possible to add by using QQmlPropertyData::load())
    static void set(QQmlPropertyValueInterceptor *interceptor, const QQmlProperty &property);
    static void set(QQmlPropertyValueSource *valueSource, const QQmlProperty &property);
};

QT_END_NAMESPACE

#endif // QQMLCPPONASSIGNMENT_P_H
