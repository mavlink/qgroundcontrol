/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCComboBox.h"

QGCComboBox::QGCComboBox(QWidget* parent) :
    QComboBox(parent)
{

}

void QGCComboBox::simulateUserSetCurrentIndex(int index)
{
    Q_ASSERT(index >=0 && index < count());
    
    // This will signal currentIndexChanged
    setCurrentIndex(index);
    
    // We have to manually signal activated
    emit activated(index);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    emit activated(itemText(index));
#else
    emit textActivated(itemText(index));
#endif
}
