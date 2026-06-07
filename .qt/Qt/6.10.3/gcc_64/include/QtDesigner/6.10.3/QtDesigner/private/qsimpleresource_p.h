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

#ifndef QSIMPLERESOURCE_H
#define QSIMPLERESOURCE_H

#include "abstractformbuilder.h"
#include "shared_global_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class DomCustomWidgets;
class DomCustomWidget;
class DomSlots;

class QDesignerFormEditorInterface;

namespace qdesigner_internal {

class WidgetDataBaseItem;

class QDESIGNER_SHARED_EXPORT QSimpleResource : public QAbstractFormBuilder
{
public:
    explicit QSimpleResource(QDesignerFormEditorInterface *core);
    ~QSimpleResource() override;

    QBrush setupBrush(DomBrush *brush);
    DomBrush *saveBrush(const QBrush &brush);

    inline QDesignerFormEditorInterface *core() const
    { return m_core; }

    // Query extensions for additional data
    static void addExtensionDataToDOM(QAbstractFormBuilder *afb,
                                      QDesignerFormEditorInterface *core,
                                      DomWidget *ui_widget, QWidget *widget);
    static void applyExtensionDataFromDOM(QAbstractFormBuilder *afb,
                                          QDesignerFormEditorInterface *core,
                                          DomWidget *ui_widget, QWidget *widget);
    // Return the script returned by the CustomWidget codeTemplate API
    static QString customWidgetScript(QDesignerFormEditorInterface *core, QObject *object);
    static QString customWidgetScript(QDesignerFormEditorInterface *core, const QString &className);
    static bool hasCustomWidgetScript(QDesignerFormEditorInterface *core, QObject *object);

    // Implementation for FormBuilder::createDomCustomWidgets() that adds
    // the custom widgets to the widget database
    static void handleDomCustomWidgets(const QDesignerFormEditorInterface *core,
                                       const DomCustomWidgets *dom_custom_widgets);

protected:
    static bool addFakeMethods(const DomSlots *domSlots, QStringList &fakeSlots, QStringList &fakeSignals);

private:
    static void addCustomWidgetsToWidgetDatabase(const QDesignerFormEditorInterface *core,
                                                 QList<DomCustomWidget *> &custom_widget_list);
    static void addFakeMethodsToWidgetDataBase(const DomCustomWidget *domCustomWidget, WidgetDataBaseItem *item);

    static bool m_warningsEnabled;
    QDesignerFormEditorInterface *m_core;
};

// Contents of clipboard for formbuilder copy and paste operations
// (Actions and widgets)
struct QDESIGNER_SHARED_EXPORT FormBuilderClipboard {
    using ActionList = QList<QAction *>;

    FormBuilderClipboard() = default;
    FormBuilderClipboard(QWidget *w);

    bool empty() const;

    QWidgetList m_widgets;
    ActionList m_actions;
};

// Base class for a form builder used in the editor that
// provides copy and paste.(move into base interface)
class QDESIGNER_SHARED_EXPORT QEditorFormBuilder : public QSimpleResource
{
public:
    explicit QEditorFormBuilder(QDesignerFormEditorInterface *core) : QSimpleResource(core) {}

    virtual bool copy(QIODevice *dev, const FormBuilderClipboard &selection) = 0;
    virtual DomUI *copy(const FormBuilderClipboard &selection) = 0;

    // A widget parent needs to be specified, otherwise, the widget factory cannot locate the form window via parent
    // and thus is not able to construct special widgets (QLayoutWidget).
    virtual FormBuilderClipboard paste(DomUI *ui, QWidget *widgetParent, QObject *actionParent = nullptr) = 0;
    virtual FormBuilderClipboard paste(QIODevice *dev, QWidget *widgetParent, QObject *actionParent = nullptr) = 0;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif
