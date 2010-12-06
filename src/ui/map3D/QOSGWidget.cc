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

#include "QOSGWidget.h"

#ifdef Q_OS_MACX

#endif

QOSGWidget::QOSGWidget( QWidget * parent, const char * name, WindowFlags f, bool overrideTraits):
    QWidget(parent, f), _overrideTraits (overrideTraits)
{
    createContext();
    
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::ClickFocus);   
}

void QOSGWidget::createContext()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance();

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;

    traits->readDISPLAY();
    if (traits->displayNum<0) traits->displayNum = 0;

    traits->windowName = "osgViewerQt";
    traits->screenNum = 0;
    traits->x = x();
    traits->y = y();
    traits->width = width();
    traits->height = height();
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();

#if defined(__APPLE__) 
    // Extract a WindowPtr from the HIViewRef that QWidget::winId() returns.
    // Without this change, the peer tries to call GetWindowPort on the HIViewRef
    // which returns 0 and we only render white.
    traits->inheritedWindowData = new WindowData(HIViewGetWindow((HIViewRef)winId()));

#else // all others
    traits->inheritedWindowData = new WindowData(winId());
#endif


    if (ds->getStereo())
    {
        switch(ds->getStereoMode())
        {
            case(osg::DisplaySettings::QUAD_BUFFER): traits->quadBufferStereo = true; break;
            case(osg::DisplaySettings::VERTICAL_INTERLACE):
            case(osg::DisplaySettings::CHECKERBOARD):
            case(osg::DisplaySettings::HORIZONTAL_INTERLACE): traits->stencil = 8; break;
            default: break;
        }
    }

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    _gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());

    // get around dearanged traits on X11 (MTCompositeViewer only)
    if (_overrideTraits)
    {
        traits->x = x();
        traits->y = y();
        traits->width = width();
        traits->height = height();
    }

}

#ifndef WIN32 
void QOSGWidget::destroyEvent(bool destroyWindow, bool destroySubWindows)
{   
    _gw->getEventQueue()->closeWindow();
}


void QOSGWidget::closeEvent( QCloseEvent * event )
{
    event->accept();

    _gw->getEventQueue()->closeWindow();
}


void QOSGWidget::resizeEvent( QResizeEvent * event )
{
    const QSize & size = event->size();
    _gw->getEventQueue()->windowResize(0, 0, size.width(), size.height() );
    _gw->resized(0, 0, size.width(), size.height());
}

void QOSGWidget::keyPressEvent( QKeyEvent* event )
{
    _gw->getEventQueue()->keyPress( (osgGA::GUIEventAdapter::KeySymbol) *(event->text().toAscii().data() ) );
}

void QOSGWidget::keyReleaseEvent( QKeyEvent* event )
{
    int c = *event->text().toAscii().data();

    _gw->getEventQueue()->keyRelease( (osgGA::GUIEventAdapter::KeySymbol) (c) );
}

void QOSGWidget::mousePressEvent( QMouseEvent* event )
{
    int button = 0;
    switch(event->button())
    {
        case(Qt::LeftButton): button = 1; break;
        case(Qt::MidButton): button = 2; break;
        case(Qt::RightButton): button = 3; break;
        case(Qt::NoButton): button = 0; break;
        default: button = 0; break;
    }
    _gw->getEventQueue()->mouseButtonPress(event->x(), event->y(), button);
}
void QOSGWidget::mouseDoubleClickEvent ( QMouseEvent * event )
{
    int button = 0;
    switch(event->button())
    {
        case(Qt::LeftButton): button = 1; break;
        case(Qt::MidButton): button = 2; break;
        case(Qt::RightButton): button = 3; break;
        case(Qt::NoButton): button = 0; break;
        default: button = 0; break;
    }
    _gw->getEventQueue()->mouseDoubleButtonPress(event->x(), event->y(), button);
}
void QOSGWidget::mouseReleaseEvent( QMouseEvent* event )
{
    int button = 0;
    switch(event->button())
    {
        case(Qt::LeftButton): button = 1; break;
        case(Qt::MidButton): button = 2; break;
        case(Qt::RightButton): button = 3; break;
        case(Qt::NoButton): button = 0; break;
        default: button = 0; break;
    }
    _gw->getEventQueue()->mouseButtonRelease(event->x(), event->y(), button);
}

void QOSGWidget::mouseMoveEvent( QMouseEvent* event )
{
    _gw->getEventQueue()->mouseMotion(event->x(), event->y());
}
#endif




void CompositeViewerQOSG::Tile()
{
  int n = getNumViews() - 1; // -1 to account for dummy view

  for ( int i = 0; i < n; ++i )
  {
    osgViewer::View * view = getView(i+1);  // +1 to account for dummy view
    view->getCamera()->setViewport( new osg::Viewport( 0, i*height()/n , width(), height()/n ) );
    view->getCamera()->setProjectionMatrixAsPerspective( 30.0f, double( width() ) / double( height()/n ), 1.0f, 10000.0f );
  }
}


void CompositeViewerQOSG::AddView( osg::Node * scene )
{
  osgViewer::View* view = new osgViewer::View;
  addView(view);

  view->setSceneData( scene );
  view->setCameraManipulator(new osgGA::TrackballManipulator);

  // add the state manipulator
  osg::ref_ptr<osgGA::StateSetManipulator> statesetManipulator = new osgGA::StateSetManipulator;
  statesetManipulator->setStateSet(view->getCamera()->getOrCreateStateSet());

  view->getCamera()->setGraphicsContext( getGraphicsWindow() );
  view->getCamera()->setClearColor( osg::Vec4( 0.08, 0.08, 0.5, 1.0 ) );
  Tile();
}

void CompositeViewerQOSG::RemoveView()
{
  if ( getNumViews() > 1 )
  {
    removeView( getView( getNumViews() - 1 ) );
  }
  Tile();
}

// we use this wrapper for CompositeViewer ONLY because of the timer
// NOTE: this is a workaround because we're not using QT's moc precompiler here.
//
class QViewerTimer : public QWidget
{

    public:

        QViewerTimer (int fps = 20, QWidget * parent = 0, WindowFlags f = 0):
            QWidget (parent, f)
    {
        _viewer = new osgViewer::CompositeViewer ();
        _viewer->setThreadingModel(osgViewer::CompositeViewer::DrawThreadPerContext);
        connect(&_timer, SIGNAL(timeout()), this, SLOT(repaint()));
        _timer.start(1000.0f/fps);
    }

    ~QViewerTimer ()
    {
        _timer.stop ();
    }

        virtual void paintEvent (QPaintEvent * event) { _viewer->frame(); }

        osg::ref_ptr <osgViewer::CompositeViewer> _viewer;
        QTimer _timer;

};

void setupHandlers(osgViewer::View * viewer)
{
    // add the state manipulator
    viewer->addEventHandler( new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()) );

    // add the thread model handler
    viewer->addEventHandler(new osgViewer::ThreadingHandler);

    // add the window size toggle handler
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);

    // add the stats handler
    viewer->addEventHandler(new osgViewer::StatsHandler);
}
