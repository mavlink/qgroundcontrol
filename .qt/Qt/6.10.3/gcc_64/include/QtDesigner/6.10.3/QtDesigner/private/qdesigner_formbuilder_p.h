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

#ifndef QDESIGNER_FORMBUILDER_H
#define QDESIGNER_FORMBUILDER_H

#include "shared_global_p.h"
#include "deviceprofile_p.h"

#include <QtDesigner/formbuilder.h>

#include <QtCore/qmap.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

class QPixmap;
class QtResourceSet;

namespace qdesigner_internal {

class DesignerPixmapCache;
class DesignerIconCache;

/* Form builder used for previewing forms and widget box.
 * It applies the system settings to its toplevel window. */

class QDESIGNER_SHARED_EXPORT QDesignerFormBuilder: public QFormBuilder
{
public:
    QDesignerFormBuilder(QDesignerFormEditorInterface *core,
                         const DeviceProfile &deviceProfile = DeviceProfile());

    virtual QWidget *createWidget(DomWidget *ui_widget, QWidget *parentWidget = nullptr)
    { return QFormBuilder::create(ui_widget, parentWidget); }

    inline QDesignerFormEditorInterface *core() const
    { return m_core; }

    QString systemStyle() const;

    // Create a preview widget (for integrations) or return 0. The widget has to be embedded into a main window.
    // Experimental, depending on script support.
    static QWidget *createPreview(const QDesignerFormWindowInterface *fw, const QString &styleName /* ="" */,
                                  const QString &appStyleSheet  /* ="" */,
                                  const DeviceProfile &deviceProfile,
                                  QString *errorMessage);
    // Convenience that pops up message boxes in case of failures.
    static QWidget *createPreview(const QDesignerFormWindowInterface *fw, const QString &styleName = QString());
    //  Create a preview widget (for integrations) or return 0. The widget has to be embedded into a main window.
    static QWidget *createPreview(const QDesignerFormWindowInterface *fw, const QString &styleName, const QString &appStyleSheet, QString *errorMessage);
    // Convenience that pops up message boxes in case of failures.
    static QWidget *createPreview(const QDesignerFormWindowInterface *fw, const QString &styleName, const QString &appStyleSheet);

    // Create a preview image
    static QPixmap createPreviewPixmap(const QDesignerFormWindowInterface *fw, const QString &styleName = QString(), const QString &appStyleSheet = QString());

protected:
    using QFormBuilder::createDom;
    using QFormBuilder::create;

    QWidget *create(DomUI *ui, QWidget *parentWidget) override;
    DomWidget *createDom(QWidget *widget, DomWidget *ui_parentWidget, bool recursive = true) override;
    QWidget *create(DomWidget *ui_widget, QWidget *parentWidget) override;
    QLayout *create(DomLayout *ui_layout, QLayout *layout, QWidget *parentWidget) override;
    void createResources(DomResources *resources) override;

    QWidget *createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name) override;
    bool addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget) override;
    bool addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout) override;

    virtual QIcon nameToIcon(const QString &filePath, const QString &qrcPath);
    virtual QPixmap nameToPixmap(const QString &filePath, const QString &qrcPath);

    void applyProperties(QObject *o, const QList<DomProperty*> &properties) override;

    void loadExtraInfo(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget) override;

    QtResourceSet *internalResourceSet() const { return m_tempResourceSet; }

    DeviceProfile deviceProfile() const { return m_deviceProfile; }

private:
    QDesignerFormEditorInterface *m_core;

    using WidgetSet = QSet<QWidget *>;
    WidgetSet m_customWidgetsWithScript;

    const DeviceProfile m_deviceProfile;

    DesignerPixmapCache *m_pixmapCache;
    DesignerIconCache *m_iconCache;
    bool m_ignoreCreateResources;
    QtResourceSet *m_tempResourceSet;
    bool m_mainWidget;
};

// Form builder for a new form widget (preview). To allow for promoted
// widgets in the template, it implements the handling of custom widgets
// (adding of them to the widget database).

class QDESIGNER_SHARED_EXPORT NewFormWidgetFormBuilder: public QDesignerFormBuilder {
public:
    NewFormWidgetFormBuilder(QDesignerFormEditorInterface *core,
                             const DeviceProfile &deviceProfile = DeviceProfile());

protected:
    void createCustomWidgets(DomCustomWidgets *) override;
};


} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_FORMBUILDER_H
