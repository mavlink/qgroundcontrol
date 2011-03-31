/********************************************************************************
** Form generated from reading UI file 'JoystickWidget.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef JOYSTICKWIDGET_H
#define JOYSTICKWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDial>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLCDNumber>
#include <QtGui/QProgressBar>
#include <QtGui/QSlider>

QT_BEGIN_NAMESPACE

class Ui_JoystickWidget
{
public:
    QDialogButtonBox *buttonBox;
    QProgressBar *thrust;
    QSlider *ySlider;
    QSlider *xSlider;
    QLCDNumber *xValue;
    QLCDNumber *yValue;
    QDial *dial;
    QGroupBox *button0Label;

    void setupUi(QDialog *JoystickWidget) {
        if (JoystickWidget->objectName().isEmpty())
            JoystickWidget->setObjectName(QString::fromUtf8("JoystickWidget"));
        JoystickWidget->resize(400, 300);
        buttonBox = new QDialogButtonBox(JoystickWidget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(30, 240, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        thrust = new QProgressBar(JoystickWidget);
        thrust->setObjectName(QString::fromUtf8("thrust"));
        thrust->setGeometry(QRect(350, 20, 31, 181));
        thrust->setValue(0);
        thrust->setOrientation(Qt::Vertical);
        ySlider = new QSlider(JoystickWidget);
        ySlider->setObjectName(QString::fromUtf8("ySlider"));
        ySlider->setGeometry(QRect(160, 190, 160, 17));
        ySlider->setMaximum(100);
        ySlider->setOrientation(Qt::Horizontal);
        xSlider = new QSlider(JoystickWidget);
        xSlider->setObjectName(QString::fromUtf8("xSlider"));
        xSlider->setGeometry(QRect(140, 40, 17, 160));
        xSlider->setMaximum(100);
        xSlider->setOrientation(Qt::Vertical);
        xValue = new QLCDNumber(JoystickWidget);
        xValue->setObjectName(QString::fromUtf8("xValue"));
        xValue->setGeometry(QRect(160, 50, 41, 23));
        xValue->setFrameShadow(QFrame::Plain);
        xValue->setSmallDecimalPoint(true);
        xValue->setNumDigits(3);
        xValue->setSegmentStyle(QLCDNumber::Flat);
        yValue = new QLCDNumber(JoystickWidget);
        yValue->setObjectName(QString::fromUtf8("yValue"));
        yValue->setGeometry(QRect(270, 160, 41, 23));
        yValue->setFrameShadow(QFrame::Plain);
        yValue->setSmallDecimalPoint(true);
        yValue->setNumDigits(3);
        yValue->setSegmentStyle(QLCDNumber::Flat);
        dial = new QDial(JoystickWidget);
        dial->setObjectName(QString::fromUtf8("dial"));
        dial->setGeometry(QRect(210, 50, 101, 101));
        button0Label = new QGroupBox(JoystickWidget);
        button0Label->setObjectName(QString::fromUtf8("button0Label"));
        button0Label->setGeometry(QRect(60, 20, 41, 16));
        button0Label->setAutoFillBackground(false);
        button0Label->setFlat(false);

        retranslateUi(JoystickWidget);
        QObject::connect(buttonBox, SIGNAL(accepted()), JoystickWidget, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), JoystickWidget, SLOT(reject()));

        QMetaObject::connectSlotsByName(JoystickWidget);
    } // setupUi

    void retranslateUi(QDialog *JoystickWidget) {
        JoystickWidget->setWindowTitle(QApplication::translate("JoystickWidget", "Dialog", 0, QApplication::UnicodeUTF8));
        button0Label->setStyleSheet(QApplication::translate("JoystickWidget", "QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: #FF2222;}", 0, QApplication::UnicodeUTF8));
        button0Label->setTitle(QString());
    } // retranslateUi

};

namespace Ui
{
class JoystickWidget: public Ui_JoystickWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // JOYSTICKWIDGET_H
