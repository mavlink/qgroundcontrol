/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef RCChannelWidget_H
#define RCChannelWidget_H

#include <QGroupBox>

/// @brief RC Channel Widget - UI Widget based on QGroupBox which displays the current calibration settings
///         for an RC channel.
class RCChannelWidget : public QGroupBox
{
    Q_OBJECT
    
public:
    explicit RCChannelWidget(QWidget *parent = 0);

    /// @brief Set the current RC value to display
    void setValue(int value);
    
    /// @brief Set the current RC Value, Minimum RC Value and Maximum RC Value
    void setValueAndMinMax(int val, int min, int max);
    
    void setMinMax(int min, int max);
    
    /// @brief Sets the Trim value for the channel
    void setTrim(int value);
    
    int value(void) { return _value; }  ///< Returns the current RC Value set in the control
    int min(void) { return _min; }      ///< Returns the min value set in the control
    int max(void) { return _max; }      ///< Returns the max values set in the control
    int trim(void) { return _trim; }    ///< Returns the trim value set in the control

    void showMinMax(bool show);
    bool isMinMaxShown() { return _showMinMax; }
    
    void showTrim(bool show);
    bool isTrimShown() { return _showTrim; }
    
protected:
    virtual void paintEvent(QPaintEvent *event);
    
private:
    void _drawValuePointer(QPainter* painter, int xTip, int yTip, int height, bool rightSideUp);
    
    int _value;         ///< Current RC value
    int _min;           ///< Min RC value
    int _max;           ///< Max RC value
    int _trim;          ///< RC Value for Trim position
    
    bool _showMinMax;   ///< true: show min max values on display
    bool _showTrim;     ///< true: show trim value on display
    
    static const int _centerValue = 1500;                           ///< RC Value which is at center
    static const int _maxDeltaRange = 700;                          ///< Delta around center value which is the max range for widget
    static const int _minRange = _centerValue - _maxDeltaRange;     ///< Smallest value widget can display
    static const int _maxRange = _centerValue + _maxDeltaRange;     ///< Largest value widget can display
};

#endif
