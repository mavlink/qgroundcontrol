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

QList<QGCPalette*>   QGCPalette::_paletteObjects;

QGCPalette::Theme QGCPalette::_theme = QGCPalette::Dark;

QColor QGCPalette::_window[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0xDD, 0xDD, 0xDD), QColor(0xDD, 0xDD, 0xDD) },
    { QColor(0x22, 0x22, 0x22), QColor(0x22, 0x22, 0x22) }
};

QColor QGCPalette::_windowShade[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(204, 204, 204), QColor(204, 204, 204) },
    { QColor(51, 51, 51), QColor(51, 51, 51) }
};

QColor QGCPalette::_windowShadeDark[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(216, 216, 216), QColor(216, 216, 216) },
    { QColor(40, 40, 40), QColor(40, 40, 40) }
};

QColor QGCPalette::_text[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(0, 0, 0) },
    { QColor(0x58, 0x58, 0x58), QColor(0xFF, 0xFF, 0xFF) }
};

QColor QGCPalette::_warningText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
		{ QColor(0xFF, 0xFF, 0x00), QColor(0xFF, 0xFF, 0x00) },
		{ QColor(0xFF, 0xFF, 0x00), QColor(0xFF, 0xFF, 0x00) }
};

QColor QGCPalette::_button[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(0x1b, 0x6f, 0xad) },
    { QColor(0x58, 0x58, 0x58), QColor(98, 98, 100) },
};

QColor QGCPalette::_buttonText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x2c, 0x2c, 0x2c), QColor(0xFF, 0xFF, 0xFF) },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0xFF, 0xFF, 0xFF) },
};

QColor QGCPalette::_buttonHighlight[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(237, 235, 51) },
    { QColor(0x58, 0x58, 0x58), QColor(237, 235, 51) },
};

QColor QGCPalette::_buttonHighlightText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
};

QColor QGCPalette::_primaryButton[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(152, 255, 252) },
    { QColor(0x58, 0x58, 0x58), QColor(152, 255, 252) },
};

QColor QGCPalette::_primaryButtonText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
};

QColor QGCPalette::_textField[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(255, 255, 255) },
    { QColor(0x58, 0x58, 0x58), QColor(255, 255, 255) },
};

QColor QGCPalette::_textFieldText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
};

QGCPalette::QGCPalette(QObject* parent) :
    QObject(parent),
    _colorGroupEnabled(true)
{
    // We have to keep track of all QGCPalette objects in the system so we can signal theme change to all of them
    _paletteObjects += this;
}

QGCPalette::~QGCPalette()
{
    bool fSuccess = _paletteObjects.removeOne(this);
    Q_ASSERT(fSuccess);
    Q_UNUSED(fSuccess);
}

void QGCPalette::setColorGroupEnabled(bool enabled)
{
    _colorGroupEnabled = enabled;
    emit paletteChanged();
}

void QGCPalette::setGlobalTheme(Theme newTheme)
{
    if (_theme != newTheme) {
        _theme = newTheme;
        
        // Notify all objects of the new theme
        foreach(QGCPalette* palette, _paletteObjects) {
            palette->_themeChanged();
        }
    }
}

void QGCPalette::_themeChanged(void)
{
    emit paletteChanged();
}

