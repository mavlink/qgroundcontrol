// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDELEGATECOMPONENT_P_H
#define QQMLDELEGATECOMPONENT_P_H

#include <private/qtqmlmodelsglobal_p.h>

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

#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQmlModels/private/qqmlabstractdelegatecomponent_p.h>
#include <QtQml/qqmlcomponent.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

class Q_QMLMODELS_EXPORT QQmlDelegateChoice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant roleValue READ roleValue WRITE setRoleValue NOTIFY roleValueChanged FINAL)
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged FINAL)
    Q_PROPERTY(int index READ row WRITE setRow NOTIFY indexChanged FINAL)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "delegate")
    QML_NAMED_ELEMENT(DelegateChoice)
    QML_ADDED_IN_VERSION(6, 9)

public:
    QVariant roleValue() const;
    void setRoleValue(const QVariant &roleValue);

    int row() const;
    void setRow(int r);

    int column() const;
    void setColumn(int c);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    virtual bool match(int row, int column, const QVariant &value) const;

Q_SIGNALS:
    void roleValueChanged();
    void rowChanged();
    void indexChanged();
    void columnChanged();
    void delegateChanged();
    void changed();

private:
    QVariant m_value;
    int m_row = -1;
    int m_column = -1;
    QQmlComponent *m_delegate = nullptr;
};

class Q_QMLMODELS_EXPORT QQmlDelegateChooser : public QQmlAbstractDelegateComponent
{
    Q_OBJECT
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQmlDelegateChoice> choices READ choices CONSTANT FINAL)
    Q_CLASSINFO("DefaultProperty", "choices")
    QML_NAMED_ELEMENT(DelegateChooser)
    QML_ADDED_IN_VERSION(6, 9)

public:
    QString role() const final { return m_role; }
    void setRole(const QString &role);

    virtual QQmlListProperty<QQmlDelegateChoice> choices();
    static void choices_append(QQmlListProperty<QQmlDelegateChoice> *, QQmlDelegateChoice *);
    static qsizetype choices_count(QQmlListProperty<QQmlDelegateChoice> *);
    static QQmlDelegateChoice *choices_at(QQmlListProperty<QQmlDelegateChoice> *, qsizetype);
    static void choices_clear(QQmlListProperty<QQmlDelegateChoice> *);
    static void choices_replace(QQmlListProperty<QQmlDelegateChoice> *, qsizetype,
                                QQmlDelegateChoice *);
    static void choices_removeLast(QQmlListProperty<QQmlDelegateChoice> *);

    QQmlComponent *delegate(QQmlAdaptorModel *adaptorModel, int row, int column = -1) const override;

Q_SIGNALS:
    void roleChanged();

private:
    QString m_role;
    QList<QQmlDelegateChoice *> m_choices;
};

QT_END_NAMESPACE

#endif // QQMLDELEGATECOMPONENT_P_H
