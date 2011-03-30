/********************************************************************************
** Form generated from reading UI file 'HDDisplay.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef HDDISPLAY_H
#define HDDISPLAY_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGraphicsView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_HDDisplay
{
public:
    QHBoxLayout *horizontalLayout;
    QGraphicsView *view;

    void setupUi(QWidget *HDDisplay) {
        if (HDDisplay->objectName().isEmpty())
            HDDisplay->setObjectName(QString::fromUtf8("HDDisplay"));
        HDDisplay->resize(400, 300);
        horizontalLayout = new QHBoxLayout(HDDisplay);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        view = new QGraphicsView(HDDisplay);
        view->setObjectName(QString::fromUtf8("view"));

        horizontalLayout->addWidget(view);


        retranslateUi(HDDisplay);

        QMetaObject::connectSlotsByName(HDDisplay);
    } // setupUi

    void retranslateUi(QWidget *HDDisplay) {
        HDDisplay->setWindowTitle(QApplication::translate("HDDisplay", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class HDDisplay: public Ui_HDDisplay {};
} // namespace Ui

QT_END_NAMESPACE

#endif // HDDISPLAY_H
