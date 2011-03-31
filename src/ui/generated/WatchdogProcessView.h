/********************************************************************************
** Form generated from reading UI file 'WatchdogProcessView.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WATCHDOGPROCESSVIEW_H
#define WATCHDOGPROCESSVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WatchdogProcessView
{
public:
    QHBoxLayout *horizontalLayout;
    QLabel *nameLabel;
    QLabel *pidLabel;
    QLabel *argumentsLabel;
    QToolButton *startButton;
    QToolButton *restartButton;

    void setupUi(QWidget *WatchdogProcessView) {
        if (WatchdogProcessView->objectName().isEmpty())
            WatchdogProcessView->setObjectName(QString::fromUtf8("WatchdogProcessView"));
        WatchdogProcessView->resize(400, 44);
        horizontalLayout = new QHBoxLayout(WatchdogProcessView);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        nameLabel = new QLabel(WatchdogProcessView);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));

        horizontalLayout->addWidget(nameLabel);

        pidLabel = new QLabel(WatchdogProcessView);
        pidLabel->setObjectName(QString::fromUtf8("pidLabel"));

        horizontalLayout->addWidget(pidLabel);

        argumentsLabel = new QLabel(WatchdogProcessView);
        argumentsLabel->setObjectName(QString::fromUtf8("argumentsLabel"));

        horizontalLayout->addWidget(argumentsLabel);

        startButton = new QToolButton(WatchdogProcessView);
        startButton->setObjectName(QString::fromUtf8("startButton"));

        horizontalLayout->addWidget(startButton);

        restartButton = new QToolButton(WatchdogProcessView);
        restartButton->setObjectName(QString::fromUtf8("restartButton"));

        horizontalLayout->addWidget(restartButton);


        retranslateUi(WatchdogProcessView);

        QMetaObject::connectSlotsByName(WatchdogProcessView);
    } // setupUi

    void retranslateUi(QWidget *WatchdogProcessView) {
        WatchdogProcessView->setWindowTitle(QApplication::translate("WatchdogProcessView", "Form", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("WatchdogProcessView", "TextLabel", 0, QApplication::UnicodeUTF8));
        pidLabel->setText(QApplication::translate("WatchdogProcessView", "TextLabel", 0, QApplication::UnicodeUTF8));
        argumentsLabel->setText(QApplication::translate("WatchdogProcessView", "TextLabel", 0, QApplication::UnicodeUTF8));
        startButton->setText(QApplication::translate("WatchdogProcessView", "...", 0, QApplication::UnicodeUTF8));
        restartButton->setText(QApplication::translate("WatchdogProcessView", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class WatchdogProcessView: public Ui_WatchdogProcessView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WATCHDOGPROCESSVIEW_H
