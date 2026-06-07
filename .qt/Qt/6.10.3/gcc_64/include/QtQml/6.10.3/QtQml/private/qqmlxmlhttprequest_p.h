// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLXMLHTTPREQUEST_P_H
#define QQMLXMLHTTPREQUEST_P_H

#include <QtQml/qjsengine.h>
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
#include <private/qtqmlglobal_p.h>

QT_REQUIRE_CONFIG(qml_xml_http_request);

QT_BEGIN_NAMESPACE

void *qt_add_qmlxmlhttprequest(QV4::ExecutionEngine *engine);
void qt_rem_qmlxmlhttprequest(QV4::ExecutionEngine *engine, void *);

QT_END_NAMESPACE

#endif // QQMLXMLHTTPREQUEST_P_H

