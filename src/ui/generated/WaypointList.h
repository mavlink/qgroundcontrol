/********************************************************************************
** Form generated from reading UI file 'WaypointList.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WAYPOINTLIST_H
#define WAYPOINTLIST_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WaypointList
{
public:
    QAction *actionAddWaypoint;
    QAction *actionTransmit;
    QAction *actionRead;
    QGridLayout *gridLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QHBoxLayout *horizontalLayout;
    QWidget *listWidget;
    QPushButton *readButton;
    QPushButton *transmitButton;
    QLabel *statusLabel;
    QToolButton *addButton;
    QPushButton *loadButton;
    QPushButton *saveButton;
    QSpacerItem *horizontalSpacer;
    QToolButton *positionAddButton;

    void setupUi(QWidget *WaypointList) {
        if (WaypointList->objectName().isEmpty())
            WaypointList->setObjectName(QString::fromUtf8("WaypointList"));
        WaypointList->resize(476, 218);
        WaypointList->setMinimumSize(QSize(100, 120));
        actionAddWaypoint = new QAction(WaypointList);
        actionAddWaypoint->setObjectName(QString::fromUtf8("actionAddWaypoint"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/actions/list-add.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionAddWaypoint->setIcon(icon);
        actionTransmit = new QAction(WaypointList);
        actionTransmit->setObjectName(QString::fromUtf8("actionTransmit"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/files/images/devices/network-wireless.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionTransmit->setIcon(icon1);
        actionRead = new QAction(WaypointList);
        actionRead->setObjectName(QString::fromUtf8("actionRead"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/files/images/status/software-update-available.svg"), QSize(), QIcon::Normal, QIcon::Off);
        actionRead->setIcon(icon2);
        gridLayout = new QGridLayout(WaypointList);
        gridLayout->setSpacing(4);
        gridLayout->setContentsMargins(4, 4, 4, 4);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        scrollArea = new QScrollArea(WaypointList);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 466, 149));
        horizontalLayout = new QHBoxLayout(scrollAreaWidgetContents);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setContentsMargins(4, 4, 4, 4);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        listWidget = new QWidget(scrollAreaWidgetContents);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        horizontalLayout->addWidget(listWidget);

        scrollArea->setWidget(scrollAreaWidgetContents);

        gridLayout->addWidget(scrollArea, 0, 0, 1, 9);

        readButton = new QPushButton(WaypointList);
        readButton->setObjectName(QString::fromUtf8("readButton"));
        readButton->setIcon(icon2);

        gridLayout->addWidget(readButton, 2, 7, 1, 1);

        transmitButton = new QPushButton(WaypointList);
        transmitButton->setObjectName(QString::fromUtf8("transmitButton"));
        transmitButton->setIcon(icon1);

        gridLayout->addWidget(transmitButton, 2, 8, 1, 1);

        statusLabel = new QLabel(WaypointList);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));

        gridLayout->addWidget(statusLabel, 3, 0, 1, 9);

        addButton = new QToolButton(WaypointList);
        addButton->setObjectName(QString::fromUtf8("addButton"));
        addButton->setIcon(icon);

        gridLayout->addWidget(addButton, 2, 6, 1, 1);

        loadButton = new QPushButton(WaypointList);
        loadButton->setObjectName(QString::fromUtf8("loadButton"));

        gridLayout->addWidget(loadButton, 2, 3, 1, 1);

        saveButton = new QPushButton(WaypointList);
        saveButton->setObjectName(QString::fromUtf8("saveButton"));

        gridLayout->addWidget(saveButton, 2, 2, 1, 1);

        horizontalSpacer = new QSpacerItem(127, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 2, 4, 1, 1);

        positionAddButton = new QToolButton(WaypointList);
        positionAddButton->setObjectName(QString::fromUtf8("positionAddButton"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/files/images/actions/go-bottom.svg"), QSize(), QIcon::Normal, QIcon::Off);
        positionAddButton->setIcon(icon3);

        gridLayout->addWidget(positionAddButton, 2, 5, 1, 1);

        gridLayout->setRowStretch(0, 100);

        retranslateUi(WaypointList);

        QMetaObject::connectSlotsByName(WaypointList);
    } // setupUi

    void retranslateUi(QWidget *WaypointList) {
        WaypointList->setWindowTitle(QApplication::translate("WaypointList", "Form", 0, QApplication::UnicodeUTF8));
        actionAddWaypoint->setText(QApplication::translate("WaypointList", "Add Waypoint", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionAddWaypoint->setToolTip(QApplication::translate("WaypointList", "Add a new waypoint to the end of the list", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionTransmit->setText(QApplication::translate("WaypointList", "Transmit", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionTransmit->setToolTip(QApplication::translate("WaypointList", "Transmit waypoints to unmanned system", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionRead->setText(QApplication::translate("WaypointList", "Read", 0, QApplication::UnicodeUTF8));
        readButton->setText(QApplication::translate("WaypointList", "Read", 0, QApplication::UnicodeUTF8));
        transmitButton->setText(QApplication::translate("WaypointList", "Write", 0, QApplication::UnicodeUTF8));
        statusLabel->setText(QApplication::translate("WaypointList", "TextLabel", 0, QApplication::UnicodeUTF8));
        addButton->setText(QApplication::translate("WaypointList", "...", 0, QApplication::UnicodeUTF8));
        loadButton->setText(QApplication::translate("WaypointList", "Load WPs", 0, QApplication::UnicodeUTF8));
        saveButton->setText(QApplication::translate("WaypointList", "Save WPs", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        positionAddButton->setToolTip(QApplication::translate("WaypointList", "Set the current vehicle position as new waypoint", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        positionAddButton->setText(QApplication::translate("WaypointList", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class WaypointList: public Ui_WaypointList {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WAYPOINTLIST_H
