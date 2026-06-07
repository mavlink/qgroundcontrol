// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSYNCHRONIZER_P_H
#define QQMLSYNCHRONIZER_P_H
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

#include <QtQml/private/qqmlfinalizer_p.h>
#include <QtQml/qqmlpropertyvaluesource.h>
#include <QtQmlMeta/qtqmlmetaexports.h>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QtCore/qobject.h>

#include <QtLabsSynchronizer/qtlabssynchronizerexports.h>

QT_BEGIN_NAMESPACE

class QQmlSynchronizerPrivate;
class Q_LABSSYNCHRONIZER_EXPORT QQmlSynchronizer
    : public QObject, public QQmlPropertyValueSource, public QQmlFinalizerHook
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlSynchronizer)
    Q_INTERFACES(QQmlPropertyValueSource QQmlFinalizerHook)
    Q_PROPERTY(QObject *sourceObject READ sourceObject WRITE setSourceObject
                       NOTIFY sourceObjectChanged FINAL)
    Q_PROPERTY(QString sourceProperty READ sourceProperty WRITE setSourceProperty
                       NOTIFY sourcePropertyChanged FINAL)
    Q_PROPERTY(QObject *targetObject READ targetObject WRITE setTargetObject
                       NOTIFY targetObjectChanged FINAL)
    Q_PROPERTY(QString targetProperty READ targetProperty WRITE setTargetProperty
                       NOTIFY targetPropertyChanged FINAL)
    QML_NAMED_ELEMENT(Synchronizer)
    QML_ADDED_IN_VERSION(6, 10)
public:
    QQmlSynchronizer(QObject *parent = nullptr);

    QObject *sourceObject() const;
    void setSourceObject(QObject *object);

    QString sourceProperty() const;
    void setSourceProperty(const QString &property);

    QObject *targetObject() const;
    void setTargetObject(QObject *object);

    QString targetProperty() const;
    void setTargetProperty(const QString &property);

Q_SIGNALS:
    void sourceObjectChanged();
    void sourcePropertyChanged();
    void targetObjectChanged();
    void targetPropertyChanged();

    void valueBounced(QObject *object, const QString &property);
    void valueIgnored(QObject *object, const QString &property);

protected:
    void setTarget(const QQmlProperty &target) final;
    void componentFinalized() final;
};

QT_END_NAMESPACE

#endif // QQMLSYNCHRONIZER_P_H
