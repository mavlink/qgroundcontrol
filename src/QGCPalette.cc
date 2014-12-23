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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCPalette.h"

#include <QApplication>
#include <QPalette>

bool QGCPalette::_paletteLoaded = false;

QColor QGCPalette::_alternateBase[QGCPalette::_cColorGroups];
QColor QGCPalette::_base[QGCPalette::_cColorGroups];
QColor QGCPalette::_button[QGCPalette::_cColorGroups];
QColor QGCPalette::_buttonText[QGCPalette::_cColorGroups];
QColor QGCPalette::_dark[QGCPalette::_cColorGroups];
QColor QGCPalette::_highlight[QGCPalette::_cColorGroups];
QColor QGCPalette::_highlightedText[QGCPalette::_cColorGroups];
QColor QGCPalette::_light[QGCPalette::_cColorGroups];
QColor QGCPalette::_mid[QGCPalette::_cColorGroups];
QColor QGCPalette::_midlight[QGCPalette::_cColorGroups];
QColor QGCPalette::_shadow[QGCPalette::_cColorGroups];
QColor QGCPalette::_text[QGCPalette::_cColorGroups];
QColor QGCPalette::_window[QGCPalette::_cColorGroups];
QColor QGCPalette::_windowText[QGCPalette::_cColorGroups];

QGCPalette::QGCPalette(QObject* parent) :
    QObject(parent)
{
    if (!_paletteLoaded) {
        _paletteLoaded = true;
        
        struct Group2Group {
            ColorGroup              qgcColorGroup;
            QPalette::ColorGroup    qtColorGroup;
        };
        
        static struct Group2Group rgGroup2Group[] = {
            { Disabled, QPalette::Disabled },
            { Active, QPalette::Active },
            { Inactive, QPalette::Inactive }
        };
        static const size_t crgGroup2Group = sizeof(rgGroup2Group) / sizeof(rgGroup2Group[0]);
        Q_ASSERT(crgGroup2Group == _cColorGroups);
        
        for (size_t i=0; i<crgGroup2Group; i++) {
            ColorGroup colorGroup = rgGroup2Group[i].qgcColorGroup;
            _window[colorGroup] = QColor(34, 34, 34);
            _windowText[colorGroup] = QColor(255, 255, 255);
        }
        
        for (size_t i=0; i<crgGroup2Group; i++) {
            struct Group2Group* prgGroup2Group = &rgGroup2Group[i];
            
            QPalette syspal = QApplication::palette();
            syspal.setCurrentColorGroup(prgGroup2Group->qtColorGroup);
            
            ColorGroup qgcColorGroup = prgGroup2Group->qgcColorGroup;
            
            _alternateBase[qgcColorGroup] = syspal.color(QPalette::AlternateBase);
            _base[qgcColorGroup] = syspal.color(QPalette::Base);
            _button[qgcColorGroup] = syspal.color(QPalette::Button);
            _buttonText[qgcColorGroup] = syspal.color(QPalette::ButtonText);
            _text[qgcColorGroup] = syspal.color(QPalette::Text);
            
            _shadow[qgcColorGroup] = syspal.shadow().color();
            _dark[qgcColorGroup] = syspal.dark().color();
            _highlight[qgcColorGroup] = syspal.highlight().color();
            _highlightedText[qgcColorGroup] = syspal.highlightedText().color();
            _light[qgcColorGroup] = syspal.light().color();
            _mid[qgcColorGroup] = syspal.mid().color();
            _midlight[qgcColorGroup] = syspal.midlight().color();
        }
    }
}

QGCPalette::~QGCPalette()
{
    
}

void QGCPalette::setColorGroup(ColorGroup colorGroup)
{
    _colorGroup = colorGroup;
    emit paletteChanged();
}
