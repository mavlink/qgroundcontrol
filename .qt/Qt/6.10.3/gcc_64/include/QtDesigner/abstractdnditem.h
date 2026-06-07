// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTDNDITEM_H
#define ABSTRACTDNDITEM_H

#include <QtDesigner/sdk_global.h>

QT_BEGIN_NAMESPACE

class DomUI;
class QWidget;
class QPoint;

class QDESIGNER_SDK_EXPORT QDesignerDnDItemInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerDnDItemInterface)

    enum DropType { MoveDrop, CopyDrop };

    QDesignerDnDItemInterface() = default;
    virtual ~QDesignerDnDItemInterface() = default;

    virtual DomUI *domUi() const = 0;
    virtual QWidget *widget() const = 0;
    virtual QWidget *decoration() const = 0;
    virtual QPoint hotSpot() const = 0;
    virtual DropType type() const = 0;
    virtual QWidget *source() const = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTDNDITEM_H
