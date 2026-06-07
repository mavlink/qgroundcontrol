// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMFIELDFILTER_P_H
#define QQMLDOMFIELDFILTER_P_H

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

#include "qqmldom_fwd_p.h"
#include "qqmldom_global.h"
#include "qqmldompath_p.h"

#include <QtCore/qobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtQml/private/qqmljsastvisitor_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT FieldFilter
{
    Q_GADGET
public:
    QString describeFieldsFilter() const;
    bool addFilter(const QString &f);
    bool operator()(const DomItem &, const Path &, const DomItem &) const;
    bool operator()(const DomItem &, const PathEls::PathComponent &c, const DomItem &) const;
    static FieldFilter noFilter();
    static FieldFilter defaultFilter();
    static FieldFilter noLocationFilter();
    static FieldFilter compareFilter();
    static FieldFilter compareNoCommentsFilter();
    void setFiltred();
    const QMultiMap<QString, QString> &fieldFilterAdd() const { return m_fieldFilterAdd; }
    QMultiMap<QString, QString> fieldFilterRemove() const { return m_fieldFilterRemove; }
    QSet<DomType> filtredTypes;

    FieldFilter(const QMultiMap<QString, QString> &fieldFilterAdd = {},
                const QMultiMap<QString, QString> &fieldFilterRemove = {})
        : m_fieldFilterAdd(fieldFilterAdd), m_fieldFilterRemove(fieldFilterRemove)
    {
        setFiltred();
    }

private:
    QMultiMap<QString, QString> m_fieldFilterAdd;
    QMultiMap<QString, QString> m_fieldFilterRemove;
    QSet<DomType> m_filtredTypes;
    QSet<size_t> m_filtredFields;
    bool m_filtredDefault = true;
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QQMLDOMFIELDFILTER_P_H
