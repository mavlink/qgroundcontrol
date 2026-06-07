// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPTIONSWIDGET_H
#define QOPTIONSWIDGET_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the help generator tools. This header file may change from version
// to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qhash.h>
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QListWidget;
class QListWidgetItem;

class QOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    QOptionsWidget(QWidget *parent = nullptr);

    void clear() { setOptions({}, {}); }
    void setOptions(const QStringList &validOptions, const QStringList &selectedOptions);
    QStringList validOptions() const { return m_validOptions; }
    QStringList selectedOptions() const { return m_selectedOptions; }

    void setNoOptionText(const QString &text);
    void setInvalidOptionText(const QString &text);

signals:
    void optionSelectionChanged(const QStringList &options);

private:
    QString optionText(const QString &optionName, bool valid) const;
    QListWidgetItem *appendItem(const QString &optionName, bool valid, bool selected);
    void appendSeparator();
    void itemChanged(QListWidgetItem *item);

    QListWidget *m_listWidget = nullptr;
    QString m_noOptionText;
    QString m_invalidOptionText;
    QStringList m_validOptions;
    QStringList m_invalidOptions;
    QStringList m_selectedOptions;
    QHash<QString, QListWidgetItem *> m_optionToItem;
    QHash<QListWidgetItem *, QString> m_itemToOption;
};

QT_END_NAMESPACE

#endif // QOPTIONSWIDGET_H
