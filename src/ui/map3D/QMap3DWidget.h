#ifndef QMAP3DWIDGET_H
#define QMAP3DWIDGET_H

#include <QLabel>

#include "Q3DWidget.h"

class CheetahModel;
class UASInterface;

class QMap3DWidget : public Q3DWidget
{
    Q_OBJECT

public:
    explicit QMap3DWidget(QWidget* parent);
    ~QMap3DWidget();

    void buildLayout(void);

    static void display(void* clientData);
    void displayHandler(void);

    static void timer(void* clientData);
    void timerHandler(void);

    double getTime(void) const;

public slots:
    void setActiveUAS(UASInterface* uas);

private slots:
    void showGrid(int state);
    void showTrail(int state);
    void recenterCamera(void);
    void toggleLockCamera(int state);

protected:
    UASInterface* uas;
    void paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter);

private:
    void drawPlatform(float roll, float pitch, float yaw);
    void drawGrid(void);
    void drawTrail(float x, float y, float z);

    double lastRedrawTime;

    bool displayGrid;
    bool displayTrail;
    bool lockCamera;

    typedef struct
    {
        float x;
        float y;
        float z;
    } Pose3D;
    QVarLengthArray<Pose3D, 10000> trail;

    QScopedPointer<CheetahModel> cheetahModel;
};

#endif // QMAP3DWIDGET_H
