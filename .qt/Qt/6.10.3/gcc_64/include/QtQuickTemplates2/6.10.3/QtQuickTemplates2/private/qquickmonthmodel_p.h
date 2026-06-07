// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMONTHMODEL_P_H
#define QQUICKMONTHMODEL_P_H

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
#include <QtCore/qlocale.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickMonthModelPrivate;

class QQuickMonthModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(int count READ rowCount CONSTANT FINAL)

public:
    explicit QQuickMonthModel(QObject *parent = nullptr);

    int month() const;
    void setMonth(int month);

    int year() const;
    void setYear(int year);

    QLocale locale() const;
    void setLocale(const QLocale &locale);

    QString title() const;
    void setTitle(const QString &title);

    Q_INVOKABLE QDateTime dateAt(int index) const;
    Q_INVOKABLE int indexOf(QDateTime date) const;

    enum {
        DateRole = Qt::UserRole + 1,
        DayRole,
        TodayRole,
        WeekNumberRole,
        MonthRole,
        YearRole
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

Q_SIGNALS:
    void monthChanged();
    void yearChanged();
    void localeChanged();
    void titleChanged();

private:
    Q_DISABLE_COPY(QQuickMonthModel)
    Q_DECLARE_PRIVATE(QQuickMonthModel)
};

QT_END_NAMESPACE

#endif // QQUICKMONTHMODEL_P_H
