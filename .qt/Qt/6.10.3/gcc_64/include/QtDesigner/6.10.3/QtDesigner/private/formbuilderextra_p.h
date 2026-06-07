// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ABSTRACTFORMBUILDERPRIVATE_H
#define ABSTRACTFORMBUILDERPRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "uilib_global.h"

#include <QtCore/qhash.h>
#include <QtCore/qpointer.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qdir.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

class QDesignerCustomWidgetInterface;
class QObject;
class QVariant;
class QWidget;
class QObject;
class QLabel;
class QButtonGroup;
class QBoxLayout;
class QGridLayout;
class QAction;
class QActionGroup;

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

class DomBrush;
class DomButtonGroups;
class DomButtonGroup;
class DomColorGroup;
class DomCustomWidget;
class DomPalette;
class DomProperty;
class DomUI;

class QAbstractFormBuilder;
class QResourceBuilder;
class QTextBuilder;

class QDESIGNER_UILIB_EXPORT QFormBuilderExtra
{
public:
    Q_DISABLE_COPY_MOVE(QFormBuilderExtra);

    QFormBuilderExtra();
    ~QFormBuilderExtra();

    struct CustomWidgetData {
        CustomWidgetData();
        explicit CustomWidgetData(const DomCustomWidget *dc);

        QString addPageMethod;
        QString script;
        QString baseClass;
        bool isContainer = false;
    };

    void clear();

    DomUI *readUi(QIODevice *dev);
    static QString msgInvalidUiFile();

    bool applyPropertyInternally(QObject *o, const QString &propertyName, const QVariant &value);

    enum BuddyMode { BuddyApplyAll, BuddyApplyVisibleOnly };

    void applyInternalProperties() const;
    static bool applyBuddy(const QString &buddyName, BuddyMode applyMode, QLabel *label);

    const QPointer<QWidget> &parentWidget() const;
    bool parentWidgetIsSet() const;
    void setParentWidget(const QPointer<QWidget> &w);

    void setProcessingLayoutWidget(bool processing);
    bool processingLayoutWidget() const;

    void setResourceBuilder(QResourceBuilder *builder);
    QResourceBuilder *resourceBuilder() const;

    void setTextBuilder(QTextBuilder *builder);
    QTextBuilder *textBuilder() const;

    void storeCustomWidgetData(const QString &className, const DomCustomWidget *d);
    QString customWidgetAddPageMethod(const QString &className) const;
    QString customWidgetBaseClass(const QString &className) const;
    bool isCustomWidgetContainer(const QString &className) const;

    // --- Hash used in creating button groups on demand. Store a map of name and pair of dom group and real group
    void registerButtonGroups(const DomButtonGroups *groups);

    using ButtonGroupEntry = std::pair<DomButtonGroup *, QButtonGroup *>;
    using ButtonGroupHash = QHash<QString, ButtonGroupEntry>;
    const ButtonGroupHash &buttonGroups() const { return m_buttonGroups; }
    ButtonGroupHash &buttonGroups()  { return m_buttonGroups; }

    static void getLayoutMargins(const QList<DomProperty*> &properties,
                                 int *left, int *top, int *right, int *bottom);

    // return stretch as a comma-separated list
    static QString boxLayoutStretch(const QBoxLayout*);
    // apply stretch
    static bool setBoxLayoutStretch(const QString &, QBoxLayout*);
    static void clearBoxLayoutStretch(QBoxLayout*);

    static QString gridLayoutRowStretch(const QGridLayout *);
    static bool setGridLayoutRowStretch(const QString &, QGridLayout *);
    static void clearGridLayoutRowStretch(QGridLayout *);

    static QString gridLayoutColumnStretch(const QGridLayout *);
    static bool setGridLayoutColumnStretch(const QString &, QGridLayout *);
    static void clearGridLayoutColumnStretch(QGridLayout *);

    // return the row/column sizes as  comma-separated lists
    static QString gridLayoutRowMinimumHeight(const QGridLayout *);
    static bool setGridLayoutRowMinimumHeight(const QString &, QGridLayout *);
    static void clearGridLayoutRowMinimumHeight(QGridLayout *);

    static QString gridLayoutColumnMinimumWidth(const QGridLayout *);
    static bool setGridLayoutColumnMinimumWidth(const QString &, QGridLayout *);
    static void clearGridLayoutColumnMinimumWidth(QGridLayout *);

    static void setPixmapProperty(DomProperty *p, const std::pair<QString, QString> &ip);
    static QPalette loadPalette(const DomPalette *dom);
    static void setupColorGroup(QPalette *palette, QPalette::ColorGroup colorGroup,
                                const DomColorGroup *group);
    static DomColorGroup *saveColorGroup(const QPalette &palette,
                                         QPalette::ColorGroup colorGroup);
    static DomPalette *savePalette(const QPalette &palette);
    static QBrush setupBrush(const DomBrush *brush);
    static DomBrush *saveBrush(const QBrush &br);

    static DomProperty *propertyByName(const QList<DomProperty*> &properties,
                                       QAnyStringView needle);

    static bool isQFontComboBox(const QWidget *w);

    QStringList m_pluginPaths;
    QMap<QString, QDesignerCustomWidgetInterface*> m_customWidgets;

    QHash<QObject*, bool> m_laidout;
    QHash<QString, QAction*> m_actions;
    QHash<QString, QActionGroup*> m_actionGroups;
    bool m_fullyQualifiedEnums = true;
    // separate horizontal/vertical size constraints since Qt 7 (QTBUG-17730).
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    bool m_separateSizeConstraints = false;
#else
    bool m_separateSizeConstraints = true;
#endif
    int m_defaultMargin;
    int m_defaultSpacing;
    QDir m_workingDirectory;
    QString m_errorString;
    QString m_language;

private:
    void clearResourceBuilder();
    void clearTextBuilder();

    QHash<QLabel *, QString> m_buddies;

    QHash<QString, CustomWidgetData> m_customWidgetDataHash;

    ButtonGroupHash m_buttonGroups;

    bool m_layoutWidget = false;
    QResourceBuilder *m_resourceBuilder = nullptr;
    QTextBuilder *m_textBuilder = nullptr;

    QPointer<QWidget> m_parentWidget;
    bool m_parentWidgetIsSet = false;
};

void uiLibWarning(const QString &message);

// Struct with static accessor that provides most strings used in the form builder.
struct QDESIGNER_UILIB_EXPORT QFormBuilderStrings {
    QFormBuilderStrings();

    static const QFormBuilderStrings &instance();

    static constexpr auto titleAttribute = QLatin1StringView("title");
    static constexpr auto labelAttribute = QLatin1StringView("label");
    static constexpr auto toolTipAttribute = QLatin1StringView("toolTip");
    static constexpr auto whatsThisAttribute = QLatin1StringView("whatsThis");
    static constexpr auto flagsAttribute = QLatin1StringView("flags");
    static constexpr auto iconAttribute = QLatin1StringView("icon");
    static constexpr auto textAttribute = QLatin1StringView("text") ;

    using RoleNName = std::pair<Qt::ItemDataRole, QString>;
    QList<RoleNName> itemRoles;
    QHash<QString, Qt::ItemDataRole> treeItemRoleHash;

    // first.first is primary role, first.second is shadow role.
    // Shadow is used for either the translation source or the designer
    // representation of the string value.
    using TextRoleNName = std::pair<std::pair<Qt::ItemDataRole, Qt::ItemDataRole>, QString>;
    QList<TextRoleNName> itemTextRoles;
    QHash<QString, std::pair<Qt::ItemDataRole, Qt::ItemDataRole> > treeItemTextRoleHash;
};
#ifdef QFORMINTERNAL_NAMESPACE
}
#endif

QT_END_NAMESPACE

#endif // ABSTRACTFORMBUILDERPRIVATE_H
