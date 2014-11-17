/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef PARAMETEREDITOR_H
#define PARAMETEREDITOR_H

#include <QWidget>

#include "UASInterface.h"

/// @file
///     @brief This is the Parameter Editor widget which is used in the Parameters tab of Vehicle Setup.
///     @author Don Gagne <don@thegagnes.com>

namespace Ui {
    class ParameterEditor;
}

class ParameterEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterEditor(UASInterface* uas, const QStringList& filterList, QWidget* parent = 0);
    ~ParameterEditor();

private:
    Ui::ParameterEditor*    _ui;
};

#endif
