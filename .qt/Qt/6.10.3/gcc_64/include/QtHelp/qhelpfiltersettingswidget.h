// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPFILTERSETTINGSWIDGET_H
#define QHELPFILTERSETTINGSWIDGET_H

#include <QtHelp/qhelp_global.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QHelpFilterEngine;
class QHelpFilterSettingsWidgetPrivate;
class QVersionNumber;

class QHELP_EXPORT QHelpFilterSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QHelpFilterSettingsWidget(QWidget *parent = nullptr);

    ~QHelpFilterSettingsWidget();

    void setAvailableComponents(const QStringList &components);
    void setAvailableVersions(const QList<QVersionNumber> &versions);

    // TODO: filterEngine may be moved to c'tor or to setFilterEngine() setter
    void readSettings(const QHelpFilterEngine *filterEngine);
    bool applySettings(QHelpFilterEngine *filterEngine) const;

private:
    QScopedPointer<class QHelpFilterSettingsWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QHelpFilterSettingsWidget)
    Q_DISABLE_COPY_MOVE(QHelpFilterSettingsWidget)
};

QT_END_NAMESPACE

#endif // QHELPFILTERSETTINGSWIDGET_H
