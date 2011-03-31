/********************************************************************************
** Form generated from reading UI file 'MAVLinkSettingsWidget.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAVLINKSETTINGSWIDGET_H
#define MAVLINKSETTINGSWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QHeaderView>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MAVLinkSettingsWidget
{
public:
    QVBoxLayout *verticalLayout;
    QCheckBox *heartbeatCheckBox;
    QCheckBox *loggingCheckBox;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *MAVLinkSettingsWidget) {
        if (MAVLinkSettingsWidget->objectName().isEmpty())
            MAVLinkSettingsWidget->setObjectName(QString::fromUtf8("MAVLinkSettingsWidget"));
        MAVLinkSettingsWidget->resize(267, 123);
        verticalLayout = new QVBoxLayout(MAVLinkSettingsWidget);
        verticalLayout->setContentsMargins(6, 6, 6, 6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        heartbeatCheckBox = new QCheckBox(MAVLinkSettingsWidget);
        heartbeatCheckBox->setObjectName(QString::fromUtf8("heartbeatCheckBox"));

        verticalLayout->addWidget(heartbeatCheckBox);

        loggingCheckBox = new QCheckBox(MAVLinkSettingsWidget);
        loggingCheckBox->setObjectName(QString::fromUtf8("loggingCheckBox"));

        verticalLayout->addWidget(loggingCheckBox);

        verticalSpacer = new QSpacerItem(20, 84, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(MAVLinkSettingsWidget);

        QMetaObject::connectSlotsByName(MAVLinkSettingsWidget);
    } // setupUi

    void retranslateUi(QWidget *MAVLinkSettingsWidget) {
        MAVLinkSettingsWidget->setWindowTitle(QApplication::translate("MAVLinkSettingsWidget", "Form", 0, QApplication::UnicodeUTF8));
        heartbeatCheckBox->setText(QApplication::translate("MAVLinkSettingsWidget", "Emit heartbeat", 0, QApplication::UnicodeUTF8));
        loggingCheckBox->setText(QApplication::translate("MAVLinkSettingsWidget", "Log all MAVLink packets", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class MAVLinkSettingsWidget: public Ui_MAVLinkSettingsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAVLINKSETTINGSWIDGET_H
