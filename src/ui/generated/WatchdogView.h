/********************************************************************************
** Form generated from reading UI file 'WatchdogView.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WATCHDOGVIEW_H
#define WATCHDOGVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WatchdogView
{
public:
    QGridLayout *gridLayout;
    QLabel *nameLabel;
    QWidget *processListWidget;

    void setupUi(QWidget *WatchdogView) {
        if (WatchdogView->objectName().isEmpty())
            WatchdogView->setObjectName(QString::fromUtf8("WatchdogView"));
        WatchdogView->resize(400, 300);
        gridLayout = new QGridLayout(WatchdogView);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        nameLabel = new QLabel(WatchdogView);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));

        gridLayout->addWidget(nameLabel, 0, 0, 1, 1);

        processListWidget = new QWidget(WatchdogView);
        processListWidget->setObjectName(QString::fromUtf8("processListWidget"));

        gridLayout->addWidget(processListWidget, 1, 0, 1, 1);

        gridLayout->setRowStretch(1, 100);

        retranslateUi(WatchdogView);

        QMetaObject::connectSlotsByName(WatchdogView);
    } // setupUi

    void retranslateUi(QWidget *WatchdogView) {
        WatchdogView->setWindowTitle(QApplication::translate("WatchdogView", "Form", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("WatchdogView", "Watchdog", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class WatchdogView: public Ui_WatchdogView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WATCHDOGVIEW_H
