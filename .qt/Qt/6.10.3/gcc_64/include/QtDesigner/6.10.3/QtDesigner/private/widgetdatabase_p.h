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


#ifndef WIDGETDATABASE_H
#define WIDGETDATABASE_H

#include "shared_global_p.h"

#include <QtDesigner/abstractwidgetdatabase.h>

#include <QtGui/qicon.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/qpair.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QObject;
class QDesignerCustomWidgetInterface;

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT WidgetDataBaseItem: public QDesignerWidgetDataBaseItemInterface
{
public:
    explicit WidgetDataBaseItem(const QString &name = QString(),
                                const QString &group = QString());

    QString name() const override;
    void setName(const QString &name) override;

    QString group() const override;
    void setGroup(const QString &group) override;

    QString toolTip() const override;
    void setToolTip(const QString &toolTip) override;

    QString whatsThis() const override;
    void setWhatsThis(const QString &whatsThis) override;

    QString includeFile() const override;
    void setIncludeFile(const QString &includeFile) override;


    QIcon icon() const override;
    void setIcon(const QIcon &icon) override;

    bool isCompat() const override;
    void setCompat(bool compat) override;

    bool isContainer() const override;
    void setContainer(bool b) override;

    bool isCustom() const override;
    void setCustom(bool b) override;

    QString pluginPath() const override;
    void setPluginPath(const QString &path) override;

    bool isPromoted() const override;
    void setPromoted(bool b) override;

    QString extends() const override;
    void setExtends(const QString &s) override;

    void setDefaultPropertyValues(const QList<QVariant> &list) override;
    QList<QVariant> defaultPropertyValues() const override;

    static WidgetDataBaseItem *clone(const QDesignerWidgetDataBaseItemInterface *item);

    QString baseClassName() const; // FIXME Qt 7: Move to QDesignerWidgetDataBaseItemInterface
    void setBaseClassName(const QString &b);

    QStringList fakeSlots() const;
    void setFakeSlots(const QStringList &);

    QStringList fakeSignals() const;
    void setFakeSignals(const QStringList &);

    QString addPageMethod() const;
    void setAddPageMethod(const QString &m);

private:
    QString m_name;
    QString m_baseClassName;
    QString m_group;
    QString m_toolTip;
    QString m_whatsThis;
    QString m_includeFile;
    QString m_pluginPath;
    QString m_extends;
    QString m_addPageMethod;
    QIcon m_icon;
    uint m_compat: 1;
    uint m_container: 1;
    uint m_custom: 1;
    uint m_promoted: 1;
    QList<QVariant> m_defaultPropertyValues;
    QStringList m_fakeSlots;
    QStringList m_fakeSignals;
};

enum IncludeType { IncludeLocal, IncludeGlobal  };

using IncludeSpecification = std::pair<QString, IncludeType>;

QDESIGNER_SHARED_EXPORT IncludeSpecification  includeSpecification(QString includeFile);
QDESIGNER_SHARED_EXPORT QString buildIncludeFile(QString includeFile, IncludeType includeType);

class QDESIGNER_SHARED_EXPORT WidgetDataBase: public QDesignerWidgetDataBaseInterface
{
    Q_OBJECT
public:
    explicit WidgetDataBase(QDesignerFormEditorInterface *core, QObject *parent = nullptr);
    ~WidgetDataBase() override;

    QDesignerFormEditorInterface *core() const override;

    int indexOfObject(QObject *o, bool resolveName = true) const override;

    void remove(int index);


    void grabDefaultPropertyValues();
    void grabStandardWidgetBoxIcons();

    // Helpers for 'New Form' wizards in integrations. Obtain a list of suitable classes and generate XML for them.
    static QStringList formWidgetClasses(const QDesignerFormEditorInterface *core);
    static QStringList customFormWidgetClasses(const QDesignerFormEditorInterface *core);
    static QString formTemplate(const QDesignerFormEditorInterface *core, const QString &className, const QString &objectName);

    // Helpers for 'New Form' wizards: Set a fixed size on a XML form template
    static QString scaleFormTemplate(const QString &xml, const QSize &size, bool fixed);

public slots:
    void loadPlugins();

private:
    QList<QVariant> defaultPropertyValues(const QString &name);

    QDesignerFormEditorInterface *m_core;
};

QDESIGNER_SHARED_EXPORT QDesignerWidgetDataBaseItemInterface
        *appendDerived(QDesignerWidgetDataBaseInterface *db,
                       const QString &className,
                       const QString &group,
                       const QString &baseClassName,
                       const QString &includeFile,
                       bool promoted,
                       bool custom);

using WidgetDataBaseItemList = QList<QDesignerWidgetDataBaseItemInterface *>;

QDESIGNER_SHARED_EXPORT WidgetDataBaseItemList
        promotionCandidates(const QDesignerWidgetDataBaseInterface *db,
                            const QString &baseClassName);
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // WIDGETDATABASE_H
