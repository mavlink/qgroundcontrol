// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUILOADER_H
#define QUILOADER_H

#include <QtUiTools/qtuitoolsglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QLayout;
class QAction;
class QActionGroup;
class QString;
class QIODevice;
class QDir;

class QUiLoaderPrivate;
class Q_UITOOLS_EXPORT QUiLoader : public QObject
{
    Q_OBJECT
public:
    explicit QUiLoader(QObject *parent = nullptr);
    ~QUiLoader() override;

    QStringList pluginPaths() const;
    void clearPluginPaths();
    void addPluginPath(const QString &path);

    QWidget *load(QIODevice *device, QWidget *parentWidget = nullptr);
    QStringList availableWidgets() const;
    QStringList availableLayouts() const;

    virtual QWidget *createWidget(const QString &className, QWidget *parent = nullptr, const QString &name = QString());
    virtual QLayout *createLayout(const QString &className, QObject *parent = nullptr, const QString &name = QString());
    virtual QActionGroup *createActionGroup(QObject *parent = nullptr, const QString &name = QString());
    virtual QAction *createAction(QObject *parent = nullptr, const QString &name = QString());

    void setWorkingDirectory(const QDir &dir);
    QDir workingDirectory() const;

    void setLanguageChangeEnabled(bool enabled);
    bool isLanguageChangeEnabled() const;

    void setTranslationEnabled(bool enabled);
    bool isTranslationEnabled() const;

    QString errorString() const;

private:
    QScopedPointer<QUiLoaderPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QUiLoader)
    Q_DISABLE_COPY_MOVE(QUiLoader)
};

QT_END_NAMESPACE

#endif // QUILOADER_H
