// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTOPTIONSPAGE_P_H
#define ABSTRACTOPTIONSPAGE_P_H

#include <QtDesigner/sdk_global.h>

QT_BEGIN_NAMESPACE

class QString;
class QWidget;

class QDESIGNER_SDK_EXPORT QDesignerOptionsPageInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerOptionsPageInterface)

    QDesignerOptionsPageInterface() = default;
    virtual ~QDesignerOptionsPageInterface() = default;

    virtual QString name() const = 0;
    virtual QWidget *createPage(QWidget *parent) = 0;
    virtual void apply() = 0;
    virtual void finish() = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTOPTIONSPAGE_P_H
