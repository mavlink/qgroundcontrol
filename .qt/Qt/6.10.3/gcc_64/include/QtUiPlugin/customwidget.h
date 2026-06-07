// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef CUSTOMWIDGET_H
#define CUSTOMWIDGET_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtGui/qicon.h>

#if 0
#pragma qt_deprecates(QtDesigner/customwidget.h)
#pragma qt_deprecates(QtDesigner/QDesignerCustomWidgetInterface)
#pragma qt_deprecates(QtDesigner/QDesignerCustomWidgetCollectionInterface)
#endif

QT_BEGIN_NAMESPACE

class QWidget;
class QDesignerFormEditorInterface;

class QDesignerCustomWidgetInterface
{
public:
    virtual ~QDesignerCustomWidgetInterface() = default; // ### FIXME: weak vtable

    virtual QString name() const = 0;
    virtual QString group() const = 0;
    virtual QString toolTip() const = 0;
    virtual QString whatsThis() const = 0;
    virtual QString includeFile() const = 0;
    virtual QIcon icon() const = 0;

    virtual bool isContainer() const = 0;

    virtual QWidget *createWidget(QWidget *parent) = 0;

    virtual bool isInitialized() const { return false; }
    virtual void initialize(QDesignerFormEditorInterface *core) { Q_UNUSED(core); }

    virtual QString domXml() const
    {
        return QLatin1StringView("<widget class=\"%1\" name=\"%2\"/>")
            .arg(name(), name().toLower());
    }

    virtual QString codeTemplate() const { return QString(); }
};

#define QDesignerCustomWidgetInterface_iid "org.qt-project.QDesignerCustomWidgetInterface"

Q_DECLARE_INTERFACE(QDesignerCustomWidgetInterface, QDesignerCustomWidgetInterface_iid)

class QDesignerCustomWidgetCollectionInterface
{
public:
    virtual ~QDesignerCustomWidgetCollectionInterface() = default; // ### FIXME: weak vtable

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const = 0;
};

#define QDesignerCustomWidgetCollectionInterface_iid "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface"

Q_DECLARE_INTERFACE(QDesignerCustomWidgetCollectionInterface, QDesignerCustomWidgetCollectionInterface_iid)

QT_END_NAMESPACE

#endif // CUSTOMWIDGET_H
