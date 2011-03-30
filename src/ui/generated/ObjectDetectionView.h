/********************************************************************************
** Form generated from reading UI file 'ObjectDetectionView.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef OBJECTDETECTIONVIEW_H
#define OBJECTDETECTIONVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ObjectDetectionView
{
public:
    QGridLayout *gridLayout;
    QListWidget *listWidget;
    QListWidget *letterListWidget;
    QLabel *imageLabel;
    QLabel *letterLabel;
    QLabel *nameLabel;
    QPushButton *clearButton;

    void setupUi(QWidget *ObjectDetectionView) {
        if (ObjectDetectionView->objectName().isEmpty())
            ObjectDetectionView->setObjectName(QString::fromUtf8("ObjectDetectionView"));
        ObjectDetectionView->resize(246, 403);
        gridLayout = new QGridLayout(ObjectDetectionView);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        listWidget = new QListWidget(ObjectDetectionView);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        gridLayout->addWidget(listWidget, 0, 0, 1, 3);

        letterListWidget = new QListWidget(ObjectDetectionView);
        letterListWidget->setObjectName(QString::fromUtf8("letterListWidget"));

        gridLayout->addWidget(letterListWidget, 1, 0, 1, 3);

        imageLabel = new QLabel(ObjectDetectionView);
        imageLabel->setObjectName(QString::fromUtf8("imageLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(10);
        sizePolicy.setVerticalStretch(10);
        sizePolicy.setHeightForWidth(imageLabel->sizePolicy().hasHeightForWidth());
        imageLabel->setSizePolicy(sizePolicy);
        imageLabel->setMinimumSize(QSize(110, 110));
        imageLabel->setSizeIncrement(QSize(2, 2));
        imageLabel->setBaseSize(QSize(10, 10));

        gridLayout->addWidget(imageLabel, 2, 0, 1, 1);

        letterLabel = new QLabel(ObjectDetectionView);
        letterLabel->setObjectName(QString::fromUtf8("letterLabel"));
        sizePolicy.setHeightForWidth(letterLabel->sizePolicy().hasHeightForWidth());
        letterLabel->setSizePolicy(sizePolicy);
        letterLabel->setMinimumSize(QSize(110, 110));
        letterLabel->setSizeIncrement(QSize(2, 2));
        letterLabel->setBaseSize(QSize(10, 10));
        letterLabel->setStyleSheet(QString::fromUtf8("font: 72pt;\n"
                                   "color: white;"));
        letterLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(letterLabel, 2, 1, 1, 2);

        nameLabel = new QLabel(ObjectDetectionView);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));

        gridLayout->addWidget(nameLabel, 3, 0, 1, 2);

        clearButton = new QPushButton(ObjectDetectionView);
        clearButton->setObjectName(QString::fromUtf8("clearButton"));
        clearButton->setMaximumSize(QSize(80, 16777215));

        gridLayout->addWidget(clearButton, 3, 2, 1, 1);


        retranslateUi(ObjectDetectionView);

        QMetaObject::connectSlotsByName(ObjectDetectionView);
    } // setupUi

    void retranslateUi(QWidget *ObjectDetectionView) {
        ObjectDetectionView->setWindowTitle(QApplication::translate("ObjectDetectionView", "Form", 0, QApplication::UnicodeUTF8));
        imageLabel->setText(QString());
        letterLabel->setText(QString());
        nameLabel->setText(QApplication::translate("ObjectDetectionView", "No objects recognized", 0, QApplication::UnicodeUTF8));
        clearButton->setText(QApplication::translate("ObjectDetectionView", "Clear", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class ObjectDetectionView: public Ui_ObjectDetectionView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // OBJECTDETECTIONVIEW_H
