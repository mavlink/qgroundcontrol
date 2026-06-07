// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFILEICONPROVIDER_P_H
#define QFILEICONPROVIDER_P_H

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
#include "qfileiconprovider.h"

#include <private/qabstractfileiconprovider_p.h>
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtWidgets/qstyle.h>

QT_BEGIN_NAMESPACE

class QFileInfo;

class QFileIconProviderPrivate : public QAbstractFileIconProviderPrivate
{
    Q_DECLARE_PUBLIC(QFileIconProvider)

public:
    QFileIconProviderPrivate(QFileIconProvider *q);
    QIcon getIcon(QStyle::StandardPixmap name) const;
    QIcon getIcon(const QFileInfo &fi) const;

    const QString homePath;

private:
    mutable QIcon file;
    mutable QIcon fileLink;
    mutable QIcon directory;
    mutable QIcon directoryLink;
    mutable QIcon harddisk;
    mutable QIcon floppy;
    mutable QIcon cdrom;
    mutable QIcon ram;
    mutable QIcon network;
    mutable QIcon computer;
    mutable QIcon desktop;
    mutable QIcon trashcan;
    mutable QIcon generic;
    mutable QIcon home;
};

QT_END_NAMESPACE

#endif // QFILEICONPROVIDER_P_H
