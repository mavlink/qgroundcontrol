// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QCUPSJOBWIDGET_P_H
#define QCUPSJOBWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>
#include <private/qcups_p.h>

QT_REQUIRE_CONFIG(cupsjobwidget);

#include <ui_qcupsjobwidget.h>

QT_BEGIN_NAMESPACE

class QString;
class QTime;
class QPrinter;
class QPrintDevice;

class QCupsJobWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QCupsJobWidget(QPrinter *printer, QPrintDevice *printDevice, QWidget *parent = nullptr);
    ~QCupsJobWidget();
    void setupPrinter();
    void updateSavedValues();
    void revertToSavedValues();

private Q_SLOTS:
    void toggleJobHoldTime();

private:

    void setJobHold(QCUPSSupport::JobHoldUntil jobHold = QCUPSSupport::NoHold, QTime holdUntilTime = QTime());
    QCUPSSupport::JobHoldUntil jobHold() const;
    QTime jobHoldTime() const;

    void setJobBilling(const QString &jobBilling = QString());
    QString jobBilling() const;

    void setJobPriority(int priority = 50);
    int jobPriority() const;

    void setStartBannerPage(const QCUPSSupport::BannerPage bannerPage = QCUPSSupport::NoBanner);
    QCUPSSupport::BannerPage startBannerPage() const;

    void setEndBannerPage(const QCUPSSupport::BannerPage bannerPage = QCUPSSupport::NoBanner);
    QCUPSSupport::BannerPage endBannerPage() const;

    void initJobHold();
    void initJobBilling();
    void initJobPriority();
    void initBannerPages();

    QPrinter *m_printer;
    QPrintDevice *m_printDevice;
    Ui::QCupsJobWidget m_ui;

    QCUPSSupport::JobHoldUntilWithTime m_savedJobHoldWithTime;
    QString m_savedJobBilling;
    int m_savedPriority;
    QCUPSSupport::JobSheets m_savedJobSheets;

    Q_DISABLE_COPY_MOVE(QCupsJobWidget)
};

QT_END_NAMESPACE

#endif  // QCUPSJOBWIDGET_P_H
