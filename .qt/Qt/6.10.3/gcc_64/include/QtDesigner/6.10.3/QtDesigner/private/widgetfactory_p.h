


// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//


#ifndef WIDGETFACTORY_H
#define WIDGETFACTORY_H

#include "shared_global_p.h"
#include "pluginmanager_p.h"

#include <QtDesigner/abstractwidgetfactory.h>

#include <QtCore/qmap.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QObject;
class QWidget;
class QLayout;
class QDesignerFormEditorInterface;
class QDesignerCustomWidgetInterface;
class QDesignerFormWindowInterface;
class QStyle;

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT WidgetFactory: public QDesignerWidgetFactoryInterface
{
    Q_OBJECT
public:
    explicit WidgetFactory(QDesignerFormEditorInterface *core, QObject *parent = nullptr);
    ~WidgetFactory();

    QWidget* containerOfWidget(QWidget *widget) const override;
    QWidget* widgetOfContainer(QWidget *widget) const override;

    QObject* createObject(const QString &className, QObject* parent) const;

    QWidget *createWidget(const QString &className, QWidget *parentWidget) const override;
    QLayout *createLayout(QWidget *widget, QLayout *layout, int type) const override;

    bool isPassiveInteractor(QWidget *widget) override;
    void initialize(QObject *object) const override;
    void initializeCommon(QWidget *object) const;
    void initializePreview(QWidget *object) const;


    QDesignerFormEditorInterface *core() const override;

    static QString classNameOf(QDesignerFormEditorInterface *core, const QObject* o);

    QDesignerFormWindowInterface *currentFormWindow(QDesignerFormWindowInterface *fw);

    static QLayout *createUnmanagedLayout(QWidget *parentWidget, int type);

    // The widget factory maintains a cache of styles which it owns.
    QString styleName() const;
    void setStyleName(const QString &styleName);

    /* Return a cached style matching the name or QApplication's style if
     * it is the default. */
    QStyle *getStyle(const QString &styleName);
    // Return the current style used by the factory. This either a cached one
    // or QApplication's style */
    QStyle *style() const;

    // Apply one of the cached styles or QApplication's style to a toplevel widget.
    void applyStyleTopLevel(const QString &styleName, QWidget *w);
    static void applyStyleToTopLevel(QStyle *style, QWidget *widget);

    // Return whether object was created by the factory for the form editor.
    static bool isFormEditorObject(const QObject *o);

    // Boolean dynamic property to set on widgets to prevent custom
    // styles from interfering
    static const char *disableStyleCustomPaintingPropertyC;

public slots:
    void loadPlugins();
    void activeFormWindowChanged(QDesignerFormWindowInterface *formWindow);
    void formWindowAdded(QDesignerFormWindowInterface *formWindow);

private:
    QWidget* createCustomWidget(const QString &className, QWidget *parentWidget, bool *creationError) const;
    QDesignerFormWindowInterface *findFormWindow(QWidget *parentWidget) const;
    void setFormWindowStyle(QDesignerFormWindowInterface *formWindow);

    QDesignerFormEditorInterface *m_core;
    QMap<QString, QDesignerCustomWidgetInterface *> m_customFactory;
    QDesignerFormWindowInterface *m_formWindow;

    // Points to the cached style or 0 if the default (qApp) is active
    QStyle *m_currentStyle;
    QHash<QString, QStyle *> m_styleCache;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // WIDGETFACTORY_H
