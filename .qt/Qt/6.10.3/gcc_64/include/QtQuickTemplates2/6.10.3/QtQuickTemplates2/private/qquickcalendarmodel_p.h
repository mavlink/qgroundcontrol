// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCALENDARMODEL_P_H
#define QQUICKCALENDARMODEL_P_H

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

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qdatetime.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickCalendarModelPrivate;

class QQuickCalendarModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QDate from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(QDate to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    QML_NAMED_ELEMENT(CalendarModel)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickCalendarModel(QObject *parent = nullptr);

    QDate from() const;
    void setFrom(QDate from);

    QDate to() const;
    void setTo(QDate to);

    Q_INVOKABLE int monthAt(int index) const;
    Q_INVOKABLE int yearAt(int index) const;
    Q_INVOKABLE int indexOf(QDate date) const;
    Q_INVOKABLE int indexOf(int year, int month) const;

    enum {
        MonthRole,
        YearRole
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void countChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQuickCalendarModel)
    Q_DECLARE_PRIVATE(QQuickCalendarModel)
};

QT_END_NAMESPACE

#endif // QQUICKCALENDARMODEL_P_H
