/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Subclass of QComboBox. Mainly used for unit test so you can simulate a user selection
///             with correct signalling.
///
///     @author Don Gagne <don@thegagnes.com>

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
    emit activated(itemText(index));
}
