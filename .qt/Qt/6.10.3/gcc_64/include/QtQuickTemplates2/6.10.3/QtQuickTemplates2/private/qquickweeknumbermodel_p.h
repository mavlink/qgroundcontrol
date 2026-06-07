// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKWEEKNUMBERMODEL_P_H
#define QQUICKWEEKNUMBERMODEL_P_H

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
#include <QtCore/qlocale.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickWeekNumberModelPrivate;

class QQuickWeekNumberModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(int count READ rowCount CONSTANT FINAL)

public:
    explicit QQuickWeekNumberModel(QObject *parent = nullptr);

    int month() const;
    void setMonth(int month);

    int year() const;
    void setYear(int year);

    QLocale locale() const;
    void setLocale(const QLocale &locale);

    Q_INVOKABLE int weekNumberAt(int index) const;
    Q_INVOKABLE int indexOf(int weekNumber) const;

    enum {
        WeekNumberRole = Qt::UserRole + 1
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

Q_SIGNALS:
    void monthChanged();
    void yearChanged();
    void localeChanged();

private:
    Q_DISABLE_COPY(QQuickWeekNumberModel)
    Q_DECLARE_PRIVATE(QQuickWeekNumberModel)
};

QT_END_NAMESPACE

#endif // QQUICKWEEKNUMBERMODEL_P_H
