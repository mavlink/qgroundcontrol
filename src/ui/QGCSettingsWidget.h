#ifndef QGCSETTINGSWIDGET_H
#define QGCSETTINGSWIDGET_H

#include <QDialog>
#include "MainWindow.h"

namespace Ui
{
class QGCSettingsWidget;
}

class QGCSettingsWidget : public QDialog
{
    Q_OBJECT

public:
    QGCSettingsWidget(JoystickInput *joystick, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    ~QGCSettingsWidget();

public slots:
    void styleChanged(int index);
    void lineEditFinished();
    void setDefaultStyle();
    void selectStylesheet();
    void selectCustomMode(int mode);

private slots:
    void _deleteSettingsToggled(bool checked);
    
private:
    MainWindow* mainWindow;
    Ui::QGCSettingsWidget* ui;
    bool updateStyle(QString style);
};

#endif // QGCSETTINGSWIDGET_H
