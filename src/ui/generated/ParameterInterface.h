/********************************************************************************
** Form generated from reading UI file 'ParameterInterface.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PARAMETERINTERFACE_H
#define PARAMETERINTERFACE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_parameterWidget
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QComboBox *vehicleComboBox;
    QGroupBox *groupBox_2;
    QHBoxLayout *horizontalLayout;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QVBoxLayout *verticalLayout;
    QWidget *page_2;
    QStackedWidget *sensorSettings;
    QWidget *page_3;
    QVBoxLayout *verticalLayout_2;
    QWidget *page_4;

    void setupUi(QWidget *parameterWidget) {
        if (parameterWidget->objectName().isEmpty())
            parameterWidget->setObjectName(QString::fromUtf8("parameterWidget"));
        parameterWidget->resize(350, 545);
        gridLayout = new QGridLayout(parameterWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(parameterWidget);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        vehicleComboBox = new QComboBox(parameterWidget);
        vehicleComboBox->setObjectName(QString::fromUtf8("vehicleComboBox"));

        gridLayout->addWidget(vehicleComboBox, 0, 1, 1, 1);

        groupBox_2 = new QGroupBox(parameterWidget);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        horizontalLayout = new QHBoxLayout(groupBox_2);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        stackedWidget = new QStackedWidget(groupBox_2);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        verticalLayout = new QVBoxLayout(page);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        stackedWidget->addWidget(page_2);

        horizontalLayout->addWidget(stackedWidget);


        gridLayout->addWidget(groupBox_2, 1, 0, 1, 2);

        sensorSettings = new QStackedWidget(parameterWidget);
        sensorSettings->setObjectName(QString::fromUtf8("sensorSettings"));
        page_3 = new QWidget();
        page_3->setObjectName(QString::fromUtf8("page_3"));
        verticalLayout_2 = new QVBoxLayout(page_3);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        sensorSettings->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName(QString::fromUtf8("page_4"));
        sensorSettings->addWidget(page_4);

        gridLayout->addWidget(sensorSettings, 2, 0, 1, 2);


        retranslateUi(parameterWidget);

        stackedWidget->setCurrentIndex(0);
        sensorSettings->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(parameterWidget);
    } // setupUi

    void retranslateUi(QWidget *parameterWidget) {
        parameterWidget->setWindowTitle(QApplication::translate("parameterWidget", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("parameterWidget", "Vehicle", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("parameterWidget", "Onboard Parameters", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class parameterWidget: public Ui_parameterWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PARAMETERINTERFACE_H
