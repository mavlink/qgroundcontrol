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

#ifndef SCREENTOOLS_H
#define SCREENTOOLS_H

#include <QObject>
#include <QCursor>

/*!
    @brief Screen helper tools for QML widgets
    To use its functions, you need to import the module with the following line:
    @code
    import QGroundControl.ScreenTools 1.0
    @endcode
*/

/// This Qml control is used to return screen parameters
class ScreenTools : public QObject
{
    Q_OBJECT
public:
    ScreenTools();

    Q_PROPERTY(bool     isAndroid           READ isAndroid  CONSTANT)
    Q_PROPERTY(bool     isiOS               READ isiOS      CONSTANT)
    Q_PROPERTY(bool     isMobile            READ isMobile   CONSTANT)

    //! Returns the global mouse X position
    Q_PROPERTY(int      mouseX              READ mouseX)
    //! Returns the global mouse Y position
    Q_PROPERTY(int      mouseY              READ mouseY)
    //! Used to trigger a \c Canvas element repaint.
    /*!
      There is a bug as of Qt 5.4 where a Canvas element defined within a QQuickWidget does not receive
      repaint events. QGC's main window will emit these signals when a \c Canvas element needs to be
      repainted.
      If you use a \c Canvas element inside some QML widget, you can use this code to handle repaint:
      @code
      import QGroundControl.ScreenTools 1.0
      ...
        Canvas {
            id: myCanvas
            height: 40
            width:  40
            Connections {
                target: ScreenTools
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

    //! Font sizes
    Q_PROPERTY(int   font22   READ font22 CONSTANT)
    Q_PROPERTY(int   font21   READ font21 CONSTANT)
    Q_PROPERTY(int   font20   READ font20 CONSTANT)
    Q_PROPERTY(int   font19   READ font19 CONSTANT)
    Q_PROPERTY(int   font18   READ font18 CONSTANT)
    Q_PROPERTY(int   font17   READ font17 CONSTANT)
    Q_PROPERTY(int   font16   READ font16 CONSTANT)
    Q_PROPERTY(int   font15   READ font15 CONSTANT)
    Q_PROPERTY(int   font14   READ font14 CONSTANT)
    Q_PROPERTY(int   font13   READ font13 CONSTANT)
    Q_PROPERTY(int   font12   READ font12 CONSTANT)
    Q_PROPERTY(int   font11   READ font11 CONSTANT)
    Q_PROPERTY(int   font10   READ font10 CONSTANT)
    Q_PROPERTY(int   font9    READ font9  CONSTANT)
    Q_PROPERTY(int   font8    READ font8  CONSTANT)

    //! Returns the system wide default font point size (properly scaled)
    Q_PROPERTY(int   defaultFontPizelSize   READ defaultFontPizelSize   CONSTANT)
    //! Returns the system wide default font point size (properly scaled)
    Q_PROPERTY(int   mediumFontPixelSize    READ mediumFontPixelSize    CONSTANT)
    //! Returns the system wide default font point size (properly scaled)
    Q_PROPERTY(int   largeFontPixelSize     READ largeFontPixelSize     CONSTANT)

    Q_PROPERTY(bool     repaintRequested    READ repaintRequested       NOTIFY repaintRequestedChanged)
    //! Returns the pixel size factor
    Q_PROPERTY(double   pixelSizeFactor     READ pixelSizeFactor        CONSTANT)
    
    //! Utility for adjusting pixel size. Not dynamic (no signals)
    Q_INVOKABLE qreal   adjustPixelSize(qreal pixelSize);

    /// Static version of adjustPixelSize of use in C++ code
    static qreal adjustPixelSize_s(qreal pixelSize);

    int     mouseX              () { return QCursor::pos().x(); }
    int     mouseY              () { return QCursor::pos().y(); }
    bool    repaintRequested    () { return true; }
    double  pixelSizeFactor     () { return _pixelFactor; }

    int   font22    () { return _font22; }
    int   font21    () { return _font21; }
    int   font20    () { return _font20; }
    int   font19    () { return _font19; }
    int   font18    () { return _font18; }
    int   font17    () { return _font17; }
    int   font16    () { return _font16; }
    int   font15    () { return _font15; }
    int   font14    () { return _font14; }
    int   font13    () { return _font13; }
    int   font12    () { return _font12; }
    int   font11    () { return _font11; }
    int   font10    () { return _font10; }
    int   font9     () { return _font9; }
    int   font8     () { return _font8; }

    int  defaultFontPizelSize    () { return _font12; }
    int  mediumFontPixelSize     () { return _font16; }
    int  largeFontPixelSize      () { return _font20; }

    /// Static version for use in C++ code
    static int   font22_s    () { return _font22; }
    static int   font21_s    () { return _font21; }
    static int   font20_s    () { return _font20; }
    static int   font19_s    () { return _font19; }
    static int   font18_s    () { return _font18; }
    static int   font17_s    () { return _font17; }
    static int   font16_s    () { return _font16; }
    static int   font15_s    () { return _font15; }
    static int   font14_s    () { return _font14; }
    static int   font13_s    () { return _font13; }
    static int   font12_s    () { return _font12; }
    static int   font11_s    () { return _font11; }
    static int   font10_s    () { return _font10; }
    static int   font9_s     () { return _font9; }
    static int   font8_s     () { return _font8; }

    static int  defaultFontPizelSize_s      () { return _font12; }
    static int  mediumFontPixelSize_s       () { return _font16; }
    static int  largeFontPixelSize_s        () { return _font20; }

    static double  pixelSizeFactor_s        () { return _pixelFactor; }

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
    bool    isMobile            () { return false; }
#endif

signals:
    void repaintRequestedChanged();

private slots:
    void _updateCanvas();

private:
    // Font Sizes
    static int _font8;
    static int _font9;
    static int _font10;
    static int _font11;
    static int _font12;
    static int _font13;
    static int _font14;
    static int _font15;
    static int _font16;
    static int _font17;
    static int _font18;
    static int _font19;
    static int _font20;
    static int _font21;
    static int _font22;
    // UI Dimension Factors
    static double _pixelFactor;
};

#endif
