#include "QMap3DWidget.h"

//#include <FTGL/ftgl.h>
#include <QCheckBox>
#include <sys/time.h>

#include "CheetahModel.h"
#include "UASManager.h"
#include "UASInterface.h"

QMap3DWidget::QMap3DWidget(QWidget* parent)
     : Q3DWidget(parent)
     , uas(NULL)
     , lastRedrawTime(0.0)
     , displayGrid(false)
     , displayTrail(false)
     , lockCamera(false)
{
    setFocusPolicy(Qt::StrongFocus);

    initialize(10, 10, 1000, 900, 10.0f);
    setCameraParams(0.05f, 0.5f, 0.01f, 0.5f, 30.0f, 0.01f, 400.0f);

    setDisplayFunc(display, this);
    addTimerFunc(100, timer, this);

    buildLayout();

    //font.reset(new FTTextureFont("images/Vera.ttf"));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));
}

QMap3DWidget::~QMap3DWidget()
{

}

void
QMap3DWidget::buildLayout(void)
{
    QCheckBox* gridCheckBox = new QCheckBox(this);
    gridCheckBox->setText("Grid");
    gridCheckBox->setChecked(displayGrid);

    QCheckBox* trailCheckBox = new QCheckBox(this);
    trailCheckBox->setText("Trail");
    trailCheckBox->setChecked(displayTrail);

    QPushButton* recenterButton = new QPushButton(this);
    recenterButton->setText("Recenter Camera");

    QCheckBox* lockCameraCheckBox = new QCheckBox(this);
    lockCameraCheckBox->setText("Lock Camera");
    lockCameraCheckBox->setChecked(lockCamera);

    //positionLabel = new QLabel(this);
    //positionLabel->setText(tr("Waiting for first position update.. "));

    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    //layout->addWidget(mc, 0, 0, 1, 2);
    layout->addWidget(gridCheckBox, 1, 0);
    layout->addWidget(trailCheckBox, 1, 1);
    layout->addWidget(recenterButton, 1, 2);
    layout->addWidget(lockCameraCheckBox, 1, 3);
    // layout->addWidget(positionLabel, 1, 4);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 50);
    setLayout(layout);

    connect(gridCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showGrid(int)));
    connect(trailCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showTrail(int)));
    connect(recenterButton, SIGNAL(clicked()), this, SLOT(recenterCamera()));
    connect(lockCameraCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(toggleLockCamera(int)));
}

void
QMap3DWidget::display(void* clientData)
{
    QMap3DWidget* map3d = reinterpret_cast<QMap3DWidget *>(clientData);
    map3d->displayHandler();
}

void
QMap3DWidget::displayHandler(void)
{
    if (cheetahModel.data() == 0)
    {
        cheetahModel.reset(new CheetahModel);
        cheetahModel->init(1.0f, 1.0f, 1.0f);
    }

    float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
    float robotRoll = 0.0f, robotPitch = 0.0f, robotYaw = 0.0f;
    if (uas != NULL)
    {
        robotX = uas->getLocalX();
        robotY = uas->getLocalY();
        robotZ = uas->getLocalZ();
        robotRoll = uas->getRoll();
        robotPitch = uas->getPitch();
        robotYaw = uas->getYaw();
    }

    setCameraLock(lockCamera);

    // turn on smooth lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // clear window
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw Cheetah model
    drawPlatform(robotRoll, robotPitch, robotYaw);

    if (displayGrid)
    {
        drawGrid();
    }

    if (displayTrail)
    {
        drawTrail(robotX, robotY, robotZ);
    }

    // switch to 2D
    setDisplayMode2D();

    // display pose information
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 45.0f);
    glVertex2f(getWindowWidth(), 45.0f);
    glVertex2f(getWindowWidth(), 0.0f);
    glEnd();

    // QT QPAINTER OPENGL PAINTING

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    paintText(QString("x = %1 y = %2 z = %3 r = %4 p = %5 y = %6").arg(robotX, 0, 'f', 2).arg(robotY, 0, 'f', 2).arg(robotZ, 0, 'f', 2).arg(robotRoll, 0, 'f', 2).arg(robotPitch, 0, 'f', 2).arg(robotYaw, 0, 'f', 2),
              QColor(255, 255, 255),
              12,
              5,
              5,
              &painter);
}

void QMap3DWidget::paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter)
{
    QPen prevPen = painter->pen();

    float pPositionX = refX;
    float pPositionY = refY;

    QFont font("Bitstream Vera Sans");
    // Enforce minimum font size of 5 pixels
    int fSize = qMax(5, (int)(fontSize));
    font.setPixelSize(fSize);

    QFontMetrics metrics = QFontMetrics(font);
    int border = qMax(4, metrics.leading());
    QRect rect = metrics.boundingRect(0, 0, width() - 2*border, int(height()*0.125),
                                      Qt::AlignLeft | Qt::TextWordWrap, text);
    painter->setPen(color);
    painter->setFont(font);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->drawText(pPositionX, pPositionY,
                      rect.width(), rect.height(),
                      Qt::AlignCenter | Qt::TextWordWrap, text);
    painter->setPen(prevPen);
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void QMap3DWidget::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL && this->uas != uas)
    {
        // Disconnect any previously connected active MAV
        //disconnect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    }

    this->uas = uas;
}

void
QMap3DWidget::timer(void* clientData)
{
    QMap3DWidget* map3d = reinterpret_cast<QMap3DWidget *>(clientData);
    map3d->timerHandler();
}

void
QMap3DWidget::timerHandler(void)
{
    double timeLapsed = getTime() - lastRedrawTime;
    if (timeLapsed > 0.1)
    {
        forceRedraw();
        lastRedrawTime = getTime();
    }
    addTimerFunc(100, timer, this);
}

double
QMap3DWidget::getTime(void) const
{
     struct timeval tv;

     gettimeofday(&tv, NULL);

     return static_cast<double>(tv.tv_sec) +
             static_cast<double>(tv.tv_usec) / 1000000.0;
}

void
QMap3DWidget::showGrid(int32_t state)
{
    if (state == Qt::Checked)
    {
        displayGrid = true;
    }
    else
    {
        displayGrid = false;
    }
}

void
QMap3DWidget::showTrail(int32_t state)
{
    if (state == Qt::Checked)
    {
        if (!displayTrail)
        {
            trail.clear();
        }

        displayTrail = true;
    }
    else
    {
        displayTrail = false;
    }
}

void
QMap3DWidget::recenterCamera(void)
{
    recenter();
}

void
QMap3DWidget::toggleLockCamera(int32_t state)
{
    if (state == Qt::Checked)
    {
        lockCamera = true;
    }
    else
    {
        lockCamera = false;
    }
}

void
QMap3DWidget::drawPlatform(float roll, float pitch, float yaw)
{
    glPushMatrix();

    glRotatef(yaw, 0.0f, 0.0f, 1.0f);
    glRotatef(pitch, 0.0f, 1.0f, 0.0f);
    glRotatef(roll, 1.0f, 0.0f, 0.0f);

    glLineWidth(3.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.3f, 0.0f, 0.0f);
    glEnd();

    cheetahModel->draw();

    glPopMatrix();
}

void
QMap3DWidget::drawGrid(void)
{
    float radius = 10.0f;
    float resolution = 0.25f;

    glPushMatrix();

    // draw a 20m x 20m grid with 0.25m resolution
    glColor3f(0.5f, 0.5f, 0.5f);
    for (float i = -radius; i <= radius; i += resolution)
    {
        if (fabsf(i - roundf(i)) < 0.01f)
        {
            glLineWidth(2.0f);
        }
        else
        {
            glLineWidth(0.25f);
        }

        glBegin(GL_LINES);
        glVertex3f(i, -radius, 0.0f);
        glVertex3f(i, radius, 0.0f);
        glVertex3f(-radius, i, 0.0f);
        glVertex3f(radius, i, 0.0f);
        glEnd();
    }

    glPopMatrix();
}

void
QMap3DWidget::drawTrail(float x, float y, float z)
{
    bool addToTrail = false;
    if (trail.size() > 0)
    {
        if (fabsf(x - trail[trail.size() - 1].x) > 0.01f ||
            fabsf(y - trail[trail.size() - 1].y) > 0.01f ||
            fabsf(z - trail[trail.size() - 1].z) > 0.01f)
        {
            addToTrail = true;
        }
    }
    else
    {
        addToTrail = true;
    }

    if (addToTrail)
    {
        Pose3D p = {x, y, z};
        if (trail.size() == trail.capacity())
        {
            memcpy(trail.data(), trail.data() + 1,
                   (trail.size() - 1) * sizeof(Pose3D));
            trail[trail.size() - 1] = p;
        }
        else
        {
            trail.append(p);
        }
    }

    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    for (int32_t i = 0; i < trail.size(); ++i)
    {
        glVertex3f(trail[i].x - x, trail[i].y - y, trail[i].z - z);
    }
    glEnd();
}
