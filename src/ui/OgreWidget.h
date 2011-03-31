#include "ogrewidget.h"

#define THIS OgreWidget

/**
 * @brief init the object
 * @author kito berg-taylor
 */
void THIS::init( std::string plugins_file,
                 std::string ogre_cfg_file,
                 std::string ogre_log )
{

    // create the main ogre object
    mOgreRoot = new Ogre::Root( plugins_file, ogre_cfg_file, ogre_log );

    // setup a renderer
    Ogre::RenderSystemList *renderers = mOgreRoot->getAvailableRenderers();
    assert( !renderers->empty() ); // we need at least one renderer to do anything useful

    Ogre::RenderSystem *renderSystem;
    renderSystem = chooseRenderer( renderers );

    assert( renderSystem ); // user might pass back a null renderer, which would be bad!

    mOgreRoot->setRenderSystem( renderSystem );
    QString dimensions = QString( "%1x%2" )
                         .arg(this->width())
                         .arg(this->height());

    renderSystem->setConfigOption( "Video Mode", dimensions.toStdString() );

    // initialize without creating window
    mOgreRoot->getRenderSystem()->setConfigOption( "Full Screen", "No" );
    mOgreRoot->saveConfig();
    mOgreRoot->initialise(false); // don't create a window
}

/**
 * @brief setup the rendering context
 * @author Kito Berg-Taylor
 */
void THIS::initializeGL()
{
    //== Creating and Acquiring Ogre Window ==//

    // Get the parameters of the window QT created
    QX11Info info = x11Info();
    Ogre::String winHandle;
    winHandle  = Ogre::StringConverter::toString((unsigned long)(info.display()));
    winHandle += ":";
    winHandle += Ogre::StringConverter::toString((unsigned int)(info.screen()));
    winHandle += ":";
    winHandle += Ogre::StringConverter::toString((unsigned long)(this->parentWidget()->winId()));

    Ogre::NameValuePairList params;
    params["parentWindowHandle"] = winHandle;

    mOgreWindow = mOgreRoot->createRenderWindow( "QOgreWidget_RenderWindow",
                  this->width(),
                  this->height(),
                  false,
                  &params );

    mOgreWindow->setActive(true);
    WId ogreWinId = 0x0;
    mOgreWindow->getCustomAttribute( "WINDOW", &ogreWinId );

    assert( ogreWinId );

    this->create( ogreWinId );
    setAttribute( Qt::WA_PaintOnScreen, true );
    setAttribute( Qt::WA_NoBackground );

    //== Ogre Initialization ==//
    Ogre::SceneType scene_manager_type = Ogre::ST_EXTERIOR_CLOSE;

    mSceneMgr = mOgreRoot->createSceneManager( scene_manager_type );
    mSceneMgr->setAmbientLight( Ogre::ColourValue(1,1,1) );

    mCamera = mSceneMgr->createCamera( "QOgreWidget_Cam" );
    mCamera->setPosition( Ogre::Vector3(0,1,0) );
    mCamera->lookAt( Ogre::Vector3(0,0,0) );
    mCamera->setNearClipDistance( 1.0 );

    Ogre::Viewport *mViewport = mOgreWindow->addViewport( mCamera );
    mViewport->setBackgroundColour( Ogre::ColourValue( 0.8,0.8,1 ) );
}

/**
 * @brief render a frame
 * @author Kito Berg-Taylor
 */
void THIS::paintGL()
{
    assert( mOgreWindow );
    mOgreRoot->renderOneFrame();
}

/**
 * @brief resize the GL window
 * @author Kito Berg-Taylor
 */
void THIS::resizeGL( int width, int height )
{
    assert( mOgreWindow );
    mOgreWindow->windowMovedOrResized();
}

/**
 * @brief choose the right renderer
 * @author Kito Berg-Taylor
 */
Ogre::RenderSystem* THIS::chooseRenderer( Ogre::RenderSystemList *renderers )
{
    // It would probably be wise to do something more friendly
    // that just use the first available renderer
    return *renderers->begin();
}
