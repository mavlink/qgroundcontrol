// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFSCOMPLETOR_P_H
#define QFSCOMPLETOR_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qcompleter.h"
#include <QtGui/qfilesystemmodel.h>

QT_REQUIRE_CONFIG(fscompleter);

QT_BEGIN_NAMESPACE

/*!
    QCompleter that can deal with QFileSystemModel
  */
class Q_WIDGETS_EXPORT QFSCompleter :  public QCompleter {
public:
    explicit QFSCompleter(QFileSystemModel *model, QObject *parent = nullptr)
        : QCompleter(model, parent), proxyModel(nullptr), sourceModel(model)
    {
#if defined(Q_OS_WIN)
        setCaseSensitivity(Qt::CaseInsensitive);
#endif
    }
    QString pathFromIndex(const QModelIndex &index) const override;
    QStringList splitPath(const QString& path) const override;

    QAbstractProxyModel *proxyModel;
    QFileSystemModel *sourceModel;
};

QT_END_NAMESPACE

#endif // QFSCOMPLETOR_P_H

