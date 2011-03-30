/********************************************************************************
** Form generated from reading UI file 'MapWidget.ui'
**
** Created: Sun Jul 11 18:53:34 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MapWidget
{
public:

    void setupUi(QWidget *MapWidget) {
        if (MapWidget->objectName().isEmpty())
            MapWidget->setObjectName(QString::fromUtf8("MapWidget"));
        MapWidget->resize(400, 300);

        retranslateUi(MapWidget);

        QMetaObject::connectSlotsByName(MapWidget);
    } // setupUi

    void retranslateUi(QWidget *MapWidget) {
        MapWidget->setWindowTitle(QApplication::translate("MapWidget", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui
{
class MapWidget: public Ui_MapWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAPWIDGET_H
