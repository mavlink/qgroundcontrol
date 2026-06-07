// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TEXTBUILDER_H
#define TEXTBUILDER_H

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

#include "uilib_global.h"
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QDir;
class QVariant;

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

class DomProperty;
class DomString;

class QDESIGNER_UILIB_EXPORT QTextBuilder
{
public:
    QTextBuilder();
    virtual ~QTextBuilder();

    virtual QVariant loadText(const DomProperty *property) const;

    virtual QVariant toNativeValue(const QVariant &value) const;

    virtual DomProperty *saveText(const QVariant &value) const;
};


#ifdef QFORMINTERNAL_NAMESPACE
}
#endif

QT_END_NAMESPACE

#endif // TEXTBUILDER_H
