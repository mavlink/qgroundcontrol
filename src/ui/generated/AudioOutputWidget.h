/********************************************************************************
** Form generated from reading UI file 'AudioOutputWidget.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef AUDIOOUTPUTWIDGET_H
#define AUDIOOUTPUTWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AudioOutputWidget
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *verticalSpacer;
    QPushButton *muteButton;

    void setupUi(QWidget *AudioOutputWidget) {
        if (AudioOutputWidget->objectName().isEmpty())
            AudioOutputWidget->setObjectName(QString::fromUtf8("AudioOutputWidget"));
        AudioOutputWidget->resize(195, 95);
        gridLayout = new QGridLayout(AudioOutputWidget);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        horizontalSpacer = new QSpacerItem(53, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 2, 1, 2);

        verticalSpacer = new QSpacerItem(180, 43, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 1, 0, 1, 4);

        muteButton = new QPushButton(AudioOutputWidget);
        muteButton->setObjectName(QString::fromUtf8("muteButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/files/images/status/audio-volume-muted.svg"), QSize(), QIcon::Normal, QIcon::Off);
        muteButton->setIcon(icon);
        muteButton->setIconSize(QSize(16, 16));

        gridLayout->addWidget(muteButton, 0, 0, 1, 1);


        retranslateUi(AudioOutputWidget);

        QMetaObject::connectSlotsByName(AudioOutputWidget);
    } // setupUi

    void retranslateUi(QWidget *AudioOutputWidget) {
        AudioOutputWidget->setWindowTitle(QApplication::translate("AudioOutputWidget", "Form", 0, QApplication::UnicodeUTF8));
        muteButton->setText(QApplication::translate("AudioOutputWidget", "Mute", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class AudioOutputWidget: public Ui_AudioOutputWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // AUDIOOUTPUTWIDGET_H
