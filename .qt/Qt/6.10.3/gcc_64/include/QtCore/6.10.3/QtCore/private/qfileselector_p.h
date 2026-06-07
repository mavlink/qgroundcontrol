// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILESELECTOR_P_H
#define QFILESELECTOR_P_H

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

#include <QtCore/QString>
#include <QtCore/QFileSelector>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

struct QFileSelectorSharedData //Not QSharedData because currently is just a global store
{
    QStringList staticSelectors;
    QStringList preloadedStatics;
};

class Q_CORE_EXPORT QFileSelectorPrivate : QObjectPrivate //Exported for use in other modules (like QtGui)
{
    Q_DECLARE_PUBLIC(QFileSelector)
public:
    static void updateSelectors();
    static QStringList platformSelectors();
    static void addStatics(const QStringList &); //For loading GUI statics from other Qt modules
    static QString selectionHelper(const QString &path, const QString &fileName,
                                   const QStringList &selectors, QChar indicator = u'+');
    QFileSelectorPrivate();
    QString select(const QString &filePath) const;

    QStringList extras;
};

QT_END_NAMESPACE

#endif

