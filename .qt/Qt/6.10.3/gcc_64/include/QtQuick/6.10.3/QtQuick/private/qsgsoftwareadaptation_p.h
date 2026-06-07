// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PLUGINMAIN_H
#define PLUGINMAIN_H

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

#include <private/qsgcontextplugin_p.h>

QT_BEGIN_NAMESPACE

class QSGContext;
class QSGRenderLoop;
class QSGSoftwareContext;

class QSGSoftwareAdaptation : public QSGContextPlugin
{
public:
    QSGSoftwareAdaptation(QObject *parent = nullptr);

    QStringList keys() const override;
    QSGContext *create(const QString &key) const override;
    QSGContextFactoryInterface::Flags flags(const QString &key) const override;
    QSGRenderLoop *createWindowManager() override;
private:
    static QSGSoftwareContext *instance;
};

QT_END_NAMESPACE

#endif // PLUGINMAIN_H
