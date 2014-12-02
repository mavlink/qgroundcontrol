#include <QMenu>
#include <QContextMenuEvent>
#include <QSettings>

#include "QGCRGBDView.h"
#include "UASManager.h"

QGCRGBDView::QGCRGBDView(int width, int height, QWidget *parent) :
    HUD(width, height, parent),
    rgbEnabled(false),
    depthEnabled(false)
{
    enableRGBAction = new QAction(tr("Enable RGB Image"), this);
    enableRGBAction->setStatusTip(tr("Show the RGB image live stream in this window"));
    enableRGBAction->setCheckable(true);
    enableRGBAction->setChecked(rgbEnabled);
    connect(enableRGBAction, SIGNAL(triggered(bool)), this, SLOT(enableRGB(bool)));

    enableDepthAction = new QAction(tr("Enable Depthmap"), this);
    enableDepthAction->setStatusTip(tr("Show the Depthmap in this window"));
    enableDepthAction->setCheckable(true);
    enableDepthAction->setChecked(depthEnabled);
    connect(enableDepthAction, SIGNAL(triggered(bool)), this, SLOT(enableDepth(bool)));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    clearData();
    loadSettings();
}

QGCRGBDView::~QGCRGBDView()
{
    storeSettings();
}

void QGCRGBDView::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_RGBDWIDGET");
    settings.setValue("STREAM_RGB_ON", rgbEnabled);
    settings.setValue("STREAM_DEPTH_ON", depthEnabled);
    settings.endGroup();
}

void QGCRGBDView::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_RGBDWIDGET");
    rgbEnabled = settings.value("STREAM_RGB_ON", rgbEnabled).toBool();
    // Only enable depth if RGB is not on
    if (!rgbEnabled) depthEnabled = settings.value("STREAM_DEPTH_ON", depthEnabled).toBool();
    settings.endGroup();
}

void QGCRGBDView::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL)
    {
        // Disconnect any previously connected active MAV
        disconnect(this->uas, SIGNAL(rgbdImageChanged(UASInterface*)), this, SLOT(updateData(UASInterface*)));

        clearData();
    }

    if (uas)
    {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(rgbdImageChanged(UASInterface*)), this, SLOT(updateData(UASInterface*)));
    }

    HUD::setActiveUAS(uas);
}

void QGCRGBDView::clearData(void)
{
    QImage offlineImg;
    qDebug() << offlineImg.load(":/files/images/status/colorbars.png");

    glImage = offlineImg;
}

void QGCRGBDView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    // Update actions
    enableHUDAction->setChecked(HUDInstrumentsEnabled);
    //enableVideoAction->setChecked(videoEnabled);
    enableRGBAction->setChecked(rgbEnabled);
    enableDepthAction->setChecked(depthEnabled);

    menu.addAction(enableHUDAction);
    menu.addAction(enableRGBAction);
    menu.addAction(enableDepthAction);
    //menu.addAction(selectHUDColorAction);
    //menu.addAction(enableVideoAction);
    //menu.addAction(selectOfflineDirectoryAction);
    //menu.addAction(selectVideoChannelAction);
    menu.exec(event->globalPos());
}

void QGCRGBDView::enableRGB(bool enabled)
{
    rgbEnabled = enabled;
    videoEnabled = rgbEnabled | depthEnabled;
    QWidget::resize(size().width(), size().height());
}

void QGCRGBDView::enableDepth(bool enabled)
{
    depthEnabled = enabled;
    videoEnabled = rgbEnabled | depthEnabled;
    QWidget::resize(size().width(), size().height());
}

float colormapJet[128][3] = {
    {0.0f,0.0f,0.53125f},
    {0.0f,0.0f,0.5625f},
    {0.0f,0.0f,0.59375f},
    {0.0f,0.0f,0.625f},
    {0.0f,0.0f,0.65625f},
    {0.0f,0.0f,0.6875f},
    {0.0f,0.0f,0.71875f},
    {0.0f,0.0f,0.75f},
    {0.0f,0.0f,0.78125f},
    {0.0f,0.0f,0.8125f},
    {0.0f,0.0f,0.84375f},
    {0.0f,0.0f,0.875f},
    {0.0f,0.0f,0.90625f},
    {0.0f,0.0f,0.9375f},
    {0.0f,0.0f,0.96875f},
    {0.0f,0.0f,1.0f},
    {0.0f,0.03125f,1.0f},
    {0.0f,0.0625f,1.0f},
    {0.0f,0.09375f,1.0f},
    {0.0f,0.125f,1.0f},
    {0.0f,0.15625f,1.0f},
    {0.0f,0.1875f,1.0f},
    {0.0f,0.21875f,1.0f},
    {0.0f,0.25f,1.0f},
    {0.0f,0.28125f,1.0f},
    {0.0f,0.3125f,1.0f},
    {0.0f,0.34375f,1.0f},
    {0.0f,0.375f,1.0f},
    {0.0f,0.40625f,1.0f},
    {0.0f,0.4375f,1.0f},
    {0.0f,0.46875f,1.0f},
    {0.0f,0.5f,1.0f},
    {0.0f,0.53125f,1.0f},
    {0.0f,0.5625f,1.0f},
    {0.0f,0.59375f,1.0f},
    {0.0f,0.625f,1.0f},
    {0.0f,0.65625f,1.0f},
    {0.0f,0.6875f,1.0f},
    {0.0f,0.71875f,1.0f},
    {0.0f,0.75f,1.0f},
    {0.0f,0.78125f,1.0f},
    {0.0f,0.8125f,1.0f},
    {0.0f,0.84375f,1.0f},
    {0.0f,0.875f,1.0f},
    {0.0f,0.90625f,1.0f},
    {0.0f,0.9375f,1.0f},
    {0.0f,0.96875f,1.0f},
    {0.0f,1.0f,1.0f},
    {0.03125f,1.0f,0.96875f},
    {0.0625f,1.0f,0.9375f},
    {0.09375f,1.0f,0.90625f},
    {0.125f,1.0f,0.875f},
    {0.15625f,1.0f,0.84375f},
    {0.1875f,1.0f,0.8125f},
    {0.21875f,1.0f,0.78125f},
    {0.25f,1.0f,0.75f},
    {0.28125f,1.0f,0.71875f},
    {0.3125f,1.0f,0.6875f},
    {0.34375f,1.0f,0.65625f},
    {0.375f,1.0f,0.625f},
    {0.40625f,1.0f,0.59375f},
    {0.4375f,1.0f,0.5625f},
    {0.46875f,1.0f,0.53125f},
    {0.5f,1.0f,0.5f},
    {0.53125f,1.0f,0.46875f},
    {0.5625f,1.0f,0.4375f},
    {0.59375f,1.0f,0.40625f},
    {0.625f,1.0f,0.375f},
    {0.65625f,1.0f,0.34375f},
    {0.6875f,1.0f,0.3125f},
    {0.71875f,1.0f,0.28125f},
    {0.75f,1.0f,0.25f},
    {0.78125f,1.0f,0.21875f},
    {0.8125f,1.0f,0.1875f},
    {0.84375f,1.0f,0.15625f},
    {0.875f,1.0f,0.125f},
    {0.90625f,1.0f,0.09375f},
    {0.9375f,1.0f,0.0625f},
    {0.96875f,1.0f,0.03125f},
    {1.0f,1.0f,0.0f},
    {1.0f,0.96875f,0.0f},
    {1.0f,0.9375f,0.0f},
    {1.0f,0.90625f,0.0f},
    {1.0f,0.875f,0.0f},
    {1.0f,0.84375f,0.0f},
    {1.0f,0.8125f,0.0f},
    {1.0f,0.78125f,0.0f},
    {1.0f,0.75f,0.0f},
    {1.0f,0.71875f,0.0f},
    {1.0f,0.6875f,0.0f},
    {1.0f,0.65625f,0.0f},
    {1.0f,0.625f,0.0f},
    {1.0f,0.59375f,0.0f},
    {1.0f,0.5625f,0.0f},
    {1.0f,0.53125f,0.0f},
    {1.0f,0.5f,0.0f},
    {1.0f,0.46875f,0.0f},
    {1.0f,0.4375f,0.0f},
    {1.0f,0.40625f,0.0f},
    {1.0f,0.375f,0.0f},
    {1.0f,0.34375f,0.0f},
    {1.0f,0.3125f,0.0f},
    {1.0f,0.28125f,0.0f},
    {1.0f,0.25f,0.0f},
    {1.0f,0.21875f,0.0f},
    {1.0f,0.1875f,0.0f},
    {1.0f,0.15625f,0.0f},
    {1.0f,0.125f,0.0f},
    {1.0f,0.09375f,0.0f},
    {1.0f,0.0625f,0.0f},
    {1.0f,0.03125f,0.0f},
    {1.0f,0.0f,0.0f},
    {0.96875f,0.0f,0.0f},
    {0.9375f,0.0f,0.0f},
    {0.90625f,0.0f,0.0f},
    {0.875f,0.0f,0.0f},
    {0.84375f,0.0f,0.0f},
    {0.8125f,0.0f,0.0f},
    {0.78125f,0.0f,0.0f},
    {0.75f,0.0f,0.0f},
    {0.71875f,0.0f,0.0f},
    {0.6875f,0.0f,0.0f},
    {0.65625f,0.0f,0.0f},
    {0.625f,0.0f,0.0f},
    {0.59375f,0.0f,0.0f},
    {0.5625f,0.0f,0.0f},
    {0.53125f,0.0f,0.0f},
    {0.5f,0.0f,0.0f}
};

void QGCRGBDView::updateData(UASInterface *uas)
{
	Q_UNUSED(uas);
}
