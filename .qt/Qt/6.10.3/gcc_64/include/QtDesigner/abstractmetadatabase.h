// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTMETADATABASE_H
#define ABSTRACTMETADATABASE_H

#include <QtDesigner/sdk_global.h>

#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QCursor;
class QWidget;

class QDesignerFormEditorInterface;

class QDesignerMetaDataBaseItemInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerMetaDataBaseItemInterface)

    QDesignerMetaDataBaseItemInterface() = default;
    virtual ~QDesignerMetaDataBaseItemInterface() = default;

    virtual QString name() const = 0;
    virtual void setName(const QString &name) = 0;

    virtual QList<QWidget*> tabOrder() const = 0;
    virtual void setTabOrder(const QList<QWidget*> &tabOrder) = 0;

    virtual bool enabled() const = 0;
    virtual void setEnabled(bool b) = 0;
};


class QDESIGNER_SDK_EXPORT QDesignerMetaDataBaseInterface: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerMetaDataBaseInterface(QObject *parent = nullptr);
    virtual ~QDesignerMetaDataBaseInterface();

    virtual QDesignerMetaDataBaseItemInterface *item(QObject *object) const = 0;
    virtual void add(QObject *object) = 0;
    virtual void remove(QObject *object) = 0;

    virtual QList<QObject*> objects() const = 0;

    virtual QDesignerFormEditorInterface *core() const = 0;

Q_SIGNALS:
    void changed();
};

QT_END_NAMESPACE

#endif // ABSTRACTMETADATABASE_H
