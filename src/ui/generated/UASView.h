/********************************************************************************
** Form generated from reading UI file 'UASView.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UASVIEW_H
#define UASVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UASView
{
public:
    QHBoxLayout *horizontalLayout_2;
    QGroupBox *uasViewFrame;
    QGridLayout *gridLayout;
    QToolButton *typeButton;
    QSpacerItem *horizontalSpacer_2;
    QLabel *nameLabel;
    QLabel *modeLabel;
    QLabel *timeRemainingLabel;
    QLabel *timeElapsedLabel;
    QProgressBar *thrustBar;
    QLabel *groundDistanceLabel;
    QLabel *speedLabel;
    QGroupBox *heartbeatIcon;
    QProgressBar *batteryBar;
    QLabel *stateLabel;
    QLabel *waypointLabel;
    QLabel *positionLabel;
    QLabel *gpsLabel;
    QLabel *statusTextLabel;
    QHBoxLayout *horizontalLayout;
    QPushButton *liftoffButton;
    QPushButton *haltButton;
    QPushButton *continueButton;
    QPushButton *landButton;
    QPushButton *shutdownButton;
    QPushButton *abortButton;
    QPushButton *killButton;

    void setupUi(QWidget *UASView) {
        if (UASView->objectName().isEmpty())
            UASView->setObjectName(QString::fromUtf8("UASView"));
        UASView->resize(362, 120);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(UASView->sizePolicy().hasHeightForWidth());
        UASView->setSizePolicy(sizePolicy);
        UASView->setMinimumSize(QSize(0, 0));
        UASView->setStyleSheet(QString::fromUtf8("QWidget#colorIcon {}\n"
                               "\n"
                               "QWidget {\n"
                               "background-color: none;\n"
                               "color: #DDDDDF;\n"
                               "border-color: #EEEEEE;\n"
                               "background-clip: margin;\n"
                               "}\n"
                               "\n"
                               "QLabel#nameLabel {\n"
                               "	font: bold 16px;\n"
                               "	color: #3C7B9E;\n"
                               "}\n"
                               "\n"
                               "QLabel#modeLabel {\n"
                               "	font: 12px;\n"
                               "}\n"
                               "\n"
                               "QLabel#stateLabel {\n"
                               "	font: 12px;\n"
                               "}\n"
                               "\n"
                               "QLabel#gpsLabel {\n"
                               "	font: 8px;\n"
                               "}\n"
                               "\n"
                               "QLabel#positionLabel {\n"
                               "	font: 8px;\n"
                               "}\n"
                               "\n"
                               "QLabel#timeElapsedLabel {\n"
                               "	font: 8px;\n"
                               "}\n"
                               "\n"
                               "QLabel#groundDistanceLabel {\n"
                               "	font: 8px;\n"
                               "}\n"
                               "\n"
                               "QLabel#speedLabel {\n"
                               "	font: 8px;\n"
                               "}\n"
                               "\n"
                               "QLabel#timeRemainingLabel {\n"
                               "	font: 8px;\n"
                               "}\n"
                               "\n"
                               "QLabel#waypointLabel {\n"
                               "	font: 24px;\n"
                               "}\n"
                               "\n"
                               "QGroupBox {\n"
                               "	border: 1px solid #4A4A4F;\n"
                               "	border-radius: 5px;\n"
                               "	padding: 0px 0px 0px 0px;\n"
                               "	margin: 0px;\n"
                               "}\n"
                               "\n"
                               " QGroupBox::title {\n"
                               "     subcontrol-origin: margin;\n"
                               "     subcontrol-position: top center; /* position at the top center */\n"
                               "     margin: 0 3px 0px 3px;\n"
                               " "
                               "    padding: 0 3px 0px 0px;\n"
                               "     font: bold 8px;\n"
                               " }\n"
                               "\n"
                               "QGroupBox#heartbeatIcon {\n"
                               "	background-color: red;\n"
                               "}\n"
                               "\n"
                               "QToolButton#typeButton {\n"
                               "	font-weight: bold;\n"
                               "	font-size: 12px;\n"
                               "	border: 2px solid #999999;\n"
                               "	border-radius: 5px;\n"
                               "	min-width:44px;\n"
                               "	max-width: 44px;\n"
                               "	min-height: 44px;\n"
                               "	max-height: 44px;\n"
                               "	padding: 0px;\n"
                               "	background-color: none;\n"
                               "}\n"
                               "\n"
                               "QPushButton {\n"
                               "	font-weight: bold;\n"
                               "	font-size: 12px;\n"
                               "	border: 1px solid #999999;\n"
                               "	border-radius: 10px;\n"
                               "	min-width: 20px;\n"
                               "	max-width: 40px;\n"
                               "	min-height: 16px;\n"
                               "	max-height: 16px;\n"
                               "	padding: 2px;\n"
                               "	background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #777777, stop: 1 #555555);\n"
                               "}\n"
                               "\n"
                               "QPushButton:pressed {\n"
                               "	background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #444444, stop: 1 #555555);\n"
                               "}\n"
                               "\n"
                               "QPushButton#abortButton {\n"
                               "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                               "                             stop:0 #"
                               "ffee01, stop:1 #ae8f00);\n"
                               "}\n"
                               "\n"
                               "QPushButton:pressed#abortButton {\n"
                               "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                               "                             stop:0 #bbaa00, stop:1 #a05b00);\n"
                               "}\n"
                               "\n"
                               "QPushButton#killButton {\n"
                               "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                               "                             stop:0 #ffb917, stop:1 #b37300);\n"
                               "}\n"
                               "\n"
                               "QPushButton:pressed#killButton {\n"
                               "	background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\n"
                               "                             stop:0 #bb8500, stop:1 #903000);\n"
                               "}\n"
                               "\n"
                               "\n"
                               "QProgressBar {\n"
                               "	border: 1px solid #4A4A4F;\n"
                               "	border-radius: 4px;\n"
                               "	text-align: center;\n"
                               "	padding: 2px;\n"
                               "	color: #DDDDDF;\n"
                               "	background-color: #111118;\n"
                               "}\n"
                               "\n"
                               "QProgressBar:horizontal {\n"
                               "	height: 10px;\n"
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
                               "	backgrou"
                               "nd-color: green;\n"
                               "}\n"
                               "\n"
                               "QProgressBar::chunk#speedBar {\n"
                               "	background-color: yellow;\n"
                               "}\n"
                               "\n"
                               "QProgressBar::chunk#thrustBar {\n"
                               "	background-color: orange;\n"
                               "}"));
        horizontalLayout_2 = new QHBoxLayout(UASView);
        horizontalLayout_2->setContentsMargins(6, 6, 6, 6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        uasViewFrame = new QGroupBox(UASView);
        uasViewFrame->setObjectName(QString::fromUtf8("uasViewFrame"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(uasViewFrame->sizePolicy().hasHeightForWidth());
        uasViewFrame->setSizePolicy(sizePolicy1);
        uasViewFrame->setMinimumSize(QSize(0, 0));
        gridLayout = new QGridLayout(uasViewFrame);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(2);
        typeButton = new QToolButton(uasViewFrame);
        typeButton->setObjectName(QString::fromUtf8("typeButton"));
        typeButton->setMinimumSize(QSize(48, 48));
        typeButton->setMaximumSize(QSize(48, 48));
        typeButton->setBaseSize(QSize(30, 30));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/mavs/unknown.svg"), QSize(), QIcon::Normal, QIcon::Off);
        typeButton->setIcon(icon);
        typeButton->setIconSize(QSize(42, 42));

        gridLayout->addWidget(typeButton, 0, 0, 5, 2);

        horizontalSpacer_2 = new QSpacerItem(6, 88, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 8, 1);

        nameLabel = new QLabel(uasViewFrame);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));
        nameLabel->setMaximumSize(QSize(16777215, 16));
        QFont font;
        font.setBold(true);
        font.setItalic(false);
        font.setWeight(75);
        nameLabel->setFont(font);

        gridLayout->addWidget(nameLabel, 0, 3, 1, 3);

        modeLabel = new QLabel(uasViewFrame);
        modeLabel->setObjectName(QString::fromUtf8("modeLabel"));
        modeLabel->setMaximumSize(QSize(16777215, 16));
        QFont font1;
        font1.setBold(false);
        font1.setItalic(false);
        font1.setWeight(50);
        modeLabel->setFont(font1);

        gridLayout->addWidget(modeLabel, 0, 6, 1, 2);

        timeRemainingLabel = new QLabel(uasViewFrame);
        timeRemainingLabel->setObjectName(QString::fromUtf8("timeRemainingLabel"));
        timeRemainingLabel->setFont(font1);

        gridLayout->addWidget(timeRemainingLabel, 1, 3, 3, 1);

        timeElapsedLabel = new QLabel(uasViewFrame);
        timeElapsedLabel->setObjectName(QString::fromUtf8("timeElapsedLabel"));
        timeElapsedLabel->setFont(font1);

        gridLayout->addWidget(timeElapsedLabel, 1, 4, 3, 2);

        thrustBar = new QProgressBar(uasViewFrame);
        thrustBar->setObjectName(QString::fromUtf8("thrustBar"));
        QFont font2;
        font2.setPointSize(8);
        thrustBar->setFont(font2);
        thrustBar->setValue(0);

        gridLayout->addWidget(thrustBar, 3, 6, 2, 2);

        groundDistanceLabel = new QLabel(uasViewFrame);
        groundDistanceLabel->setObjectName(QString::fromUtf8("groundDistanceLabel"));
        groundDistanceLabel->setFont(font1);

        gridLayout->addWidget(groundDistanceLabel, 4, 3, 1, 1);

        speedLabel = new QLabel(uasViewFrame);
        speedLabel->setObjectName(QString::fromUtf8("speedLabel"));
        speedLabel->setFont(font1);

        gridLayout->addWidget(speedLabel, 4, 4, 1, 2);

        heartbeatIcon = new QGroupBox(uasViewFrame);
        heartbeatIcon->setObjectName(QString::fromUtf8("heartbeatIcon"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(heartbeatIcon->sizePolicy().hasHeightForWidth());
        heartbeatIcon->setSizePolicy(sizePolicy2);
        heartbeatIcon->setMinimumSize(QSize(18, 0));
        heartbeatIcon->setMaximumSize(QSize(18, 40));

        gridLayout->addWidget(heartbeatIcon, 5, 0, 3, 1);

        batteryBar = new QProgressBar(uasViewFrame);
        batteryBar->setObjectName(QString::fromUtf8("batteryBar"));
        batteryBar->setMinimumSize(QSize(18, 0));
        batteryBar->setMaximumSize(QSize(18, 40));
        QFont font3;
        font3.setPointSize(6);
        batteryBar->setFont(font3);
        batteryBar->setValue(0);
        batteryBar->setOrientation(Qt::Vertical);

        gridLayout->addWidget(batteryBar, 5, 1, 3, 1);

        stateLabel = new QLabel(uasViewFrame);
        stateLabel->setObjectName(QString::fromUtf8("stateLabel"));
        stateLabel->setFont(font1);

        gridLayout->addWidget(stateLabel, 5, 3, 2, 2);

        waypointLabel = new QLabel(uasViewFrame);
        waypointLabel->setObjectName(QString::fromUtf8("waypointLabel"));
        waypointLabel->setFont(font1);
        waypointLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(waypointLabel, 7, 3, 1, 2);

        positionLabel = new QLabel(uasViewFrame);
        positionLabel->setObjectName(QString::fromUtf8("positionLabel"));
        positionLabel->setMinimumSize(QSize(0, 12));
        positionLabel->setMaximumSize(QSize(16777215, 12));
        positionLabel->setFont(font1);

        gridLayout->addWidget(positionLabel, 2, 6, 1, 1);

        gpsLabel = new QLabel(uasViewFrame);
        gpsLabel->setObjectName(QString::fromUtf8("gpsLabel"));
        gpsLabel->setMinimumSize(QSize(0, 12));
        gpsLabel->setMaximumSize(QSize(16777215, 12));
        gpsLabel->setFont(font1);

        gridLayout->addWidget(gpsLabel, 2, 7, 1, 1);

        statusTextLabel = new QLabel(uasViewFrame);
        statusTextLabel->setObjectName(QString::fromUtf8("statusTextLabel"));
        statusTextLabel->setMaximumSize(QSize(16777215, 12));
        QFont font4;
        font4.setPointSize(10);
        statusTextLabel->setFont(font4);

        gridLayout->addWidget(statusTextLabel, 5, 6, 1, 2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(4);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        liftoffButton = new QPushButton(uasViewFrame);
        liftoffButton->setObjectName(QString::fromUtf8("liftoffButton"));
        liftoffButton->setMinimumSize(QSize(26, 22));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/files/images/control/launch.svg"), QSize(), QIcon::Normal, QIcon::Off);
        liftoffButton->setIcon(icon1);

        horizontalLayout->addWidget(liftoffButton);

        haltButton = new QPushButton(uasViewFrame);
        haltButton->setObjectName(QString::fromUtf8("haltButton"));
        haltButton->setMinimumSize(QSize(26, 22));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/files/images/actions/media-playback-pause.svg"), QSize(), QIcon::Normal, QIcon::Off);
        haltButton->setIcon(icon2);

        horizontalLayout->addWidget(haltButton);

        continueButton = new QPushButton(uasViewFrame);
        continueButton->setObjectName(QString::fromUtf8("continueButton"));
        continueButton->setMinimumSize(QSize(26, 22));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/files/images/actions/media-playback-start.svg"), QSize(), QIcon::Normal, QIcon::Off);
        continueButton->setIcon(icon3);

        horizontalLayout->addWidget(continueButton);

        landButton = new QPushButton(uasViewFrame);
        landButton->setObjectName(QString::fromUtf8("landButton"));
        landButton->setMinimumSize(QSize(26, 22));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/files/images/control/land.svg"), QSize(), QIcon::Normal, QIcon::Off);
        landButton->setIcon(icon4);

        horizontalLayout->addWidget(landButton);

        shutdownButton = new QPushButton(uasViewFrame);
        shutdownButton->setObjectName(QString::fromUtf8("shutdownButton"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/files/images/actions/system-log-out.svg"), QSize(), QIcon::Normal, QIcon::Off);
        shutdownButton->setIcon(icon5);

        horizontalLayout->addWidget(shutdownButton);

        abortButton = new QPushButton(uasViewFrame);
        abortButton->setObjectName(QString::fromUtf8("abortButton"));
        abortButton->setMinimumSize(QSize(26, 22));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/files/images/actions/media-playback-stop.svg"), QSize(), QIcon::Normal, QIcon::Off);
        abortButton->setIcon(icon6);

        horizontalLayout->addWidget(abortButton);

        killButton = new QPushButton(uasViewFrame);
        killButton->setObjectName(QString::fromUtf8("killButton"));
        killButton->setMinimumSize(QSize(26, 22));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/files/images/actions/process-stop.svg"), QSize(), QIcon::Normal, QIcon::Off);
        killButton->setIcon(icon7);

        horizontalLayout->addWidget(killButton);


        gridLayout->addLayout(horizontalLayout, 6, 5, 2, 3);


        horizontalLayout_2->addWidget(uasViewFrame);


        retranslateUi(UASView);

        QMetaObject::connectSlotsByName(UASView);
    } // setupUi

    void retranslateUi(QWidget *UASView) {
        UASView->setWindowTitle(QApplication::translate("UASView", "Form", 0, QApplication::UnicodeUTF8));
        uasViewFrame->setTitle(QString());
        typeButton->setText(QApplication::translate("UASView", "...", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("UASView", "UAS001", 0, QApplication::UnicodeUTF8));
        modeLabel->setText(QApplication::translate("UASView", "Waiting for first update..", 0, QApplication::UnicodeUTF8));
        timeRemainingLabel->setText(QApplication::translate("UASView", "00:00:00", 0, QApplication::UnicodeUTF8));
        timeElapsedLabel->setText(QApplication::translate("UASView", "00:00:00", 0, QApplication::UnicodeUTF8));
        groundDistanceLabel->setText(QApplication::translate("UASView", "00.00 m", 0, QApplication::UnicodeUTF8));
        speedLabel->setText(QApplication::translate("UASView", "00.0 m/s", 0, QApplication::UnicodeUTF8));
        heartbeatIcon->setTitle(QString());
        stateLabel->setText(QApplication::translate("UASView", "UNINIT", 0, QApplication::UnicodeUTF8));
        waypointLabel->setText(QApplication::translate("UASView", "WPX", 0, QApplication::UnicodeUTF8));
        positionLabel->setText(QApplication::translate("UASView", "00.0  00.0  00.0 m", 0, QApplication::UnicodeUTF8));
        gpsLabel->setText(QApplication::translate("UASView", "00 00 00 N  00 00 00 E", 0, QApplication::UnicodeUTF8));
        statusTextLabel->setText(QApplication::translate("UASView", "Unknown status", 0, QApplication::UnicodeUTF8));
        liftoffButton->setText(QString());
        haltButton->setText(QString());
        continueButton->setText(QString());
        landButton->setText(QString());
        shutdownButton->setText(QString());
        abortButton->setText(QString());
        killButton->setText(QString());
    } // retranslateUi

};

namespace Ui
{
class UASView: public Ui_UASView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UASVIEW_H
