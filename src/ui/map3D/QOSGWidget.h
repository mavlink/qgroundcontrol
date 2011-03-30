/* OpenSceneGraph example, osganimate.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include <osg/Config>

#if defined(_MSC_VER) && defined(OSG_DISABLE_MSVC_WARNINGS)
// disable warning "'QtConcurrent::BlockSizeManager' : assignment operator could not be generated"
#pragma warning( disable : 4512 )
#endif

#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QApplication>
#include <QtGui/QtGui>
#include <QtGui/QWidget>
using Qt::WindowFlags;

#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/GraphicsWindow>

#include <osgViewer/ViewerEventHandlers>

#if defined(WIN32) && !defined(__CYGWIN__)
#include <osgViewer/api/Win32/GraphicsWindowWin32>
typedef HWND WindowHandle;
typedef osgViewer::GraphicsWindowWin32::WindowData WindowData;
#elif defined(__APPLE__)  // Assume using Carbon on Mac.
#include <osgViewer/api/Carbon/GraphicsWindowCarbon>
typedef WindowRef WindowHandle;
typedef osgViewer::GraphicsWindowCarbon::WindowData WindowData;
#else // all other unix
#include <osgViewer/api/X11/GraphicsWindowX11>
typedef Window WindowHandle;
typedef osgViewer::GraphicsWindowX11::WindowData WindowData;
#endif


#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>

#include <osgDB/ReadFile>

#include <iostream>
#include <sstream>

#ifndef QOSGWIDGET_H
#define QOSGWIDGET_H

class QOSGWidget : public QWidget
{
public:

    QOSGWidget( QWidget * parent = 0, const char * name = 0, WindowFlags f = 0, bool overrideTraits = false);

    virtual ~QOSGWidget() {}

    osgViewer::GraphicsWindow* getGraphicsWindow() {
        return _gw.get();
    }
    const osgViewer::GraphicsWindow* getGraphicsWindow() const {
        return _gw.get();
    }

protected:

    void init();
    void createContext();

    //  The GraphincsWindowWin32 implementation already takes care of message handling.
    //  We don't want to relay these on Windows, it will just cause duplicate messages
    //  with further problems downstream (i.e. not being able to throw the trackball
#ifndef WIN32
    virtual void mouseDoubleClickEvent ( QMouseEvent * event );
    virtual void closeEvent( QCloseEvent * event );
    virtual void destroyEvent( bool destroyWindow = true, bool destroySubWindows = true);
    virtual void resizeEvent( QResizeEvent * event );
    virtual void keyPressEvent( QKeyEvent* event );
    virtual void keyReleaseEvent( QKeyEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
#endif
    osg::ref_ptr<osgViewer::GraphicsWindow> _gw;
    bool _overrideTraits;
};

class ViewerQOSG : public osgViewer::Viewer, public QOSGWidget
{
public:

    ViewerQOSG(QWidget * parent = 0, const char * name = 0, WindowFlags f = 0, int fps = 20):
        QOSGWidget( parent, name, f ) {
        setThreadingModel(osgViewer::Viewer::SingleThreaded);

        connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));
        _timer.start(1000.0f/fps);
    }

    void updateCamera() {
        getCamera()->setViewport(new osg::Viewport(0,0,width(),height()));
        getCamera()->setProjectionMatrixAsPerspective(30.0f, 1.0f , static_cast<double>(width())/static_cast<double>(height()), 10000.0f);
        getCamera()->setGraphicsContext(getGraphicsWindow());
    }

    virtual void paintEvent( QPaintEvent * event ) {
        Q_UNUSED(event);
        frame();
    }

protected:

    QTimer _timer;
};


class CompositeViewerQOSG : public osgViewer::CompositeViewer, public QOSGWidget
{
public:
    CompositeViewerQOSG(QWidget * parent = 0, const char * name = 0, WindowFlags f = 0, int fps = 20)
        : QOSGWidget( parent, name, f ) {
        setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);

        connect(&_timer, SIGNAL(timeout()), this, SLOT(repaint()));

        // The composite viewer needs at least one view to work
        // Create a dummy view with a zero sized viewport and no
        // scene to keep the viewer alive.
        osgViewer::View * pView = new osgViewer::View;
        pView->getCamera()->setGraphicsContext( getGraphicsWindow() );
        pView->getCamera()->setViewport( 0, 0, 0, 0 );
        addView( pView );

        // Clear the viewer of removed views
        getGraphicsWindow()->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        getGraphicsWindow()->setClearColor( osg::Vec4( 0.08, 0.08, 0.5, 1.0 ) );

        // The app would hang on exit when using start(1).  Behaves better with 10
        // like the non-composite viewer.  Was this just a typo?
        _timer.start(1000.0f/fps);
    }

    virtual void paintEvent( QPaintEvent * event ) {
        Q_UNUSED(event);
        frame();
    }

    void keyPressEvent( QKeyEvent* event ) {
        if ( event->text() == "a" ) {
            AddView( _scene.get() );
        }

        if ( event->text() == "r" ) {
            RemoveView();
        }

        QOSGWidget::keyPressEvent( event );
    }


    void AddView( osg::Node * scene );
    void RemoveView();
    void Tile();

    osg::ref_ptr< osg::Node > _scene;

protected:
    QTimer _timer;
};

void setupHandlers(osgViewer::View * viewer);


#endif // QOSGWIDGET_H

