/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGCPalette.h"

#include <QApplication>
#include <QPalette>

QList<QGCPalette*>   QGCPalette::_paletteObjects;

QGCPalette::Theme QGCPalette::_theme = QGCPalette::Dark;

QColor QGCPalette::_window[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#ffffff"), QColor("#ffffff") },
    { QColor(0x22, 0x22, 0x22), QColor(0x22, 0x22, 0x22) }
};

QColor QGCPalette::_windowShade[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#d9d9d9"), QColor("#d9d9d9") },
    { QColor(51, 51, 51), QColor(51, 51, 51) }
};

QColor QGCPalette::_windowShadeDark[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#bdbdbd"), QColor("#bdbdbd") },
    { QColor(40, 40, 40), QColor(40, 40, 40) }
};

QColor QGCPalette::_text[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#9d9d9d"), QColor("#000000") },
    { QColor(0x58, 0x58, 0x58), QColor(0xFF, 0xFF, 0xFF) }
};

QColor QGCPalette::_warningText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#cc0808"), QColor("#cc0808") },
    { QColor("#e4e428"), QColor("#e4e428") }
};

QColor QGCPalette::_button[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#ffffff"),  QColor("#ffffff") },
    { QColor(0x58, 0x58, 0x58), QColor(98, 98, 100) },
};

QColor QGCPalette::_buttonText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#9d9d9d"), QColor("#000000") },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0xFF, 0xFF, 0xFF) },
};

QColor QGCPalette::_buttonHighlight[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#e4e4e4"), QColor("#33b5e5") },
    { QColor(0x58, 0x58, 0x58), QColor(237, 235, 51) },
};

QColor QGCPalette::_buttonHighlightText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
};

QColor QGCPalette::_primaryButton[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor("#badec5") },
    { QColor(0x58, 0x58, 0x58), QColor("#badec5") },
};

QColor QGCPalette::_primaryButtonText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
};

QColor QGCPalette::_textField[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#ffffff"), QColor("#ffffff") },
    { QColor(0x58, 0x58, 0x58), QColor(255, 255, 255) },
};

QColor QGCPalette::_textFieldText[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#dedede"), QColor("#000000") },
    { QColor(0x2c, 0x2c, 0x2c), QColor(0, 0, 0) },
};

QColor QGCPalette::_mapButton[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(0, 0, 0) },
    { QColor(0x58, 0x58, 0x58), QColor(0, 0, 0) },
};

QColor QGCPalette::_mapButtonHighlight[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0x58, 0x58, 0x58), QColor(190, 120, 28) },
    { QColor(0x58, 0x58, 0x58), QColor(190, 120, 28) },
};

// Map widget colors are not affecting by theming
QColor QGCPalette::_mapWidgetBorderLight[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(255, 255, 255), QColor(255, 255, 255) },
    { QColor(255, 255, 255), QColor(255, 255, 255) },
};

QColor QGCPalette::_mapWidgetBorderDark[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor(0, 0, 0), QColor(0, 0, 0) },
    { QColor(0, 0, 0), QColor(0, 0, 0) },
};

QColor QGCPalette::_brandingPurple[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#4A2C6D"), QColor("#4A2C6D") },
    { QColor("#4A2C6D"), QColor("#4A2C6D") },
};

QColor QGCPalette::_brandingBlue[QGCPalette::_cThemes][QGCPalette::_cColorGroups] = {
    { QColor("#48D6FF"), QColor("#48D6FF") },
    { QColor("#48D6FF"), QColor("#48D6FF") },
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
    // Mobile build does not have themes
    if (_theme != newTheme) {
        _theme = newTheme;
        _signalPaletteChangeToAll();
    }
}

void QGCPalette::_signalPaletteChangeToAll(void)
{
    // Notify all objects of the new theme
    foreach (QGCPalette* palette, _paletteObjects) {
        palette->_signalPaletteChanged();
    }
}


void QGCPalette::_signalPaletteChanged(void)
{
    emit paletteChanged();
}
