/********************************************************************************
** Form generated from reading UI file 'QGCSensorSettingsWidget.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QGCSENSORSETTINGSWIDGET_H
#define QGCSENSORSETTINGSWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QGCSensorSettingsWidget
{
public:
    QGridLayout *gridLayout_4;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QCheckBox *sendRawCheckBox;
    QCheckBox *sendExtendedCheckBox;
    QCheckBox *sendRCCheckBox;
    QCheckBox *sendControllerCheckBox;
    QCheckBox *sendPositionCheckBox;
    QCheckBox *sendExtra1CheckBox;
    QCheckBox *sendExtra2CheckBox;
    QCheckBox *sendExtra3CheckBox;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout;
    QPushButton *gyroCalButton;
    QLabel *gyroCalDate;
    QPushButton *magCalButton;
    QLabel *magCalLabel;

    void setupUi(QWidget *QGCSensorSettingsWidget) {
        if (QGCSensorSettingsWidget->objectName().isEmpty())
            QGCSensorSettingsWidget->setObjectName(QString::fromUtf8("QGCSensorSettingsWidget"));
        QGCSensorSettingsWidget->resize(350, 545);
        gridLayout_4 = new QGridLayout(QGCSensorSettingsWidget);
        gridLayout_4->setContentsMargins(0, 0, 0, 0);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        groupBox = new QGroupBox(QGCSensorSettingsWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setContentsMargins(6, 6, 6, 6);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        sendRawCheckBox = new QCheckBox(groupBox);
        sendRawCheckBox->setObjectName(QString::fromUtf8("sendRawCheckBox"));

        gridLayout_2->addWidget(sendRawCheckBox, 0, 0, 1, 1);

        sendExtendedCheckBox = new QCheckBox(groupBox);
        sendExtendedCheckBox->setObjectName(QString::fromUtf8("sendExtendedCheckBox"));

        gridLayout_2->addWidget(sendExtendedCheckBox, 1, 0, 1, 1);

        sendRCCheckBox = new QCheckBox(groupBox);
        sendRCCheckBox->setObjectName(QString::fromUtf8("sendRCCheckBox"));

        gridLayout_2->addWidget(sendRCCheckBox, 2, 0, 1, 1);

        sendControllerCheckBox = new QCheckBox(groupBox);
        sendControllerCheckBox->setObjectName(QString::fromUtf8("sendControllerCheckBox"));

        gridLayout_2->addWidget(sendControllerCheckBox, 3, 0, 1, 1);

        sendPositionCheckBox = new QCheckBox(groupBox);
        sendPositionCheckBox->setObjectName(QString::fromUtf8("sendPositionCheckBox"));

        gridLayout_2->addWidget(sendPositionCheckBox, 4, 0, 1, 1);

        sendExtra1CheckBox = new QCheckBox(groupBox);
        sendExtra1CheckBox->setObjectName(QString::fromUtf8("sendExtra1CheckBox"));

        gridLayout_2->addWidget(sendExtra1CheckBox, 5, 0, 1, 1);

        sendExtra2CheckBox = new QCheckBox(groupBox);
        sendExtra2CheckBox->setObjectName(QString::fromUtf8("sendExtra2CheckBox"));

        gridLayout_2->addWidget(sendExtra2CheckBox, 6, 0, 1, 1);

        sendExtra3CheckBox = new QCheckBox(groupBox);
        sendExtra3CheckBox->setObjectName(QString::fromUtf8("sendExtra3CheckBox"));

        gridLayout_2->addWidget(sendExtra3CheckBox, 7, 0, 1, 1);


        gridLayout_4->addWidget(groupBox, 0, 0, 1, 1);

        groupBox_3 = new QGroupBox(QGCSensorSettingsWidget);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        gridLayout = new QGridLayout(groupBox_3);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gyroCalButton = new QPushButton(groupBox_3);
        gyroCalButton->setObjectName(QString::fromUtf8("gyroCalButton"));

        gridLayout->addWidget(gyroCalButton, 0, 0, 1, 1);

        gyroCalDate = new QLabel(groupBox_3);
        gyroCalDate->setObjectName(QString::fromUtf8("gyroCalDate"));

        gridLayout->addWidget(gyroCalDate, 0, 1, 1, 1);

        magCalButton = new QPushButton(groupBox_3);
        magCalButton->setObjectName(QString::fromUtf8("magCalButton"));

        gridLayout->addWidget(magCalButton, 1, 0, 1, 1);

        magCalLabel = new QLabel(groupBox_3);
        magCalLabel->setObjectName(QString::fromUtf8("magCalLabel"));

        gridLayout->addWidget(magCalLabel, 1, 1, 1, 1);


        gridLayout_4->addWidget(groupBox_3, 1, 0, 1, 1);


        retranslateUi(QGCSensorSettingsWidget);

        QMetaObject::connectSlotsByName(QGCSensorSettingsWidget);
    } // setupUi

    void retranslateUi(QWidget *QGCSensorSettingsWidget) {
        QGCSensorSettingsWidget->setWindowTitle(QApplication::translate("QGCSensorSettingsWidget", "Form", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("QGCSensorSettingsWidget", "Activate Extended Output", 0, QApplication::UnicodeUTF8));
        sendRawCheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send RAW Sensor data", 0, QApplication::UnicodeUTF8));
        sendExtendedCheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send extended status", 0, QApplication::UnicodeUTF8));
        sendRCCheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send RC-values", 0, QApplication::UnicodeUTF8));
        sendControllerCheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send raw controller outputs", 0, QApplication::UnicodeUTF8));
        sendPositionCheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send position setpoint and estimate", 0, QApplication::UnicodeUTF8));
        sendExtra1CheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send Extra1", 0, QApplication::UnicodeUTF8));
        sendExtra2CheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send Extra2", 0, QApplication::UnicodeUTF8));
        sendExtra3CheckBox->setText(QApplication::translate("QGCSensorSettingsWidget", "Send Extra3", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("QGCSensorSettingsWidget", "Calibration Wizards", 0, QApplication::UnicodeUTF8));
        gyroCalButton->setText(QApplication::translate("QGCSensorSettingsWidget", "Start dynamic calibration", 0, QApplication::UnicodeUTF8));
        gyroCalDate->setText(QApplication::translate("QGCSensorSettingsWidget", "Date unknown", 0, QApplication::UnicodeUTF8));
        magCalButton->setText(QApplication::translate("QGCSensorSettingsWidget", "Start static calibration", 0, QApplication::UnicodeUTF8));
        magCalLabel->setText(QApplication::translate("QGCSensorSettingsWidget", "Date unknown", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class QGCSensorSettingsWidget: public Ui_QGCSensorSettingsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QGCSENSORSETTINGSWIDGET_H
