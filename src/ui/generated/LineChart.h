/********************************************************************************
** Form generated from reading UI file 'LineChart.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef LINECHART_H
#define LINECHART_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_linechart
{
public:
    QHBoxLayout *horizontalLayout_2;
    QGroupBox *curveGroupBox;
    QVBoxLayout *verticalLayout;
    QScrollArea *curveListWidget;
    QWidget *scrollAreaWidgetContents;
    QGroupBox *diagramGroupBox;

    void setupUi(QWidget *linechart) {
        if (linechart->objectName().isEmpty())
            linechart->setObjectName(QString::fromUtf8("linechart"));
        linechart->resize(662, 381);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(3);
        sizePolicy.setVerticalStretch(2);
        sizePolicy.setHeightForWidth(linechart->sizePolicy().hasHeightForWidth());
        linechart->setSizePolicy(sizePolicy);
        linechart->setMinimumSize(QSize(300, 200));
        horizontalLayout_2 = new QHBoxLayout(linechart);
        horizontalLayout_2->setSpacing(3);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(2, 0, 2, 2);
        curveGroupBox = new QGroupBox(linechart);
        curveGroupBox->setObjectName(QString::fromUtf8("curveGroupBox"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(curveGroupBox->sizePolicy().hasHeightForWidth());
        curveGroupBox->setSizePolicy(sizePolicy1);
        curveGroupBox->setMinimumSize(QSize(250, 300));
        curveGroupBox->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        verticalLayout = new QVBoxLayout(curveGroupBox);
        verticalLayout->setSpacing(2);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        curveListWidget = new QScrollArea(curveGroupBox);
        curveListWidget->setObjectName(QString::fromUtf8("curveListWidget"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(curveListWidget->sizePolicy().hasHeightForWidth());
        curveListWidget->setSizePolicy(sizePolicy2);
        curveListWidget->setMinimumSize(QSize(240, 250));
        curveListWidget->setBaseSize(QSize(150, 200));
        curveListWidget->setAutoFillBackground(false);
        curveListWidget->setFrameShape(QFrame::NoFrame);
        curveListWidget->setFrameShadow(QFrame::Sunken);
        curveListWidget->setWidgetResizable(true);
        curveListWidget->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 242, 361));
        curveListWidget->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(curveListWidget);


        horizontalLayout_2->addWidget(curveGroupBox);

        diagramGroupBox = new QGroupBox(linechart);
        diagramGroupBox->setObjectName(QString::fromUtf8("diagramGroupBox"));
        QSizePolicy sizePolicy3(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy3.setHorizontalStretch(9);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(diagramGroupBox->sizePolicy().hasHeightForWidth());
        diagramGroupBox->setSizePolicy(sizePolicy3);
        diagramGroupBox->setMinimumSize(QSize(400, 300));
        diagramGroupBox->setBaseSize(QSize(800, 300));

        horizontalLayout_2->addWidget(diagramGroupBox);


        retranslateUi(linechart);

        QMetaObject::connectSlotsByName(linechart);
    } // setupUi

    void retranslateUi(QWidget *linechart) {
        linechart->setWindowTitle(QApplication::translate("linechart", "Form", 0, QApplication::UnicodeUTF8));
        curveGroupBox->setTitle(QApplication::translate("linechart", "Curve Selection", 0, QApplication::UnicodeUTF8));
        diagramGroupBox->setTitle(QApplication::translate("linechart", "Diagram", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class linechart: public Ui_linechart {};
} // namespace Ui

QT_END_NAMESPACE

#endif // LINECHART_H
