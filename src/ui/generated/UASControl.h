/********************************************************************************
** Form generated from reading UI file 'UASControl.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UASCONTROL_H
#define UASCONTROL_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_uasControl
{
public:
    QGridLayout *gridLayout;
    QLabel *controlStatusLabel;
    QPushButton *controlButton;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *liftoffButton;
    QPushButton *landButton;
    QPushButton *shutdownButton;
    QSpacerItem *horizontalSpacer_2;
    QComboBox *modeComboBox;
    QPushButton *setModeButton;
    QLabel *lastActionLabel;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *uasControl) {
        if (uasControl->objectName().isEmpty())
            uasControl->setObjectName(QString::fromUtf8("uasControl"));
        uasControl->resize(280, 164);
        uasControl->setMinimumSize(QSize(280, 130));
        gridLayout = new QGridLayout(uasControl);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(6, 12, 6, 6);
        controlStatusLabel = new QLabel(uasControl);
        controlStatusLabel->setObjectName(QString::fromUtf8("controlStatusLabel"));
        controlStatusLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(controlStatusLabel, 0, 1, 1, 4);

        controlButton = new QPushButton(uasControl);
        controlButton->setObjectName(QString::fromUtf8("controlButton"));

        gridLayout->addWidget(controlButton, 1, 1, 1, 4);

        verticalSpacer_2 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        gridLayout->addItem(verticalSpacer_2, 2, 1, 1, 4);

        horizontalSpacer = new QSpacerItem(31, 159, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 0, 6, 1);

        liftoffButton = new QPushButton(uasControl);
        liftoffButton->setObjectName(QString::fromUtf8("liftoffButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/control/launch.svg"), QSize(), QIcon::Normal, QIcon::Off);
        liftoffButton->setIcon(icon);

        gridLayout->addWidget(liftoffButton, 3, 1, 1, 1);

        landButton = new QPushButton(uasControl);
        landButton->setObjectName(QString::fromUtf8("landButton"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/files/images/control/land.svg"), QSize(), QIcon::Normal, QIcon::Off);
        landButton->setIcon(icon1);

        gridLayout->addWidget(landButton, 3, 2, 1, 2);

        shutdownButton = new QPushButton(uasControl);
        shutdownButton->setObjectName(QString::fromUtf8("shutdownButton"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/files/images/actions/system-log-out.svg"), QSize(), QIcon::Normal, QIcon::Off);
        shutdownButton->setIcon(icon2);

        gridLayout->addWidget(shutdownButton, 3, 4, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(30, 159, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 5, 6, 1);

        modeComboBox = new QComboBox(uasControl);
        modeComboBox->setObjectName(QString::fromUtf8("modeComboBox"));

        gridLayout->addWidget(modeComboBox, 4, 1, 1, 2);

        setModeButton = new QPushButton(uasControl);
        setModeButton->setObjectName(QString::fromUtf8("setModeButton"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/files/images/devices/network-wireless.svg"), QSize(), QIcon::Normal, QIcon::Off);
        setModeButton->setIcon(icon3);

        gridLayout->addWidget(setModeButton, 4, 3, 1, 2);

        lastActionLabel = new QLabel(uasControl);
        lastActionLabel->setObjectName(QString::fromUtf8("lastActionLabel"));
        lastActionLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(lastActionLabel, 5, 1, 1, 4);

        verticalSpacer = new QSpacerItem(0, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 6, 0, 1, 6);

        gridLayout->setRowMinimumHeight(0, 5);
        gridLayout->setRowMinimumHeight(1, 10);
        gridLayout->setRowMinimumHeight(2, 10);
        gridLayout->setRowMinimumHeight(4, 10);
        gridLayout->setRowMinimumHeight(5, 5);

        retranslateUi(uasControl);

        QMetaObject::connectSlotsByName(uasControl);
    } // setupUi

    void retranslateUi(QWidget *uasControl) {
        uasControl->setWindowTitle(QApplication::translate("uasControl", "Form", 0, QApplication::UnicodeUTF8));
        controlStatusLabel->setText(QApplication::translate("uasControl", "UNCONNECTED", 0, QApplication::UnicodeUTF8));
        controlButton->setText(QApplication::translate("uasControl", "Activate Engine", 0, QApplication::UnicodeUTF8));
        liftoffButton->setText(QApplication::translate("uasControl", "Liftoff", 0, QApplication::UnicodeUTF8));
        landButton->setText(QApplication::translate("uasControl", "Land", 0, QApplication::UnicodeUTF8));
        shutdownButton->setText(QApplication::translate("uasControl", "Halt", 0, QApplication::UnicodeUTF8));
        setModeButton->setText(QApplication::translate("uasControl", "Set Mode", 0, QApplication::UnicodeUTF8));
        lastActionLabel->setText(QApplication::translate("uasControl", "No actions executed so far", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class uasControl: public Ui_uasControl {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UASCONTROL_H
