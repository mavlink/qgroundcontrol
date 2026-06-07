// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CONTAINER_H
#define CONTAINER_H

#include <QtDesigner/extension.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QWidget;

class QDesignerContainerExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerContainerExtension)

    QDesignerContainerExtension() = default;
    virtual ~QDesignerContainerExtension() = default;

    virtual int count() const = 0;
    virtual QWidget *widget(int index) const = 0;

    virtual int currentIndex() const = 0;
    virtual void setCurrentIndex(int index) = 0;

    virtual bool canAddWidget() const = 0;
    virtual void addWidget(QWidget *widget) = 0;
    virtual void insertWidget(int index, QWidget *widget) = 0;
    virtual bool canRemove(int index) const = 0;
    virtual void remove(int index) = 0;
};

Q_DECLARE_EXTENSION_INTERFACE(QDesignerContainerExtension, "org.qt-project.Qt.Designer.Container")

QT_END_NAMESPACE

#endif // CONTAINER_H
