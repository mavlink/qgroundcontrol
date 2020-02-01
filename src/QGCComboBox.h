/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QComboBox>

/// @file
///     @brief Subclass of QComboBox. Mainly used for unit test so you can simulate a user selection
///             with correct signalling.
///
///     @author Don Gagne <don@thegagnes.com>

class QGCComboBox : public QComboBox {
    Q_OBJECT
    
public:
    QGCComboBox(QWidget* parent = nullptr);
    
	/// @brief Sets the current index on the combo. Signals activated, as well as currentIndexChanged.
	void simulateUserSetCurrentIndex(int index);
};
