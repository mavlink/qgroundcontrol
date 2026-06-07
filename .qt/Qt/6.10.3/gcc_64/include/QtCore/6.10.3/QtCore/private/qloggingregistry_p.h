// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOGGINGREGISTRY_P_H
#define QLOGGINGREGISTRY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qloggingcategory_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>

#include <map>

class tst_QLoggingRegistry;

QT_BEGIN_NAMESPACE

#define Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE_IMPL(name, env, categoryName) \
    const QLoggingCategory &name() \
    { \
        static constexpr char cname[] = categoryName;                               \
        static_assert(cname[0] == 'q' && cname[1] == 't' && cname[2] == '.'         \
                      && cname[4] != '\0', "Category name must start with 'qt.'");  \
        static const QLoggingCategoryWithEnvironmentOverride category(cname, env);  \
        return category;                                                            \
    }

#define Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(name, env, categoryName) \
    inline namespace QtPrivateLogging { \
    Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE_IMPL(name, env, categoryName) \
    } \
    Q_WEAK_OVERLOAD \
    Q_DECL_DEPRECATED_X("Logging categories should either be static or declared in a header") \
    const QLoggingCategory &name() { return QtPrivateLogging::name(); }

#define Q_STATIC_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(name, env, categoryName) \
    static Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE_IMPL(name, env, categoryName)

class Q_AUTOTEST_EXPORT QLoggingRule
{
public:
    QLoggingRule();
    QLoggingRule(QStringView pattern, bool enabled);
    int pass(QLatin1StringView categoryName, QtMsgType type) const;

    enum PatternFlag {
        FullText = 0x1,
        LeftFilter = 0x2,
        RightFilter = 0x4,
        MidFilter = LeftFilter | RightFilter
    };
    Q_DECLARE_FLAGS(PatternFlags, PatternFlag)

    QString category;
    int messageType = -1;
    PatternFlags flags;
    bool enabled = false;

private:
    void parse(QStringView pattern);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLoggingRule::PatternFlags)
Q_DECLARE_TYPEINFO(QLoggingRule, Q_RELOCATABLE_TYPE);

class Q_AUTOTEST_EXPORT QLoggingSettingsParser
{
public:
    void setImplicitRulesSection(bool inRulesSection) { m_inRulesSection = inRulesSection; }

    void setContent(QStringView content, char16_t separator = u'\n');
    void setContent(FILE *stream);

    QList<QLoggingRule> rules() const { return _rules; }

private:
    void parseNextLine(QStringView line);

private:
    bool m_inRulesSection = false;
    QList<QLoggingRule> _rules;
};

class QLoggingRegistry
{
    Q_DISABLE_COPY_MOVE(QLoggingRegistry)
public:
    QLoggingRegistry();

    Q_AUTOTEST_EXPORT void initializeRules();

    void registerCategory(QLoggingCategory *category, QtMsgType enableForLevel);
    void unregisterCategory(QLoggingCategory *category);

    Q_CORE_EXPORT void registerEnvironmentOverrideForCategory(const char *categoryName,
                                                              const char *environment);

    void setApiRules(const QString &content);

    QLoggingCategory::CategoryFilter
    installFilter(QLoggingCategory::CategoryFilter filter);

    Q_CORE_EXPORT static QLoggingRegistry *instance();

    static constexpr const char defaultCategoryName[] = "default";
    static QLoggingCategory *defaultCategory();

private:
    Q_AUTOTEST_EXPORT void updateRules();
    static inline QLoggingRegistry *self = nullptr;

    static void defaultCategoryFilter(QLoggingCategory *category);

    enum RuleSet {
        // sorted by order in which defaultCategoryFilter considers them:
        QtConfigRules,
        ConfigRules,
        ApiRules,
        EnvironmentRules,

        NumRuleSets
    };

    QMutex registryMutex;

    // protected by mutex:
    QList<QLoggingRule> ruleSets[NumRuleSets];
    QHash<QLoggingCategory *, QtMsgType> categories;
    QLoggingCategory::CategoryFilter categoryFilter;
    std::map<QByteArrayView, const char *> qtCategoryEnvironmentOverrides;

    friend class ::tst_QLoggingRegistry;
};

class QLoggingCategoryWithEnvironmentOverride : public QLoggingCategory
{
public:
    QLoggingCategoryWithEnvironmentOverride(const char *category, const char *env)
        : QLoggingCategory(registerOverride(category, env), QtInfoMsg)
    {}

private:
    static const char *registerOverride(const char *categoryName, const char *environment)
    {
        QLoggingRegistry *c = QLoggingRegistry::instance();
        if (c)
            c->registerEnvironmentOverrideForCategory(categoryName, environment);
        return categoryName;
    }
};

QT_END_NAMESPACE

#endif // QLOGGINGREGISTRY_P_H
