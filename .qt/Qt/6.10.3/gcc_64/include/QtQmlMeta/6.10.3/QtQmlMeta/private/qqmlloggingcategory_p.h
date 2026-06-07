// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLOGGINGCATEGORY_P_H
#define QQMLLOGGINGCATEGORY_P_H

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

#include <private/qqmlloggingcategorybase_p.h>

#include <QtQmlMeta/qtqmlmetaexports.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlparserstatus.h>

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class Q_QMLMETA_EXPORT QQmlLoggingCategory : public QQmlLoggingCategoryBase, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(DefaultLogLevel defaultLogLevel READ defaultLogLevel WRITE setDefaultLogLevel REVISION(2, 12))
    QML_NAMED_ELEMENT(LoggingCategory)
    QML_ADDED_IN_VERSION(2, 8)

public:
    enum DefaultLogLevel {
        Debug = QtDebugMsg,
        Info = QtInfoMsg,
        Warning = QtWarningMsg,
        Critical = QtCriticalMsg,
        Fatal = QtFatalMsg
    };
    Q_ENUM(DefaultLogLevel);

    QQmlLoggingCategory(QObject *parent = nullptr);
    virtual ~QQmlLoggingCategory();

    DefaultLogLevel defaultLogLevel() const;
    void setDefaultLogLevel(DefaultLogLevel defaultLogLevel);
    QString name() const;
    void setName(const QString &name);

    void classBegin() override;
    void componentComplete() override;

    void forceCompletion() final;

private:
    QByteArray m_name;
    DefaultLogLevel m_defaultLogLevel = Debug;
    bool m_initialized;
};

QT_END_NAMESPACE

#endif // QQMLLOGGINGCATEGORY_H
