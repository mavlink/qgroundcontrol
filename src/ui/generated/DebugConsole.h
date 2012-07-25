/********************************************************************************
** Form generated from reading UI file 'DebugConsole.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DEBUGCONSOLE_H
#define DEBUGCONSOLE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DebugConsole
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_2;
    QComboBox *linkComboBox;
    QLabel *speedLabel;
    QCheckBox *mavlinkCheckBox;
    QCheckBox *hexCheckBox;
    QCheckBox *holdCheckBox;
    QPlainTextEdit *receiveText;
    QLineEdit *sentText;
    QLineEdit *sendText;
    QHBoxLayout *horizontalLayout;
    QPushButton *transmitButton;
    QPushButton *holdButton;
    QPushButton *clearButton;

    void setupUi(QWidget *DebugConsole) {
        if (DebugConsole->objectName().isEmpty())
            DebugConsole->setObjectName(QString::fromUtf8("DebugConsole"));
        DebugConsole->resize(435, 185);
        gridLayout = new QGridLayout(DebugConsole);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(4);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        linkComboBox = new QComboBox(DebugConsole);
        linkComboBox->setObjectName(QString::fromUtf8("linkComboBox"));
        linkComboBox->setMaximumSize(QSize(130, 16777215));

        horizontalLayout_2->addWidget(linkComboBox);

        speedLabel = new QLabel(DebugConsole);
        speedLabel->setObjectName(QString::fromUtf8("speedLabel"));

        horizontalLayout_2->addWidget(speedLabel);

        mavlinkCheckBox = new QCheckBox(DebugConsole);
        mavlinkCheckBox->setObjectName(QString::fromUtf8("mavlinkCheckBox"));

        horizontalLayout_2->addWidget(mavlinkCheckBox);

        hexCheckBox = new QCheckBox(DebugConsole);
        hexCheckBox->setObjectName(QString::fromUtf8("hexCheckBox"));

        horizontalLayout_2->addWidget(hexCheckBox);

        holdCheckBox = new QCheckBox(DebugConsole);
        holdCheckBox->setObjectName(QString::fromUtf8("holdCheckBox"));

        horizontalLayout_2->addWidget(holdCheckBox);

        horizontalLayout_2->setStretch(0, 10);

        gridLayout->addLayout(horizontalLayout_2, 0, 0, 1, 2);

        receiveText = new QPlainTextEdit(DebugConsole);
        receiveText->setObjectName(QString::fromUtf8("receiveText"));

        gridLayout->addWidget(receiveText, 1, 0, 1, 2);

        sentText = new QLineEdit(DebugConsole);
        sentText->setObjectName(QString::fromUtf8("sentText"));
        sentText->setReadOnly(true);

        gridLayout->addWidget(sentText, 2, 0, 1, 2);

        sendText = new QLineEdit(DebugConsole);
        sendText->setObjectName(QString::fromUtf8("sendText"));

        gridLayout->addWidget(sendText, 3, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        transmitButton = new QPushButton(DebugConsole);
        transmitButton->setObjectName(QString::fromUtf8("transmitButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/devices/network-wireless.svg"), QSize(), QIcon::Normal, QIcon::Off);
        transmitButton->setIcon(icon);

        horizontalLayout->addWidget(transmitButton);

        holdButton = new QPushButton(DebugConsole);
        holdButton->setObjectName(QString::fromUtf8("holdButton"));
        holdButton->setCheckable(true);

        horizontalLayout->addWidget(holdButton);

        clearButton = new QPushButton(DebugConsole);
        clearButton->setObjectName(QString::fromUtf8("clearButton"));

        horizontalLayout->addWidget(clearButton);


        gridLayout->addLayout(horizontalLayout, 3, 1, 1, 1);


        retranslateUi(DebugConsole);
        QObject::connect(clearButton, SIGNAL(clicked()), receiveText, SLOT(clear()));

        QMetaObject::connectSlotsByName(DebugConsole);
    } // setupUi

    void retranslateUi(QWidget *DebugConsole) {
        DebugConsole->setWindowTitle(QApplication::translate("DebugConsole", "Form", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        linkComboBox->setToolTip(QApplication::translate("DebugConsole", "Select the link to monitor", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        speedLabel->setText(QApplication::translate("DebugConsole", "0.0 kB/s", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        mavlinkCheckBox->setToolTip(QApplication::translate("DebugConsole", "Ignore MAVLINK protocol messages in display", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        mavlinkCheckBox->setText(QApplication::translate("DebugConsole", "No MAVLINK", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        hexCheckBox->setToolTip(QApplication::translate("DebugConsole", "Display and send bytes in HEX representation", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        hexCheckBox->setText(QApplication::translate("DebugConsole", "HEX", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        holdCheckBox->setToolTip(QApplication::translate("DebugConsole", "Saves CPU ressources, automatically set view to hold if data rate is too high to prevent fast scrolling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        holdCheckBox->setStatusTip(QApplication::translate("DebugConsole", "Saves CPU ressources, automatically set view to hold if data rate is too high to prevent fast scrolling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
#ifndef QT_NO_WHATSTHIS
        holdCheckBox->setWhatsThis(QApplication::translate("DebugConsole", "Enable auto hold to lower the CPU consumption", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        holdCheckBox->setText(QApplication::translate("DebugConsole", "Auto hold", 0, QApplication::UnicodeUTF8));
        sentText->setText(QApplication::translate("DebugConsole", "Enter data/text below to send", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        sendText->setToolTip(QApplication::translate("DebugConsole", "Type the bytes to send here, use 0xAA format for HEX (Check HEX checkbox above)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        transmitButton->setToolTip(QApplication::translate("DebugConsole", "Send the ASCII text or HEX values over the link", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        transmitButton->setText(QApplication::translate("DebugConsole", "Send", 0, QApplication::UnicodeUTF8));
        holdButton->setText(QApplication::translate("DebugConsole", "Hold", 0, QApplication::UnicodeUTF8));
        clearButton->setText(QApplication::translate("DebugConsole", "Clear", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class DebugConsole: public Ui_DebugConsole {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DEBUGCONSOLE_H
