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


#ifndef ICONSELECTOR_H
#define ICONSELECTOR_H

#include "shared_global_p.h"

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qdialog.h>

#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QComboBox;

class QtResourceModel;
class QDesignerFormEditorInterface;
class QDesignerDialogGuiInterface;
class QDesignerResourceBrowserInterface;

namespace qdesigner_internal {

class DesignerIconCache;
class DesignerPixmapCache;
class PropertySheetIconValue;
struct IconThemeEditorPrivate;

// Resource Dialog that embeds the language-dependent resource widget as returned by the language extension
class QDESIGNER_SHARED_EXPORT LanguageResourceDialog : public QDialog
{
    Q_OBJECT

    explicit LanguageResourceDialog(QDesignerResourceBrowserInterface *rb, QWidget *parent = nullptr);

public:
    ~LanguageResourceDialog() override;
    // Factory: Returns 0 if the language extension does not provide a resource browser.
    static LanguageResourceDialog* create(QDesignerFormEditorInterface *core, QWidget *parent);

    void setCurrentPath(const QString &filePath);
    QString currentPath() const;

private:
    QScopedPointer<class LanguageResourceDialogPrivate> d_ptr;
    Q_DECLARE_PRIVATE(LanguageResourceDialog)
    Q_DISABLE_COPY_MOVE(LanguageResourceDialog)

};

class QDESIGNER_SHARED_EXPORT IconSelector: public QWidget
{
    Q_OBJECT
public:
    IconSelector(QWidget *parent = nullptr);
    ~IconSelector() override;

    void setFormEditor(QDesignerFormEditorInterface *core); // required for dialog gui.
    void setIconCache(DesignerIconCache *iconCache);
    void setPixmapCache(DesignerPixmapCache *pixmapCache);

    void setIcon(const PropertySheetIconValue &icon);
    PropertySheetIconValue icon() const;

    // Check whether a pixmap may be read
    enum CheckMode { CheckFast, CheckFully };
    static bool checkPixmap(const QString &fileName, CheckMode cm = CheckFully, QString *errorMessage = nullptr);
    // Choose a pixmap from file
    static QString choosePixmapFile(const QString &directory, QDesignerDialogGuiInterface *dlgGui, QWidget *parent);
    // Choose a pixmap from resource; use language-dependent resource browser if present
    static QString choosePixmapResource(QDesignerFormEditorInterface *core, QtResourceModel *resourceModel, const QString &oldPath, QWidget *parent);

signals:
    void iconChanged(const PropertySheetIconValue &icon);
private:
    QScopedPointer<class IconSelectorPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IconSelector)
    Q_DISABLE_COPY_MOVE(IconSelector)
};

// IconThemeEditor: Let's the user input theme icon names and shows a preview label.
class QDESIGNER_SHARED_EXPORT IconThemeEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme DESIGNABLE true)
public:
    explicit IconThemeEditor(QWidget *parent = nullptr, bool wantResetButton = true);
    ~IconThemeEditor() override;

    QString theme() const;
    void setTheme(const QString &theme);

signals:
    void edited(const QString &);

public slots:
    void reset();

private:
    QScopedPointer<IconThemeEditorPrivate> d;
};

// IconThemeEnumEditor: Let's the user input theme icon enum values
// (QIcon::ThemeIcon) and shows a preview label. -1 means nothing selected.
class QDESIGNER_SHARED_EXPORT IconThemeEnumEditor : public QWidget
{
    Q_OBJECT
public:
    explicit IconThemeEnumEditor(QWidget *parent = nullptr, bool wantResetButton = true);
    ~IconThemeEnumEditor() override;

    int themeEnum() const;
    void setThemeEnum(int);

    static QString iconName(int e);
    static QComboBox *createComboBox(QWidget *parent = nullptr);

signals:
    void edited(int);

public slots:
    void reset();

private:
    QScopedPointer<IconThemeEditorPrivate> d;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // ICONSELECTOR_H

