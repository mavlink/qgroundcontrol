/********************************************************************************
** Form generated from reading UI file 'WatchdogControl.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WATCHDOGCONTROL_H
#define WATCHDOGCONTROL_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WatchdogControl
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *mainWidget;
    QLabel *processInfoLabel;

    void setupUi(QWidget *WatchdogControl) {
        if (WatchdogControl->objectName().isEmpty())
            WatchdogControl->setObjectName(QString::fromUtf8("WatchdogControl"));
        WatchdogControl->resize(400, 300);
        verticalLayout = new QVBoxLayout(WatchdogControl);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        mainWidget = new QWidget(WatchdogControl);
        mainWidget->setObjectName(QString::fromUtf8("mainWidget"));

        verticalLayout->addWidget(mainWidget);

        processInfoLabel = new QLabel(WatchdogControl);
        processInfoLabel->setObjectName(QString::fromUtf8("processInfoLabel"));

        verticalLayout->addWidget(processInfoLabel);

        verticalLayout->setStretch(0, 100);

        retranslateUi(WatchdogControl);

        QMetaObject::connectSlotsByName(WatchdogControl);
    } // setupUi

    void retranslateUi(QWidget *WatchdogControl) {
        WatchdogControl->setWindowTitle(QApplication::translate("WatchdogControl", "Form", 0, QApplication::UnicodeUTF8));
        processInfoLabel->setText(QApplication::translate("WatchdogControl", "0 Processes  Core 1: 0%  Core 2: 0%", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class WatchdogControl: public Ui_WatchdogControl {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WATCHDOGCONTROL_H
