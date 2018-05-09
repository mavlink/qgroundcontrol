/**
 * @file
 *   @brief 3dConnexion 3dMouse interface for QGroundControl
 *
 *   @author Matthias Krebs <makrebs@student.ethz.ch>
 *
 */

#include "Mouse6dofInput.h"
#include "UAS.h"
#include "MultiVehicleManager.h"
#include "QGCMessageBox.h"

#ifdef QGC_MOUSE_ENABLED_LINUX
#include <QX11Info>
#include <X11/Xlib.h>
#ifdef Success
#undef Success              // Eigen library doesn't work if Success is defined
#endif //Success
extern "C"
{
#include "xdrvlib.h"
}
#endif // QGC_MOUSE_ENABLED_LINUX

#ifdef QGC_MOUSE_ENABLED_WIN
Mouse6dofInput::Mouse6dofInput(Mouse3DInput* mouseInput) :
    mouse3DMax(0.075),   // TODO: check maximum value fot plugged device
    uas(NULL),
    done(false),
    mouseActive(false),
    translationActive(true),
    rotationActive(true),
    xValue(0.0),
    yValue(0.0),
    zValue(0.0),
    aValue(0.0),
    bValue(0.0),
    cValue(0.0)
{
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &Mouse6dofInput::_activeVehicleChanged);
    
    // Connect 3DxWare SDK MotionEvent
    connect(mouseInput, SIGNAL(Move3d(std::vector<float>&)), this, SLOT(motion3DMouse(std::vector<float>&)));
    connect(mouseInput, SIGNAL(On3dmouseKeyDown(int)), this, SLOT(button3DMouseDown(int)));
    //connect(mouseInput, SIGNAL(On3dmouseKeyUp(int)), this, SLOT(FUNCTION(int)));

}
#endif //QGC_MOUSE_ENABLED_WIN

#ifdef QGC_MOUSE_ENABLED_LINUX
Mouse6dofInput::Mouse6dofInput(QWidget* parent) :
    mouse3DMax(350.0),   // TODO: check maximum value fot plugged device
    uas(NULL),
    done(false),
    mouseActive(false),
    translationActive(true),
    rotationActive(true),
    xValue(0.0),
    yValue(0.0),
    zValue(0.0),
    aValue(0.0),
    bValue(0.0),
    cValue(0.0)
{
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &Mouse6dofInput::_activeVehicleChanged);

    if (!mouseActive)
    {
//        // man visudo --> then you can omit giving password (success not guarantied..)
//        qDebug() << "Starting 3DxWare Daemon for 3dConnexion 3dMouse";
//        QString processProgramm = "gksudo";
//        QStringList processArguments;
//        processArguments << "/etc/3DxWare/daemon/3dxsrv -d usb";
//        process3dxDaemon = new QProcess();
//        process3dxDaemon->start(processProgramm, processArguments);
//    //    process3dxDaemon->waitForFinished();
//    //    {
//    //        qDebug() << "... continuing without 3DxWare. May not be initialized properly!";
//    //        qDebug() << "Try in terminal as user root:" << processArguments.last();
//    //    }

        Display *display = QX11Info::display();
        if(!display)
        {
            qDebug() << "Cannot open display!" << endl;
        }
        if ( !MagellanInit( display, parent->winId() ) )
        {
            QGCMessageBox::critical(tr("No 3DxWare driver is running."),
                                    tr("Enter in Terminal 'sudo /etc/3DxWare/daemon/3dxsrv -d usb' and then restart QGroundControl."));
            qDebug() << "No 3DxWare driver is running!";
            return;
        }
        else
        {
            qDebug() << "Initialized 3dMouse";
            mouseActive = true;
        }
    }
    else
    {
        qDebug() << "3dMouse already initialized..";
    }

}
#endif //QGC_MOUSE_ENABLED_LINUX


Mouse6dofInput::~Mouse6dofInput()
{
    done = true;
}

void Mouse6dofInput::_activeVehicleChanged(Vehicle* vehicle)
{
    if (this->uas)
    {
        disconnect(this, SIGNAL(mouse6dofChanged(double,double,double,double,double,double)), uas, SLOT(setManual6DOFControlCommands(double,double,double,double,double,double)));
        // Todo: disconnect button mapping
        uas = NULL;
    }

    if (vehicle) {
        uas = vehicle->uas();
        
        connect(this, SIGNAL(mouse6dofChanged(double,double,double,double,double,double)), uas, SLOT(setManual6DOFControlCommands(double,double,double,double,double,double)));
            // Todo: connect button mapping
    }
    
    if (!isRunning())
    {
        start();
    }
}

void Mouse6dofInput::init()
{
    // Make sure active UAS is set
    _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
}

void Mouse6dofInput::run()
{
    init();

    forever
    {
        if (done)
        {
           done = false;
           exit();
        }

        if (mouseActive)
        {
            // Bound x value
            if (xValue > 1.0) xValue = 1.0;
            if (xValue < -1.0) xValue = -1.0;
            // Bound x value
            if (yValue > 1.0) yValue = 1.0;
            if (yValue < -1.0) yValue = -1.0;
            // Bound x value
            if (zValue > 1.0) zValue = 1.0;
            if (zValue < -1.0) zValue = -1.0;
            // Bound x value
            if (aValue > 1.0) aValue = 1.0;
            if (aValue < -1.0) aValue = -1.0;
            // Bound x value
            if (bValue > 1.0) bValue = 1.0;
            if (bValue < -1.0) bValue = -1.0;
            // Bound x value
            if (cValue > 1.0) cValue = 1.0;
            if (cValue < -1.0) cValue = -1.0;

            emit mouse6dofChanged(xValue, yValue, zValue, aValue, bValue, cValue);
        }

        // Sleep, update rate of 3d mouse is approx. 50 Hz (1000 ms / 50 = 20 ms)
        QGC::SLEEP::msleep(20);
    }
}

#ifdef QGC_MOUSE_ENABLED_WIN
void Mouse6dofInput::motion3DMouse(std::vector<float> &motionData)
{
    if (motionData.size() < 6) return;
    mouseActive = true;

    if (translationActive)
    {
        xValue = (double)1.0e2f*motionData[ 1 ] / mouse3DMax;
        yValue = (double)1.0e2f*motionData[ 0 ] / mouse3DMax;
        zValue = (double)1.0e2f*motionData[ 2 ] / mouse3DMax;
    }else{
        xValue = 0;
        yValue = 0;
        zValue = 0;
    }
    if (rotationActive)
    {
        aValue = (double)1.0e2f*motionData[ 4 ] / mouse3DMax;
        bValue = (double)1.0e2f*motionData[ 3 ] / mouse3DMax;
        cValue = (double)1.0e2f*motionData[ 5 ] / mouse3DMax;
    }else{
        aValue = 0;
        bValue = 0;
        cValue = 0;
    }

    //qDebug() << "NEW 3D MOUSE VALUES -- X" << xValue << " -- Y" << yValue << " -- Z" << zValue << " -- A" << aValue << " -- B" << bValue << " -- C" << cValue;
}
#endif //QGC_MOUSE_ENABLED_WIN

#ifdef QGC_MOUSE_ENABLED_WIN
void Mouse6dofInput::button3DMouseDown(int button)
{
    switch(button)
    {
    case 1:
    {
            rotationActive = !rotationActive;
            emit mouseRotationActiveChanged(rotationActive);
            qDebug() << "Changed 3DMouse Rotation to " << (bool)rotationActive;
        break;
    }
    case 2:
    {
            translationActive = !translationActive;
            emit mouseTranslationActiveChanged(translationActive);
            qDebug() << "Changed 3DMouse Translation to" << (bool)translationActive;
        break;
    }
    default:
        break;
    }
}
#endif //QGC_MOUSE_ENABLED_WIN

#ifdef QGC_MOUSE_ENABLED_LINUX
void Mouse6dofInput::handleX11Event(XEvent *event)
{
    //qDebug("XEvent occurred...");
    if (!mouseActive)
    {
        qDebug() << "3dMouse not initialized. Cancelled handling X11event for 3dMouse";
        return;
    }

    MagellanFloatEvent MagellanEvent;

    Display *display = QX11Info::display();
    if(!display)
    {
        qDebug() << "Cannot open display!" << endl;
    }

    switch (event->type)
    {
    case ClientMessage:
      switch( MagellanTranslateEvent( display, event, &MagellanEvent, 1.0, 1.0 ) )
      {
        case MagellanInputMotionEvent :
             MagellanRemoveMotionEvents( display );
             for (int i = 0; i < 6; i++) {  // Saturation
                 MagellanEvent.MagellanData[i] = (abs(MagellanEvent.MagellanData[i]) < mouse3DMax) ? MagellanEvent.MagellanData[i] : (mouse3DMax*MagellanEvent.MagellanData[i]/abs(MagellanEvent.MagellanData[i]));
             }

             // Check whether translational motions are enabled
             if (translationActive)
             {
                 xValue = MagellanEvent.MagellanData[ MagellanZ ] / mouse3DMax;
                 yValue = MagellanEvent.MagellanData[ MagellanX ] / mouse3DMax;
                 zValue = - MagellanEvent.MagellanData[ MagellanY ] / mouse3DMax;
             }else{
                 xValue = 0;
                 yValue = 0;
                 zValue = 0;
             }
             // Check whether rotational motions are enabled
             if (rotationActive)
             {
                 aValue = MagellanEvent.MagellanData[ MagellanC ] / mouse3DMax;
                 bValue = MagellanEvent.MagellanData[ MagellanA ] / mouse3DMax;
                 cValue = - MagellanEvent.MagellanData[ MagellanB ] / mouse3DMax;
             }else{
                 aValue = 0;
                 bValue = 0;
                 cValue = 0;
             }
             //qDebug() << "NEW 3D MOUSE VALUES -- X" << xValue << " -- Y" << yValue << " -- Z" << zValue << " -- A" << aValue << " -- B" << bValue << " -- C" << cValue;
        break;

        case MagellanInputButtonPressEvent :
            qDebug() << "MagellanInputButtonPressEvent called with button " << MagellanEvent.MagellanButton;
            switch (MagellanEvent.MagellanButton)
            {
            case 1:
            {
                    rotationActive = !rotationActive;
                    emit mouseRotationActiveChanged(rotationActive);
                    qDebug() << "Changed 3DMouse Rotation to " << (bool)rotationActive;
                break;
            }
            case 2:
            {
                    translationActive = !translationActive;
                    emit mouseTranslationActiveChanged(translationActive);
                    qDebug() << "Changed 3DMouse Translation to" << (bool)translationActive;
                break;
            }
            default:
                break;
            }
        default:
            break;
        }
    }
}
#endif //QGC_MOUSE_ENABLED_LINUX
