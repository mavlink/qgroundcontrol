/********************************************************************************
** Form generated from reading UI file 'WaypointView.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WAYPOINTVIEW_H
#define WAYPOINTVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WaypointView
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout;
    QCheckBox *selectedBox;
    QLabel *idLabel;
    QSpacerItem *horizontalSpacer_2;
    QDoubleSpinBox *xSpinBox;
    QDoubleSpinBox *ySpinBox;
    QDoubleSpinBox *zSpinBox;
    QSpinBox *yawSpinBox;
    QDoubleSpinBox *orbitSpinBox;
    QSpinBox *holdTimeSpinBox;
    QCheckBox *autoContinue;
    QPushButton *upButton;
    QPushButton *downButton;
    QPushButton *removeButton;

    void setupUi(QWidget *WaypointView) {
        if (WaypointView->objectName().isEmpty())
            WaypointView->setObjectName(QString::fromUtf8("WaypointView"));
        WaypointView->resize(763, 40);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(WaypointView->sizePolicy().hasHeightForWidth());
        WaypointView->setSizePolicy(sizePolicy);
        WaypointView->setMinimumSize(QSize(200, 0));
        gridLayout = new QGridLayout(WaypointView);
        gridLayout->setSpacing(0);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        groupBox = new QGroupBox(WaypointView);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        horizontalLayout = new QHBoxLayout(groupBox);
        horizontalLayout->setSpacing(2);
        horizontalLayout->setContentsMargins(5, 5, 5, 5);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        selectedBox = new QCheckBox(groupBox);
        selectedBox->setObjectName(QString::fromUtf8("selectedBox"));
        selectedBox->setIconSize(QSize(16, 16));

        horizontalLayout->addWidget(selectedBox);

        idLabel = new QLabel(groupBox);
        idLabel->setObjectName(QString::fromUtf8("idLabel"));
        idLabel->setMinimumSize(QSize(15, 0));
        idLabel->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(idLabel);

        horizontalSpacer_2 = new QSpacerItem(25, 12, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        xSpinBox = new QDoubleSpinBox(groupBox);
        xSpinBox->setObjectName(QString::fromUtf8("xSpinBox"));
        xSpinBox->setMinimum(-1000);
        xSpinBox->setMaximum(1000);
        xSpinBox->setSingleStep(0.05);

        horizontalLayout->addWidget(xSpinBox);

        ySpinBox = new QDoubleSpinBox(groupBox);
        ySpinBox->setObjectName(QString::fromUtf8("ySpinBox"));
        ySpinBox->setMinimum(-1000);
        ySpinBox->setMaximum(1000);
        ySpinBox->setSingleStep(0.05);
        ySpinBox->setValue(0);

        horizontalLayout->addWidget(ySpinBox);

        zSpinBox = new QDoubleSpinBox(groupBox);
        zSpinBox->setObjectName(QString::fromUtf8("zSpinBox"));
        zSpinBox->setMinimum(-1000);
        zSpinBox->setMaximum(0);
        zSpinBox->setSingleStep(0.05);

        horizontalLayout->addWidget(zSpinBox);

        yawSpinBox = new QSpinBox(groupBox);
        yawSpinBox->setObjectName(QString::fromUtf8("yawSpinBox"));
        yawSpinBox->setWrapping(true);
        yawSpinBox->setMinimum(-180);
        yawSpinBox->setMaximum(180);
        yawSpinBox->setSingleStep(10);

        horizontalLayout->addWidget(yawSpinBox);

        orbitSpinBox = new QDoubleSpinBox(groupBox);
        orbitSpinBox->setObjectName(QString::fromUtf8("orbitSpinBox"));
        orbitSpinBox->setMinimum(0.05);
        orbitSpinBox->setMaximum(1);
        orbitSpinBox->setSingleStep(0.05);

        horizontalLayout->addWidget(orbitSpinBox);

        holdTimeSpinBox = new QSpinBox(groupBox);
        holdTimeSpinBox->setObjectName(QString::fromUtf8("holdTimeSpinBox"));
        holdTimeSpinBox->setMaximum(60000);
        holdTimeSpinBox->setSingleStep(500);
        holdTimeSpinBox->setValue(0);

        horizontalLayout->addWidget(holdTimeSpinBox);

        autoContinue = new QCheckBox(groupBox);
        autoContinue->setObjectName(QString::fromUtf8("autoContinue"));

        horizontalLayout->addWidget(autoContinue);

        upButton = new QPushButton(groupBox);
        upButton->setObjectName(QString::fromUtf8("upButton"));
        upButton->setMinimumSize(QSize(28, 22));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/actions/go-up.svg"), QSize(), QIcon::Normal, QIcon::Off);
        upButton->setIcon(icon);

        horizontalLayout->addWidget(upButton);

        downButton = new QPushButton(groupBox);
        downButton->setObjectName(QString::fromUtf8("downButton"));
        downButton->setMinimumSize(QSize(28, 22));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/files/images/actions/go-down.svg"), QSize(), QIcon::Normal, QIcon::Off);
        downButton->setIcon(icon1);

        horizontalLayout->addWidget(downButton);

        removeButton = new QPushButton(groupBox);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));
        removeButton->setMinimumSize(QSize(28, 22));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/files/images/actions/list-remove.svg"), QSize(), QIcon::Normal, QIcon::Off);
        removeButton->setIcon(icon2);

        horizontalLayout->addWidget(removeButton);


        gridLayout->addWidget(groupBox, 0, 0, 1, 1);


        retranslateUi(WaypointView);

        QMetaObject::connectSlotsByName(WaypointView);
    } // setupUi

    void retranslateUi(QWidget *WaypointView) {
        WaypointView->setWindowTitle(QApplication::translate("WaypointView", "Form", 0, QApplication::UnicodeUTF8));
        WaypointView->setStyleSheet(QApplication::translate("WaypointView", "QWidget#colorIcon {}\n"
                                    "\n"
                                    "QWidget {\n"
                                    "background-color: #252528;\n"
                                    "color: #DDDDDF;\n"
                                    "border-color: #EEEEEE;\n"
                                    "background-clip: border;\n"
                                    "}\n"
                                    "\n"
                                    "QCheckBox {\n"
                                    "background-color: #252528;\n"
                                    "color: #454545;\n"
                                    "}\n"
                                    "\n"
                                    "QGroupBox {\n"
                                    "	border: 1px solid #EEEEEE;\n"
                                    "	border-radius: 5px;\n"
                                    "	padding: 0px 0px 0px 0px;\n"
                                    "margin-top: 1ex; /* leave space at the top for the title */\n"
                                    "	margin: 0px;\n"
                                    "}\n"
                                    "\n"
                                    " QGroupBox::title {\n"
                                    "     subcontrol-origin: margin;\n"
                                    "     subcontrol-position: top center; /* position at the top center */\n"
                                    "     margin: 0 3px 0px 3px;\n"
                                    "     padding: 0 3px 0px 0px;\n"
                                    "     font: bold 8px;\n"
                                    " }\n"
                                    "\n"
                                    "QGroupBox#heartbeatIcon {\n"
                                    "	background-color: red;\n"
                                    "}\n"
                                    "\n"
                                    " QDockWidget {\n"
                                    "	font: bold;\n"
                                    "    border: 1px solid #32345E;\n"
                                    "}\n"
                                    "\n"
                                    "QPushButton {\n"
                                    "	font-weight: bold;\n"
                                    "	font-size: 12px;\n"
                                    "	border: 1px solid #999999;\n"
                                    "	border-radius: 10px;\n"
                                    "	min-width:22px;\n"
                                    "	max-width: 36px;\n"
                                    "	min-height: 16px;\n"
                                    "	max-height:"
                                    " 16px;\n"
                                    "	padding: 2px;\n"
                                    "	background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #777777, stop: 1 #555555);\n"
                                    "}\n"
                                    "\n"
                                    "QPushButton:pressed {\n"
                                    "	background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #444444, stop: 1 #555555);\n"
                                    "}\n"
                                    "\n"
                                    "QPushButton#landButton {\n"
                                    "	color: #000000;\n"
                                    "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                                    "                             stop:0 #ffee01, stop:1 #ae8f00) url(\"ICONDIR/control/emergency-button.png\");\n"
                                    "}\n"
                                    "\n"
                                    "QPushButton:pressed#landButton {\n"
                                    "	color: #000000;\n"
                                    "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                                    "                             stop:0 #bbaa00, stop:1 #a05b00) url(\"ICONDIR/control/emergency-button.png\");\n"
                                    "}\n"
                                    "\n"
                                    "QPushButton#killButton {\n"
                                    "	color: #000000;\n"
                                    "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                                    "                             stop:0 #ffb917, stop:1 #b37300) url(\"ICONDIR/control/emergency-button.png\");\n"
                                    "}\n"
                                    "\n"
                                    "QPushButton:pressed#killButton {\n"
                                    "	color: "
                                    "#000000;\n"
                                    "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                                    "                             stop:0 #bb8500, stop:1 #903000) url(\"ICONDIR/control/emergency-button.png\");\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar {\n"
                                    "	border: 1px solid white;\n"
                                    "	border-radius: 4px;\n"
                                    "	text-align: center;\n"
                                    "	padding: 2px;\n"
                                    "	color: white;\n"
                                    "	background-color: #111111;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar:horizontal {\n"
                                    "	height: 12px;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar QLabel {\n"
                                    "	font-size: 8px;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar:vertical {\n"
                                    "	width: 12px;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar::chunk {\n"
                                    "	background-color: #656565;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar::chunk#batteryBar {\n"
                                    "	background-color: green;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar::chunk#speedBar {\n"
                                    "	background-color: yellow;\n"
                                    "}\n"
                                    "\n"
                                    "QProgressBar::chunk#thrustBar {\n"
                                    "	background-color: orange;\n"
                                    "}", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QString());
#ifndef QT_NO_TOOLTIP
        selectedBox->setToolTip(QApplication::translate("WaypointView", "Currently selected waypoint", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        selectedBox->setText(QString());
#ifndef QT_NO_TOOLTIP
        idLabel->setToolTip(QApplication::translate("WaypointView", "Waypoint Sequence Number", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        idLabel->setText(QApplication::translate("WaypointView", "TextLabel", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        xSpinBox->setToolTip(QApplication::translate("WaypointView", "Position X coordinate", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        xSpinBox->setSuffix(QApplication::translate("WaypointView", " m", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ySpinBox->setToolTip(QApplication::translate("WaypointView", "Position Y coordinate", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        ySpinBox->setSuffix(QApplication::translate("WaypointView", " m", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        zSpinBox->setToolTip(QApplication::translate("WaypointView", "Position Z coordinate", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        zSpinBox->setSuffix(QApplication::translate("WaypointView", " m", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        yawSpinBox->setToolTip(QApplication::translate("WaypointView", "Yaw angle", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        yawSpinBox->setSuffix(QApplication::translate("WaypointView", "\302\260", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        orbitSpinBox->setToolTip(QApplication::translate("WaypointView", "Orbit radius", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        orbitSpinBox->setSuffix(QApplication::translate("WaypointView", " m", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_STATUSTIP
        holdTimeSpinBox->setStatusTip(QApplication::translate("WaypointView", "Time in milliseconds that the MAV has to stay inside the orbit before advancing", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        holdTimeSpinBox->setSuffix(QApplication::translate("WaypointView", " ms", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        autoContinue->setToolTip(QApplication::translate("WaypointView", "Automatically continue after this waypoint", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        autoContinue->setText(QString());
#ifndef QT_NO_TOOLTIP
        upButton->setToolTip(QApplication::translate("WaypointView", "Move Up", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        upButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        downButton->setToolTip(QApplication::translate("WaypointView", "Move Down", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        downButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        removeButton->setToolTip(QApplication::translate("WaypointView", "Delete", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        removeButton->setText(QString());
    } // retranslateUi

};

namespace Ui
{
class WaypointView: public Ui_WaypointView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WAYPOINTVIEW_H
