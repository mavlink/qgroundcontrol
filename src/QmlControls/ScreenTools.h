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

    Q_PROPERTY(bool     repaintRequested     READ repaintRequested     NOTIFY repaintRequestedChanged)
    //! Returns the font point size factor
    Q_PROPERTY(double   fontPointFactor      READ fontPointFactor      NOTIFY fontPointFactorChanged)
    //! Returns the pixel size factor
    Q_PROPERTY(double   pixelSizeFactor      READ pixelSizeFactor      NOTIFY pixelSizeFactorChanged)
    
    //! Returns the system wide default font point size (properly scaled)
    Q_PROPERTY(double   defaultFontPointSize READ defaultFontPointSize NOTIFY fontSizesChanged)
    //! Returns the system wide default font point size (properly scaled)
    Q_PROPERTY(double   mediumFontPointSize READ mediumFontPointSize NOTIFY fontSizesChanged)
    //! Returns the system wide default font point size (properly scaled)
    Q_PROPERTY(double   largeFontPointSize READ largeFontPointSize NOTIFY fontSizesChanged)

    //! Utility for adjusting font point size. Not dynamic (no signals)
    Q_INVOKABLE qreal   adjustFontPointSize(qreal pointSize);
    //! Utility for adjusting pixel size. Not dynamic (no signals)
    Q_INVOKABLE qreal   adjustPixelSize(qreal pixelSize);

    //! Utility for increasing pixel size.
    Q_INVOKABLE void    increasePixelSize();
    //! Utility for decreasing pixel size.
    Q_INVOKABLE void    decreasePixelSize();
    //! Utility for increasing font size.
    Q_INVOKABLE void    increaseFontSize();
    //! Utility for decreasing font size.
    Q_INVOKABLE void    decreaseFontSize();

    /// Static version of adjustFontPointSize of use in C++ code
    static qreal adjustFontPointSize_s(qreal pointSize);
    /// Static version of adjustPixelSize of use in C++ code
    static qreal adjustPixelSize_s(qreal pixelSize);

    int     mouseX              () { return QCursor::pos().x(); }
    int     mouseY              () { return QCursor::pos().y(); }
    bool    repaintRequested    () { return true; }
    double  fontPointFactor     ();
    double  pixelSizeFactor     ();
    double  defaultFontPointSize(void);
    double  mediumFontPointSize(void);
    double  largeFontPointSize(void);

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
    void pixelSizeFactorChanged();
    void fontPointFactorChanged();
    void fontSizesChanged();

private slots:
    void _updateCanvas();
    void _updatePixelSize();
    void _updateFontSize();

private:
    static const double _defaultFontPointSize;
    static const double _mediumFontPointSize;
    static const double _largeFontPointSize;
};

#endif
