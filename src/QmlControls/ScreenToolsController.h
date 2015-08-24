/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
///     @author Gus Grubba <mavlink@grubba.com>

#ifndef ScreenToolsController_H
#define ScreenToolsController_H

#include "QGCApplication.h"

#include <QQuickItem>
#include <QCursor>

/*!
    @brief Screen helper tools for QML widgets
    To use its functions, you need to import the module with the following line:
    @code
    
    @endcode
*/

/// This Qml control is used to return screen parameters
class ScreenToolsController : public QQuickItem
{
    Q_OBJECT
public:
    ScreenToolsController();

    Q_PROPERTY(bool     isAndroid           READ isAndroid  CONSTANT)
    Q_PROPERTY(bool     isiOS               READ isiOS      CONSTANT)
    Q_PROPERTY(bool     isMobile            READ isMobile   CONSTANT)

    //! Used to trigger a \c Canvas element repaint.
    /*!
      There is a bug as of Qt 5.4 where a Canvas element defined within a QQuickWidget does not receive
      repaint events. QGC's main window will emit these signals when a \c Canvas element needs to be
      repainted.
      If you use a \c Canvas element inside some QML widget, you can use this code to handle repaint:
      @code
      import QGroundControl.ScreenToolsController 1.0
      ...
        Canvas {
            id: myCanvas
            height: 40
            width:  40
            Connections {
                target: ScreenToolsController
                onRepaintRequestedChanged: {
                    myCanvas.requestPaint();
                }
            }
            onPaint: {
                var context = getContext("2d");
                ...
            }
        }
      @endcode
     */

    // Returns current mouse position
    Q_INVOKABLE int mouseX(void) { return QCursor::pos().x(); }
    Q_INVOKABLE int mouseY(void) { return QCursor::pos().y(); }
    
	// Used to adjust default font size on an OS basis
	Q_PROPERTY(double defaultFontPixelSizeRatio   MEMBER _defaultFontPixelSizeRatio     CONSTANT)

	// Used to calculate font sizes based on default font size
    Q_PROPERTY(double smallFontPixelSizeRatio   MEMBER _smallFontPixelSizeRatio     CONSTANT)
    Q_PROPERTY(double mediumFontPixelSizeRatio  MEMBER _mediumFontPixelSizeRatio    CONSTANT)
    Q_PROPERTY(double largeFontPixelSizeRatio   MEMBER _largeFontPixelSizeRatio     CONSTANT)
    
    Q_PROPERTY(double qmlDefaultFontPixelSize MEMBER _qmlDefaultFontPixelSize)
    
    static double getQmlDefaultFontPixelSize(void);

    static int  defaultFontPixelSize_s()    { return (int)getQmlDefaultFontPixelSize(); }
	static int  smallFontPixelSize_s()      { return (int)((double)defaultFontPixelSize_s() * _smallFontPixelSizeRatio); }
    static int  mediumFontPixelSize_s()     { return (int)((double)defaultFontPixelSize_s() * _mediumFontPixelSizeRatio); }
    static int  largeFontPixelSize_s()      { return (int)((double)defaultFontPixelSize_s() * _largeFontPixelSizeRatio); }

#if defined (__android__)
    bool    isAndroid           () { return true;  }
    bool    isiOS               () { return false; }
    bool    isMobile            () { return true;  }
#elif defined(__ios__)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return true; }
    bool    isMobile            () { return true; }
#else
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isMobile            () { return qgcApp()->fakeMobile(); }
#endif

signals:
    void repaintRequested(void);

private slots:
    void _updateCanvas();

private:
	static const double _defaultFontPixelSizeRatio;
	static const double _smallFontPixelSizeRatio;
    static const double _mediumFontPixelSizeRatio;
    static const double _largeFontPixelSizeRatio;
    
    static int _qmlDefaultFontPixelSize;
};

#endif
