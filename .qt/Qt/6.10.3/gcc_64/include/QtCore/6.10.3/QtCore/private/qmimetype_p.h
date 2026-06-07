// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMIMETYPE_P_H
#define QMIMETYPE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qshareddata.h>

QT_REQUIRE_CONFIG(mimetype);

#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QMimeTypePrivate : public QSharedData
{
public:
    typedef QHash<QString, QString> LocaleHash;

    QMimeTypePrivate() { }
    explicit QMimeTypePrivate(const QString &name) : name(name) { }

    QString name;
};

QT_END_NAMESPACE

#endif // QMIMETYPE_P_H
