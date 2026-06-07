// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTWIDGETFACTORY_H
#define ABSTRACTWIDGETFACTORY_H

#include <QtDesigner/sdk_global.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QWidget;
class QLayout;

class QDESIGNER_SDK_EXPORT QDesignerWidgetFactoryInterface: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerWidgetFactoryInterface(QObject *parent = nullptr);
    virtual ~QDesignerWidgetFactoryInterface();

    virtual QDesignerFormEditorInterface *core() const = 0;

    virtual QWidget* containerOfWidget(QWidget *w) const = 0;
    virtual QWidget* widgetOfContainer(QWidget *w) const = 0;

    virtual QWidget *createWidget(const QString &name, QWidget *parentWidget = nullptr) const = 0;
    virtual QLayout *createLayout(QWidget *widget, QLayout *layout, int type) const = 0;

    virtual bool isPassiveInteractor(QWidget *widget) = 0;
    virtual void initialize(QObject *object) const = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTWIDGETFACTORY_H
