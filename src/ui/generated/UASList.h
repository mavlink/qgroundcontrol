/********************************************************************************
** Form generated from reading UI file 'UASList.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UASLIST_H
#define UASLIST_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UASList
{
public:

    void setupUi(QWidget *UASList) {
        if (UASList->objectName().isEmpty())
            UASList->setObjectName(QString::fromUtf8("UASList"));
        UASList->resize(400, 300);
        UASList->setMinimumSize(QSize(500, 0));

        retranslateUi(UASList);

        QMetaObject::connectSlotsByName(UASList);
    } // setupUi

    void retranslateUi(QWidget *UASList) {
        UASList->setWindowTitle(QApplication::translate("UASList", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class UASList: public Ui_UASList {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UASLIST_H
