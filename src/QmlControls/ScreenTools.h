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
    @remark As for the screen density functions, QtQuick provides the \c Screen type (defined in QtQuick.Window)
    but as of Qt 5.4 (QtQuick.Window 2.2), this only works if the main window is QtQuick. As QGC is primarily
    a Qt application and only some of its UI elements are QLM widgets, this does not work. Hence, these function
    defined here.
    @sa <a href="http://doc.qt.io/qt-5/qml-qtquick-window-screen.html">Screen QML Type</a>
*/

/// This Qml control is used to return screen parameters
class ScreenTools : public QObject
{
    Q_OBJECT
public:
    ScreenTools();

    //! Returns the screen density in Dots Per Inch
    Q_PROPERTY(double   screenDPI           READ screenDPI CONSTANT)
    //! Returns a factor used to calculate the font point size to use
    /*!
      When defining fonts in point size, as in:
      @code
      Text {
        text: "Foo Bar"
        font.pointSize: 14
      }
      @endcode
      The size is device dependent. If you define this based on a screen set to 72dpi (Mac OS), once
      this is displayed on a different screen with a different pixel density, such as 96dpi (Windows),
      the text will be displayed in the wrong size.
      Use \c dpiFactor to accomodate for these differences. All font point sizes are given in 72dpi
      and \c dpiFactor returns a factor to use for adjusting it to the current target screen.
      @code
      import QGroundControl.ScreenTools 1.0
      property ScreenTools screenTools: ScreenTools { }
      Text {
        text: "Foo Bar"
        font.pointSize: 14 * screenTools.dpiFactor
      }
      @endcode
     */
    Q_PROPERTY(double   dpiFactor           READ dpiFactor CONSTANT)
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
      property ScreenTools screenTools: ScreenTools { }
      ...
        Canvas {
            id: myCanvas
            height: 40
            width:  40
            Connections {
                target: screenTools
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
    Q_PROPERTY(bool     repaintRequested    READ repaintRequested   NOTIFY repaintRequestedChanged)

    //! Utility for adjusting font point size.
    /*!
      @sa dpiFactor
     */
    Q_INVOKABLE qreal   dpiAdjustedPointSize(qreal pointSize);

    /// Static version of dpiAdjustedPointSize of use in C++ code
    static qreal dpiAdjustedPointSize_s(qreal pointSize);
    
    double  screenDPI           () { return _dotsPerInch; }
    double  dpiFactor           () { return _dpiFactor; }
    int     mouseX              () { return QCursor::pos().x(); }
    int     mouseY              () { return QCursor::pos().y(); }
    bool    repaintRequested    () { return true; }

signals:
    void repaintRequestedChanged();

private slots:
    void _updateCanvas();

private:
    static void _setDpiFactor(void);
    
    static bool _dpiFactorSet;
    static double _dotsPerInch;
    static double _dpiFactor;
};

#endif
