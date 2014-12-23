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

#ifndef QGCPalette_h
#define QGCPalette_h

#include <QObject>
#include <QColor>

class QGCPalette : public QObject
{
    Q_OBJECT
    
    Q_ENUMS(ColorGroup)
    
    Q_PROPERTY(ColorGroup colorGroup READ colorGroup WRITE setColorGroup NOTIFY paletteChanged)

    Q_PROPERTY(QColor alternateBase READ alternateBase NOTIFY paletteChanged)
    Q_PROPERTY(QColor base READ base NOTIFY paletteChanged)
    Q_PROPERTY(QColor button READ button NOTIFY paletteChanged)
    Q_PROPERTY(QColor buttonText READ buttonText NOTIFY paletteChanged)
    Q_PROPERTY(QColor dark READ dark NOTIFY paletteChanged)
    Q_PROPERTY(QColor highlight READ highlight NOTIFY paletteChanged)
    Q_PROPERTY(QColor highlightedText READ highlightedText NOTIFY paletteChanged)
    Q_PROPERTY(QColor light READ light NOTIFY paletteChanged)
    Q_PROPERTY(QColor mid READ mid NOTIFY paletteChanged)
    Q_PROPERTY(QColor midlight READ midlight NOTIFY paletteChanged)
    Q_PROPERTY(QColor shadow READ shadow NOTIFY paletteChanged)
    Q_PROPERTY(QColor text READ text NOTIFY paletteChanged)
    Q_PROPERTY(QColor window READ window NOTIFY paletteChanged)
    Q_PROPERTY(QColor windowText READ windowText NOTIFY paletteChanged)

public:
    enum ColorGroup {
        Disabled,
        Active,
        Inactive
    };
    
    QGCPalette(QObject* parent = NULL);
    ~QGCPalette();
    
    ColorGroup colorGroup(void) const { return _colorGroup; }
    void setColorGroup(ColorGroup colorGroup);
    
    QColor alternateBase(void) const { return _alternateBase[_colorGroup]; }
    QColor base(void) const { return _base[_colorGroup]; }
    QColor button(void) const { return _button[_colorGroup]; }
    QColor buttonText(void) const { return _buttonText[_colorGroup]; }
    QColor dark(void) const { return _dark[_colorGroup]; }
    QColor highlight(void) const { return _highlight[_colorGroup]; }
    QColor highlightedText(void) const { return _highlightedText[_colorGroup]; }
    QColor light(void) const { return _light[_colorGroup]; }
    QColor mid(void) const { return _mid[_colorGroup]; }
    QColor midlight(void) const { return _midlight[_colorGroup]; }
    QColor shadow(void) const { return _shadow[_colorGroup]; }
    QColor text(void) const { return _text[_colorGroup]; }
    QColor window(void) const { return _window[_colorGroup]; }
    QColor windowText(void) const { return _windowText[_colorGroup]; }
    
signals:
    void paletteChanged(void);
    
private:
    ColorGroup _colorGroup;
    
    static bool _paletteLoaded;
    
    static const int _cColorGroups = 3;
    static QColor _alternateBase[_cColorGroups];
    static QColor _base[_cColorGroups];
    static QColor _button[_cColorGroups];
    static QColor _buttonText[_cColorGroups];
    static QColor _dark[_cColorGroups];
    static QColor _highlight[_cColorGroups];
    static QColor _highlightedText[_cColorGroups];
    static QColor _light[_cColorGroups];
    static QColor _mid[_cColorGroups];
    static QColor _midlight[_cColorGroups];
    static QColor _shadow[_cColorGroups];
    static QColor _text[_cColorGroups];
    static QColor _window[_cColorGroups];
    static QColor _windowText[_cColorGroups];
};

#endif
