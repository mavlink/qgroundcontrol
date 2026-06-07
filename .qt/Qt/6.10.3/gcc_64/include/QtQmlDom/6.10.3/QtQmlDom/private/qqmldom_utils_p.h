// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOM_UTILS_P_H
#define QQMLDOM_UTILS_P_H

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

#include <QtCore/qglobal.h>
#include "qqmldom_fwd_p.h"
#include "qqmldomconstants_p.h"
#include <QtQml/private/qqmljssourcelocation_p.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qcborvalue.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QQmlJSDomImporting);

template<class... Ts>
struct qOverloadedVisitor : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
qOverloadedVisitor(Ts...) -> qOverloadedVisitor<Ts...>;

namespace QQmlJS {
namespace Dom {

QString fileLocationRegionName(FileLocationRegion region);
FileLocationRegion fileLocationRegionValue(QStringView region);

QCborValue sourceLocationToQCborValue(SourceLocation loc);

} // namespace Dom
}; // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOM_UTILS_P_H
